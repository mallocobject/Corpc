#ifndef CORPC_CURRENT_STOP_TOKEN_HPP
#define CORPC_CURRENT_STOP_TOKEN_HPP

#include <coroutine>
#include <stop_token>
namespace corpc
{
struct CurrentStopTokenAwaiter
{
	std::stop_token token;

	bool await_ready() const noexcept
	{
		return false;
	}

	template <typename PromiseType>
	bool await_suspend(std::coroutine_handle<PromiseType> h) noexcept
	{
		if constexpr (requires { h.promise().stop_token; })
		{
			token = h.promise().stop_token;
		}
		return false;
	}

	std::stop_token await_resume() const noexcept
	{
		return token;
	}
};

inline CurrentStopTokenAwaiter current_stop_token() noexcept
{
	return {};
}
} // namespace corpc

#endif