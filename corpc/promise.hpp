#ifndef CORPC_PROMISE_HPP
#define CORPC_PROMISE_HPP

#include "corpc/rbtree.hpp"
#include "corpc/unintialized.hpp"
#include <cassert>
#include <chrono>
#include <coroutine>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <stop_token>
#include <utility>
#include <variant>

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

template <typename T = void> struct Promise
{
	std::coroutine_handle<> pre_coro;
	std::exception_ptr exception;
	Uninitialized<T> var;
	std::stop_token stop_token;
	std::unique_ptr<std::stop_callback<std::function<void()>>> stop_callback;

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

struct ReturnPrePromise
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

} // namespace corpc

#endif
