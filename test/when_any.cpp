#include "corpc/when_any.hpp"
#include "corpc/and_then.hpp"
#include "corpc/async_loop.hpp"
#include "corpc/check_error.hpp"
#include "corpc/epoll_loop.hpp"
#include "corpc/future.hpp"
#include "corpc/promise.hpp"
#include "corpc/timer_loop.hpp"
#include "corpc/when_all.hpp"
#include <chrono>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <variant>

using namespace corpc;
using namespace std::chrono_literals;

AsyncLoop loop;

Future<std::string> reader()
{
	std::string message;
	co_await wait_fd(loop, STDIN_FILENO, EPOLLIN | EPOLLONESHOT);
	while (true)
	{
		char c;
		ssize_t n = read(STDIN_FILENO, &c, sizeof(c));
		if (n == -1)
		{
			if (errno != EWOULDBLOCK) [[unlikely]]
			{
				throw std::system_error(errno, std::system_category());
			}
			break;
		}
		message += c;
	}
	co_return message;
}

Future<> async_main()
{
	while (true)
	{
		auto v = co_await when_any(reader(), sleep_for(loop, 1s));

		if (auto* str = std::get_if<0>(&v))
		{
			if (*str == "q" || *str == "q\n")
			{
				break;
			}
			std::cout << "-> " << *str << std::endl;
		}
		else
		{
			std::cout << "read out time" << std::endl;
		}
	}
}

Future<> hello()
{
	std::this_thread::sleep_for(2s);
	std::cout << "hello" << std::endl;
	co_return;
}

Future<> test()
{
	auto s1 = sleep_for(loop, 1s);
	auto s2 = sleep_for(loop, 2s);
	co_await when_any(s1, s2);
}

int main()
{
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag &= ~ICANON;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);

	auto task = async_main();
	task.coro.resume();
	loop.run();
	return 0;
}
