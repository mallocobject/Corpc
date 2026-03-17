#ifndef CORPC_WHEN_ALL_HPP
#define CORPC_WHEN_ALL_HPP

#include "corpc/concepts.hpp"
#include "corpc/current_stop_token.hpp"
#include "corpc/future.hpp"
#include "corpc/return_pre_future.hpp"
#include "corpc/unintialized.hpp"
#include <coroutine>
#include <cstddef>
#include <exception>
#include <iostream>
#include <optional>
#include <span>
#include <stop_token>
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
	std::stop_source stop_src;
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
			co_await set_stop_token(std::forward<A>(t),
									ctrl.stop_src.get_token());
			result.putValue(NonVoidHelper<>{});
		}
		else
		{
			result.putValue(co_await set_stop_token(std::forward<A>(t),
													ctrl.stop_src.get_token()));
		}
	}
	catch (...)
	{
		ctrl.exception = std::current_exception();
		ctrl.stop_src.request_stop();
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
	std::stop_token parent_token = co_await current_stop_token();
	WhenAllCtrlBlock ctrl{sizeof...(Ts), nullptr, nullptr};
	std::optional<std::stop_callback<std::function<void()>>> cb;
	if (parent_token.stop_possible())
	{
		cb.emplace(parent_token, [&ctrl] { ctrl.stop_src.request_stop(); });
	}

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
