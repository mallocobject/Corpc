#ifndef CORPC_EPOLL_LOOP_HPP
#define CORPC_EPOLL_LOOP_HPP

#include "corpc/check_error.hpp"
#include "corpc/future.hpp"
#include "corpc/promise.hpp"
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <sys/epoll.h>

namespace corpc
{
using EpollEventMask = std::uint32_t;

struct EpollFdAwaiter;

struct EpollFdPromise : public Promise<EpollEventMask>
{
	EpollFdAwaiter* awaiter{nullptr}; // 短命

	auto get_return_object()
	{
		return std::coroutine_handle<EpollFdPromise>::from_promise(*this);
	}

	EpollFdPromise& operator=(EpollFdPromise&&) = delete;

	~EpollFdPromise();
};

struct EpollLoop
{
	static constexpr std::size_t kEventSize = 1024;

	int epfd = checkError(epoll_create1(0));
	std::size_t count{0}; // 注册 fd 数量
	struct epoll_event evs[kEventSize];

	// std::vector<std::coroutine_handle<>> coro_queue;

	bool empty() const noexcept
	{
		return count == 0;
	}

	bool registerEvent(EpollFdPromise& promise, int ctl_op);

	void unregisterEvent(int fd)
	{
		checkError(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr));
		--count;
	}

	bool run(std::optional<std::chrono::system_clock::duration> timeout =
				 std::nullopt);

	EpollLoop& operator=(EpollLoop&&) = delete;

	~EpollLoop()
	{
		close(epfd);
	}
};

struct EpollFdAwaiter
{
	EpollLoop& loop;
	int fd{-1};
	EpollEventMask events{0};
	EpollEventMask revents{0};
	int ctl_op{EPOLL_CTL_ADD};

	bool await_ready() const noexcept
	{
		return false;
	}

	void await_suspend(std::coroutine_handle<EpollFdPromise> coro)
	{
		auto& promise = coro.promise();
		promise.awaiter = this;

		if (promise.stop_token.stop_possible())
		{
			promise.stop_callback =
				std::make_unique<std::stop_callback<std::function<void()>>>(
					promise.stop_token, [&loop = loop, fd = fd, coro]
					{ loop.unregisterEvent(fd); });
		}

		if (!loop.registerEvent(promise, ctl_op))
		{
			promise.awaiter = nullptr;
			coro.resume();
		}
	}

	EpollEventMask await_resume() const noexcept
	{
		return revents;
	}
};

inline EpollFdPromise::~EpollFdPromise()
{
	if (awaiter)
	{
		awaiter->loop.unregisterEvent(awaiter->fd);
	}
}

inline bool EpollLoop::registerEvent(EpollFdPromise& promise, int ctl_op)
{
	struct epoll_event ev;
	ev.events = promise.awaiter->events;
	ev.data.ptr = &promise;
	int nevs = epoll_ctl(epfd, ctl_op, promise.awaiter->fd, &ev);
	if (nevs == -1)
	{
		return false;
	}

	if (ctl_op == EPOLL_CTL_ADD)
	{
		++count;
	}
	return true;
}

inline bool EpollLoop::run(
	std::optional<std::chrono::system_clock::duration> timeout)
{
	// for (auto const &coro: coro_queue) {
	//     coro.resume();
	// }
	// coro_queue.clear();

	if (count == 0)
	{
		return false;
	}

	int timeout_ms = -1;
	if (timeout.has_value())
	{
		timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
						 timeout.value())
						 .count();
	}

	int ret = checkError(epoll_wait(epfd, evs, kEventSize, timeout_ms));
	for (int i = 0; i < ret; i++)
	{
		auto& ev = evs[i];
		auto& promise = *reinterpret_cast<EpollFdPromise*>(ev.data.ptr);
		std::coroutine_handle<EpollFdPromise>::from_promise(promise).resume();
	}
	return true;
}

inline Future<void, EpollFdPromise> wait_fd(EpollLoop& loop, int fd,
											uint32_t events)
{
	co_return co_await EpollFdAwaiter(loop, fd, events | EPOLLONESHOT);
}

} // namespace corpc

#endif
