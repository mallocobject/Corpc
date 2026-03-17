#ifndef CORPC_TIMER_LOOP_HPP
#define CORPC_TIMER_LOOP_HPP

#include "corpc/future.hpp"
#include "corpc/promise.hpp"
#include "corpc/rbtree.hpp"
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <stop_token>

namespace corpc
{
using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

struct SleepUntilPromise : public RbTree<SleepUntilPromise>::RbNode,
						   public Promise<>
{
	TimePoint expire;

	auto get_return_object()
	{
		return std::coroutine_handle<SleepUntilPromise>::from_promise(*this);
	}

	SleepUntilPromise& operator=(SleepUntilPromise&&) = delete;

	friend bool operator<(SleepUntilPromise const& lhs,
						  SleepUntilPromise const& rhs) noexcept
	{
		return lhs.expire < rhs.expire;
	}
};

struct TimerLoop
{
	RbTree<SleepUntilPromise> timer_table;

	void registerTimer(SleepUntilPromise& promise)
	{
		timer_table.insert(promise);
	}

	void unregisterTimer(SleepUntilPromise& promise)
	{
		timer_table.erase(promise);
	}

	std::optional<Duration> run()
	{
		while (!timer_table.empty())
		{
			auto now = Clock::now();
			auto& promise = timer_table.front();
			if (promise.expire < now)
			{
				timer_table.erase(promise);
				std::coroutine_handle<SleepUntilPromise>::from_promise(promise)
					.resume();
			}
			else
			{
				return promise.expire - now;
			}
		}
		return std::nullopt;
	}
};

struct SleepAwaiter
{
	TimerLoop& loop;
	TimePoint expire;

	bool await_ready() const noexcept
	{
		return expire <= Clock::now();
	}

	void await_suspend(
		std::coroutine_handle<SleepUntilPromise> coro) const noexcept
	{
		auto& promise = coro.promise();
		promise.expire = expire;

		if (promise.stop_token.stop_possible())
		{
			promise.stop_callback =
				std::make_unique<std::stop_callback<std::function<void()>>>(
					promise.stop_token,
					[&loop = loop, coro]
					{
						if (coro.promise().in_tree())
						{
							// std::cout << "[TIMER CANCEL] unregisterTimer,
							// coro="
							// 		  << coro.address() << std::endl;
							loop.unregisterTimer(coro.promise());
							// coro.resume();
						}
					});
		}

		loop.registerTimer(promise);
	}

	void await_resume() const noexcept
	{
	}
};

inline Future<void, SleepUntilPromise> sleep_until(TimerLoop& loop,
												   TimePoint expire)
{
	co_await SleepAwaiter{loop, expire};
}

inline Future<void, SleepUntilPromise> sleep_for(TimerLoop& loop,
												 Duration duration)
{
	co_await SleepAwaiter{loop, Clock::now() + duration};
}
} // namespace corpc

#endif
