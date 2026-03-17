#include "corpc/async_loop.hpp"
#include "corpc/epoll_loop.hpp"
#include "corpc/future.hpp"
#include "corpc/when_any.hpp"
#include "elog/logger.h"
#include <arpa/inet.h>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

using namespace corpc;
using namespace std::chrono_literals;

AsyncLoop main_loop;
AsyncLoop loops[8];

Future<> handle_client(int fd, AsyncLoop& loop)
{
	char buf[1024];

	while (true)
	{
		ssize_t n = read(fd, buf, sizeof(buf));
		if (n > 0)
		{
			// std::cout << "-> " << std::string(buf, n);
			break;
		}
		else if (n == 0)
		{
			close(fd);
			co_return;
		}
		else // n == -1
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				co_await wait_fd(loop, fd, EPOLLIN);
			}
			else
			{
				close(fd);
				co_return;
			}
		}
	}

	const char* response = "HTTP/1.1 200 OK\r\n"
						   "Content-Type: text/plain\r\n"
						   "Content-Length: 12\r\n"
						   "Connection: close\r\n\r\n"
						   "Hello, world";

	std::size_t total = strlen(response);
	std::size_t written = 0;

	while (written < total)
	{
		ssize_t n = write(fd, response + written, total - written);
		if (n > 0)
		{
			written += n;
		}
		else if (n == -1)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				co_await wait_fd(loop, fd, EPOLLOUT);
			}
			else
			{
				break;
			}
		}
	}

	close(fd);
	co_return;
}

Future<> async_main(int listen_fd)
{
	while (true)
	{
		co_await wait_fd(main_loop, listen_fd, EPOLLIN);
		while (true)
		{
			int conn_fd = accept4(listen_fd, nullptr, nullptr,
								  SOCK_NONBLOCK | SOCK_CLOEXEC);
			if (conn_fd == -1)
			{
				if (errno == EWOULDBLOCK || errno == EAGAIN)
				{
					break;
				}
				continue;
			}
			co_await handle_client(
				conn_fd, main_loop); // 大量时必须开 o3 优化，否则尾递归不断压栈
		}
	}
	co_return;
}

int main()
{
	int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	int opt = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = ::htons(8080);
	::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

	int ret =
		::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	::listen(listen_fd, SOMAXCONN);

	LOG_WARN << "Server listening on http://127.0.0.1:8080";

	auto task = async_main(listen_fd);
	task.coro.resume();
	main_loop.run();
	// run_future(main_loop, task);

	return 0;
}