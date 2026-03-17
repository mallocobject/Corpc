#ifndef CORPC_PROMISE_HPP
#define CORPC_PROMISE_HPP

#include "corpc/pre_awaiter.hpp"
#include "corpc/unintialized.hpp"
#include <cassert>
#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <stop_token>
#include <utility>

namespace corpc
{
template <typename T = void> struct Promise
{
	std::coroutine_handle<> pre_coro;
	std::exception_ptr exception;
	Uninitialized<T> var;
	std::stop_token stop_token;
	std::unique_ptr<std::stop_callback<std::function<void()>>> stop_callback;

	template <typename A> A&& await_transform(A&& awaitable) noexcept
	{
		if constexpr (requires { awaitable.coro.promise().stop_token; })
		{
			if (awaitable.coro && this->stop_token.stop_possible())
			{
				awaitable.coro.promise().stop_token = this->stop_token;
			}
		}
		return std::forward<A>(awaitable);
	}

	std::suspend_always initial_suspend() const noexcept
	{
		return {};
	}

	auto final_suspend() const noexcept
	{
		return PreAwaiter{pre_coro};
	}

	void unhandled_exception() noexcept
	{
		exception = std::current_exception();
	}

	void return_value(T const& val)
	{
		var.putValue(val);
	}

	void return_value(T&& val)
	{
		var.putValue(std::move(val));
	}

	T result()
	{
		if (exception) [[unlikely]]
		{
			std::rethrow_exception(exception);
		}
		return var.moveValue();
	}

	std::coroutine_handle<Promise> get_return_object()
	{
		return std::coroutine_handle<Promise>::from_promise(*this);
	}

	Promise() noexcept
	{
	}

	Promise(Promise&&) = delete;

	~Promise() noexcept
	{
	}
};

template <> struct Promise<void>
{
	std::coroutine_handle<> pre_coro;
	std::exception_ptr exception;
	std::stop_token stop_token;
	std::unique_ptr<std::stop_callback<std::function<void()>>> stop_callback;

	template <typename A> A&& await_transform(A&& awaitable) noexcept
	{
		if constexpr (requires { awaitable.coro.promise().stop_token; })
		{
			if (awaitable.coro && this->stop_token.stop_possible())
			{
				awaitable.coro.promise().stop_token = this->stop_token;
			}
		}
		return std::forward<A>(awaitable);
	}

	std::suspend_always initial_suspend() const noexcept
	{
		return {};
	}

	auto final_suspend() const noexcept
	{
		return PreAwaiter{pre_coro};
	}

	void unhandled_exception() noexcept
	{
		exception = std::current_exception();
	}

	void return_void() noexcept
	{
	}

	void result()
	{
		if (exception) [[unlikely]]
		{
			std::rethrow_exception(exception);
		}
	}

	std::coroutine_handle<Promise> get_return_object()
	{
		return std::coroutine_handle<Promise>::from_promise(*this);
	}

	Promise() = default;

	Promise(Promise&&) = delete;

	~Promise() = default;
};

} // namespace corpc

#endif
