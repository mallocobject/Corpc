#ifndef CORPC_TIMER_LOOP_HPP
#define CORPC_TIMER_LOOP_HPP

#include "corpc/rbtree.hpp"
#include "corpc/task.hpp"
#include <chrono>
#include <coroutine>
#include <optional>
namespace corpc
{
using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;

struct SleepUntilPromise : public RbTree<SleepUntilPromise>::RbNode,
						   public Promise<void>
{
	auto get_return_object()
	{
		return std::coroutine_handle<SleepUntilPromise>::from_promise(*this);
	}

	SleepUntilPromise& operator=(SleepUntilPromise&&) = delete;
	friend bool operator<(const SleepUntilPromise& lhs,
						  const SleepUntilPromise& rhs) noexcept
	{
		return lhs.expire_time_ < rhs.expire_time_;
	}

	TimePoint expire_time_;
};

struct TimerLoop
{
	bool hasTimer() const noexcept
	{
		return !timer_table_.empty();
	}

	void registerTimer(SleepUntilPromise& promise)
	{
		timer_table_.insert(promise);
	}

	std::optional<Clock::duration> run()
	{
		while (!timer_table_.empty())
		{
			auto now_time = Clock::now();
			auto& promise = timer_table_.front();
			if (promise.expire_time_ <= now_time)
			{
				timer_table_.erase(promise);
				std::coroutine_handle<SleepUntilPromise>::from_promise(promise)
					.resume();
			}
			else
			{
				return promise.expire_time_ - now_time; // 返回最近的时间差
			}
		}
		return std::nullopt;
	}

	TimerLoop& operator=(TimerLoop&&) = delete;

	RbTree<SleepUntilPromise> timer_table_{};
};

struct SleepAwaiter
{
	bool await_ready() const noexcept
	{
		return false;
	}

	void await_suspend(std::coroutine_handle<SleepUntilPromise> coro) const
	{
		auto& promise = coro.promise();
		promise.expire_time_ = expire_time_;
		loop_.registerTimer(promise);
	}

	void await_resume() const noexcept
	{
	}

	TimerLoop& loop_;
	TimePoint expire_time_;
};

// inline TimerLoop& loop()
// {
// 	static TimerLoop loop;
// 	return loop;
// }

inline Task<void, SleepUntilPromise> sleep_until(TimerLoop& loop,
												 TimePoint expire_time)
{
	co_await SleepAwaiter(loop, expire_time);
}

inline Task<void, SleepUntilPromise> sleep_for(TimerLoop& loop,
											   Clock::duration duration)
{
	if (duration.count() > 0)
	{
		co_await SleepAwaiter(loop, Clock::now() + duration);
	}
}
} // namespace corpc

#endif