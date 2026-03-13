#ifndef CORPC_RETURN_PREVIOUS_TASK_HPP
#define CORPC_RETURN_PREVIOUS_TASK_HPP

#include "corpc/previous_awaiter.hpp"
#include <coroutine>
namespace corpc
{
struct ReturnPreviousPromise
{
	auto initial_suspend() const noexcept
	{
		return std::suspend_always{};
	}

	auto final_suspend() const noexcept
	{
		return PreviousAwaiter{pre_coro_};
	}

	void unhandled_exception()
	{
		throw;
	}

	void return_value(std::coroutine_handle<> pre_coro) noexcept
	{
		pre_coro_ = pre_coro;
	}

	auto get_return_object()
	{
		return std::coroutine_handle<ReturnPreviousPromise>::from_promise(
			*this);
	}

	ReturnPreviousPromise& operator=(ReturnPreviousPromise&&) = delete;

	std::coroutine_handle<> pre_coro_;
};

struct ReturnPreviousTask
{
	using promise_type = ReturnPreviousPromise;

	ReturnPreviousTask(std::coroutine_handle<promise_type> coro) noexcept
		: coro_(coro)
	{
	}

	ReturnPreviousTask(ReturnPreviousPromise&&) = delete;
	~ReturnPreviousTask()
	{
		coro_.destroy();
	}

	std::coroutine_handle<promise_type> coro_;
};
} // namespace corpc

#endif