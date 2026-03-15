#ifndef CORPC_WHEN_ANY_HPP
#define CORPC_WHEN_ANY_HPP

#include "corpc/concepts.hpp"
#include "corpc/future.hpp"
#include "corpc/non_void_helper.hpp"
#include "corpc/unintialized.hpp"
#include <coroutine>
#include <cstddef>
#include <exception>
#include <stop_token>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace corpc
{
struct WhenAnyCtrlBlock
{
	std::size_t index{static_cast<std::size_t>(-1)};
	std::coroutine_handle<> coro;
	std::exception_ptr exception;
};

struct WhenAnyAwaiter
{
	WhenAnyCtrlBlock& ctrl;
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

	void await_resume() const noexcept
	{
	}
};

template <Awaitable A, typename T>
ReturnPreFuture whenAnyHelper(A const& t, WhenAnyCtrlBlock& ctrl,
							  Uninitialized<T>& result, std::size_t index)
{
	try
	{
		if constexpr (std::is_void_v<T>)
		{
			co_await t;
			result.putValue(NonVoidHelper<>{});
		}
		else
		{
			result.putValue(co_await t);
		}
	}
	catch (...)
	{
		ctrl.exception = std::current_exception();
		co_return ctrl.coro;
	}
	ctrl.index = index;
	co_return ctrl.coro;
}

template <std::size_t... Is, typename... Ts>
Future<std::variant<typename AwaitableTraits<Ts>::NonVoidRetType...>>
whenAnyImpl(std::index_sequence<Is...>, Ts&&... ts)
{
	WhenAnyCtrlBlock ctrl{sizeof...(Ts), nullptr, nullptr};
	std::tuple<Uninitialized<typename AwaitableTraits<Ts>::RetType>...> result;
	ReturnPreFuture future_array[]{
		whenAnyHelper(ts, ctrl, std::get<Is>(result), Is)...};
	co_await WhenAnyAwaiter{ctrl, future_array};
	Uninitialized<std::variant<typename AwaitableTraits<Ts>::NonVoidRetType...>>
		var_result;
	((ctrl.index == Is &&
	  (var_result.putValue(
		   std::in_place_index<
			   Is>, // 精确指定 variant
					// 应激活的选项索引，并利用该索引完成就地构造
		   std::get<Is>(result).moveValue()),
	   0)),
	 ...);
	co_return var_result.moveValue();
}

template <Awaitable... Ts>
	requires(sizeof...(Ts) != 0)
auto when_any(Ts&&... ts)
{
	return whenAnyImpl(std::make_index_sequence<sizeof...(Ts)>{},
					   std::forward<Ts>(ts)...);
}
} // namespace corpc

#endif
