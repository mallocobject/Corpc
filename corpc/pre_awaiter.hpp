#ifndef CORPC_PRE_AWAITER_HPP
#define CORPC_PRE_AWAITER_HPP

#include <coroutine>
namespace corpc
{
struct PreAwaiter
{
	std::coroutine_handle<> coro;

	bool await_ready() const noexcept
	{
		return false;
	}

	std::coroutine_handle<> await_suspend(
		std::coroutine_handle<>) const noexcept
	{
		if (coro)
		{
			return coro;
		}
		return std::noop_coroutine();
	}

	void await_resume() const noexcept
	{
	}
};
} // namespace corpc

#endif