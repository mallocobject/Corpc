#ifndef CORPC_PREVIOUS_AWAITER_HPP
#define CORPC_PREVIOUS_AWAITER_HPP

#include <coroutine>
namespace corpc
{
struct PreviousAwaiter
{
	bool await_ready() const noexcept
	{
		return false;
	}

	std::coroutine_handle<> await_suspend(
		std::coroutine_handle<> coroutine) const noexcept
	{
		if (pre_coro_)
		{
			return pre_coro_;
		}
		return std::noop_coroutine();
	}

	void await_resume() const noexcept
	{
	}

	std::coroutine_handle<> pre_coro_;
};
} // namespace corpc

#endif