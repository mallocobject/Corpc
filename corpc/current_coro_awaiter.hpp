#ifndef CORPC_CURRENT_CORO_AWAITER_HPP
#define CORPC_CURRENT_CORO_AWAITER_HPP

#include <coroutine>
namespace corpc
{
struct CurrentCoroAwaiter
{
	bool await_ready() const noexcept
	{
		return false;
	}

	std::coroutine_handle<> await_suspend(std::coroutine_handle<> coro) noexcept
	{
		coro_ = coro;
		return coro;
	}

	std::coroutine_handle<> await_resume() const noexcept
	{
		return coro_;
	}

	std::coroutine_handle<> coro_;
};
} // namespace corpc

#endif