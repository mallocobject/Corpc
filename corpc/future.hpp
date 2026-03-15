#ifndef CORPC_FUTURE_HPP
#define CORPC_FUTURE_HPP

#include "corpc/promise.hpp"
#include <type_traits>

namespace corpc
{
template <typename T = void, typename P = Promise<T>> struct Future
{
	using promise_type = P;

	std::coroutine_handle<promise_type> coro;

	struct Awaiter
	{
		std::coroutine_handle<promise_type> child_coro;

		bool await_ready() const noexcept
		{
			return false;
		}

		std::coroutine_handle<promise_type> await_suspend(
			std::coroutine_handle<> parent_coro) noexcept
		{
			child_coro.promise().pre_coro = parent_coro;
			return child_coro;
		}

		T await_resume() const
		{
			if constexpr (std::is_void_v<T>)
			{
				child_coro.promise().result();
				return;
			}
			else
			{
				return child_coro.promise().result();
			}
		}
	};

	Future(std::coroutine_handle<promise_type> coro) : coro(coro)
	{
	}

	Future(Future&&) = delete;

	~Future()
	{
		assert(coro);
		coro.destroy();
	}

	operator std::coroutine_handle<>() const noexcept
	{
		return coro;
	}

	auto operator co_await() const
	{
		return Awaiter{coro};
	}
};

struct ReturnPreFuture
{
	using promise_type = ReturnPrePromise;

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

template <typename Loop, typename T, typename P>
T run_future(Loop& loop, Future<T, P> const& t)
{
	auto a = t.operator co_await();					 // 返回 Awaiter
	a.await_suspend(std::noop_coroutine()).resume(); // 执行完 t 后返回
	while (loop.run())
	{
	}
	return a.await_resume();
}

template <typename T, typename P> void spawn_future(Future<T, P> const& t)
{
	auto a = t.operator co_await();
	a.await_suspend(std::noop_coroutine()).resume();
}
} // namespace corpc

#endif
