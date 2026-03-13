#ifndef CORPC_TASK_HPP
#define CORPC_TASK_HPP

#include "corpc/previous_awaiter.hpp"
#include "corpc/utils.hpp"
#include <coroutine>
#include <exception>
#include <utility>
namespace corpc
{
template <typename T> struct Promise
{
	auto initial_suspend() const noexcept
	{
		return std::suspend_always{};
	}

	auto final_suspend() noexcept
	{
		return PreviousAwaiter{pre_coro_};
	}

	void unhandled_exception() noexcept
	{
		exception_ = std::current_exception();
	}

	void return_value(T&& ret)
	{
		result_.putValue(std::move(ret));
	}

	void return_value(const T& ret)
	{
		result_.putValue(ret);
	}

	T result()
	{
		if (exception_) [[unlikely]]
		{
			std::rethrow_exception(exception_);
		}
		return result_.moveValue();
	}

	auto get_return_object()
	{
		return std::coroutine_handle<Promise>::from_promise(*this);
	}

	std::coroutine_handle<> pre_coro_;
	std::exception_ptr exception_;
	Uninitialized<T> result_;

	Promise& operator=(Promise&&) = delete;
};

template <> struct Promise<void>
{
	auto initial_suspend() const noexcept
	{
		return std::suspend_always{};
	}

	auto final_suspend() noexcept
	{
		return PreviousAwaiter{pre_coro_};
	}

	void unhandled_exception() noexcept
	{
		exception_ = std::current_exception();
	}

	void return_void() const noexcept
	{
	}

	void result()
	{
		if (exception_) [[unlikely]]
		{
			std::rethrow_exception(exception_);
		}
	}

	auto get_return_object()
	{
		return std::coroutine_handle<Promise>::from_promise(*this);
	}

	std::coroutine_handle<> pre_coro_;
	std::exception_ptr exception_;

	Promise& operator=(Promise&&) = delete;
};

template <typename T = void, typename P = Promise<T>> struct Task
{
	using promise_type = P;

	Task(std::coroutine_handle<promise_type> coro = nullptr) noexcept
		: coro_(coro)
	{
	}

	Task(Task&& other) noexcept : coro_(other.coro_)
	{
		other.coro_ = nullptr;
	}

	Task& operator=(Task&& other) noexcept
	{
		std::swap(coro_, other.coro_);
	}

	~Task()
	{
		if (coro_)
		{
			coro_.destroy();
		}
	}

	struct Awaiter
	{
		bool await_ready() const noexcept
		{
			return false;
		}

		std::coroutine_handle<promise_type> await_suspend(
			std::coroutine_handle<> coro) const noexcept
		{
			coro_.promise().pre_coro_ = coro;
			return coro_;
		}

		T await_resume() const
		{
			return coro_.promise().result();
		}

		std::coroutine_handle<promise_type> coro_;
	};

	auto operator co_await() const noexcept
	{
		return Awaiter(coro_);
	}

	operator std::coroutine_handle<>() const noexcept
	{
		return coro_;
	}

	std::coroutine_handle<promise_type> coro_;
};

template <typename Loop, typename T, typename P>
T run_task(Loop& loop, const Task<T, P>& t)
{
	auto a = t.operator co_await();
	a.await_suspend(std::noop_coroutine()).resume();
	while (loop.run())
	{
	};

	return a.await_resume();
}

template <typename T, typename P> void spawn_task(const Task<T, P>& t)
{
	auto a = t.operator co_await();
	a.await_suspend(std::noop_coroutine()).resume();
}
} // namespace corpc

#endif