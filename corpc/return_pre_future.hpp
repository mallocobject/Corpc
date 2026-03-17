#ifndef CORPC_RETURN_PRE_FUTURE_HPP
#define CORPC_RETURN_PRE_FUTURE_HPP

#include "corpc/pre_awaiter.hpp"
#include <cassert>
#include <coroutine>
namespace corpc
{
struct ReturnPreFuture
{
	// using promise_type = ReturnPrePromise;

	using promise_type = struct ReturnPrePromise
	{
		std::coroutine_handle<> pre_coro;

		std::suspend_always initial_suspend() const noexcept
		{
			return {};
		}

		auto final_suspend() const noexcept
		{
			return PreAwaiter{pre_coro};
		}

		void unhandled_exception()
		{
			throw;
		}

		void return_value(std::coroutine_handle<> coro) noexcept
		{
			pre_coro = coro;
		}

		std::coroutine_handle<ReturnPrePromise> get_return_object()
		{
			return std::coroutine_handle<ReturnPrePromise>::from_promise(*this);
		}

		ReturnPrePromise() = default;

		ReturnPrePromise(ReturnPrePromise&&) = delete;

		~ReturnPrePromise() = default;
	};

	std::coroutine_handle<promise_type> coro;

	ReturnPreFuture(std::coroutine_handle<promise_type> coro) : coro(coro)
	{
	}

	ReturnPreFuture(ReturnPreFuture&&) = delete;

	~ReturnPreFuture()
	{
		assert(coro);
		coro.destroy();
	}
};
} // namespace corpc

#endif