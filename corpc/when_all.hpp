#ifndef CORPC_WHEN_ALL_HPP
#define CORPC_WHEN_ALL_HPP

#include "corpc/concepts.hpp"
#include "corpc/future.hpp"
#include "corpc/non_void_helper.hpp"
#include "corpc/unintialized.hpp"
#include <coroutine>
#include <cstddef>
#include <exception>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>

namespace corpc
{
struct WhenAllCtrlBlock
{
	std::size_t count;
	std::coroutine_handle<> coro;
	std::exception_ptr exception;
};

struct WhenAllAwaiter
{
	WhenAllCtrlBlock& ctrl;
	std::span<ReturnPreFuture const> tasks;

	bool await_ready() const noexcept
	{
		return false;
	}

	std::coroutine_handle<> await_suspend(
		std::coroutine_handle<> coro) const noexcept
	{
		if (tasks.empty())
		{
			return coro;
		}
		ctrl.coro = coro;
		for (auto const& t : tasks.subspan(0, tasks.size() - 1))
		{
			t.coro.resume();
		}
		return tasks.back().coro;
	}

	void await_resume() const
	{
		if (ctrl.exception) [[unlikely]]
		{
			std::cout << "throw e" << std::endl;
			std::rethrow_exception(ctrl.exception);
		}
	}
};

template <Awaitable A, typename T>
ReturnPreFuture whenAllHelper(A&& t, WhenAllCtrlBlock& ctrl,
							  Uninitialized<T>& result)
{
	try
	{
		if constexpr (std::is_void_v<T>)
		{
			co_await std::forward<A>(t);
			result.putValue(NonVoidHelper<>{});
		}
		else
		{
			result.putValue(
				co_await std::forward<A>(t)); // t 最后转化为调用 await_resume
											  // 移动返回 promise 的 val
		}
	}
	catch (...)
	{
		ctrl.exception = std::current_exception();
		co_return ctrl.coro; // 返回到 whenAllImpl 里调用 co_await,
		// 但此前会调用 WhenAllAwaiter::await_resume 重新抛出异常
	}
	--ctrl.count;
	if (ctrl.count == 0)
	{
		co_return ctrl.coro;
	}
	co_return std::noop_coroutine(); // 返回到 WhenAllAwaiter::await_suspend
									 // 执行下一个 resume
}

template <std::size_t... Is, typename... Ts>
Future<std::tuple<typename AwaitableTraits<Ts>::NonVoidRetType...>> whenAllImpl(
	std::index_sequence<Is...>, Ts&&... ts)
{
	WhenAllCtrlBlock ctrl{sizeof...(Ts), nullptr, nullptr};
	std::tuple<Uninitialized<typename AwaitableTraits<Ts>::RetType>...> result;
	ReturnPreFuture future_array[]{
		whenAllHelper(ts, ctrl, std::get<Is>(result))...};
	try
	{
		co_await WhenAllAwaiter{ctrl, future_array};
	}
	catch (std::exception const& e)
	{
		std::cout << "Caught in whenAllImpl: " << e.what() << std::endl;
	}
	co_return std::tuple<typename AwaitableTraits<Ts>::NonVoidRetType...>{
		std::get<Is>(result).moveValue()...};
}

template <Awaitable... Ts>
	requires(sizeof...(Ts) != 0)
auto when_all(Ts&&... ts)
{
	return whenAllImpl(std::make_index_sequence<sizeof...(Ts)>{},
					   std::forward<Ts>(ts)...);
}
} // namespace corpc

#endif
