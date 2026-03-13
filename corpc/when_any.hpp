#ifndef CORPC_WHEN_ANY_HPP
#define CORPC_WHEN_ANY_HPP

#include "corpc/concepts.hpp"
#include "corpc/return_previous_task.hpp"
#include "corpc/task.hpp"
#include "corpc/utils.hpp"
#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <span>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
namespace corpc
{
struct WhenAnyCtlBlock
{
	static constexpr std::size_t kNullIndex = std::size_t(-1);
	std::size_t index_{kNullIndex};
	std::coroutine_handle<> pre_coro_;
	std::exception_ptr exception_;
};

struct WhenAnyAwaiter
{
	bool await_ready() const noexcept
	{
		return false;
	}

	std::coroutine_handle<> await_suspend(std::coroutine_handle<> coro) const
	{
		if (tasks_.empty())
		{
			return coro;
		}
		ctrl_.pre_coro_ = coro;
		for (const auto& t : tasks_.subspan(0, tasks_.size() - 1))
		{
			t.coro_.resume();
		}
		return tasks_.back().coro_;
	}

	void await_resume() const
	{
		if (ctrl_.exception_) [[unlikely]]
		{
			std::rethrow_exception(ctrl_.exception_);
		}
	}

	WhenAnyCtlBlock& ctrl_;
	std::span<const ReturnPreviousTask> tasks_;
};

template <typename T>
ReturnPreviousTask whenAnyHelper(auto&& t, WhenAnyCtlBlock& ctrl,
								 Uninitialized<T>& result, std::size_t index)
{
	try
	{
		if constexpr (std::is_void_v<T>)
		{
			co_await std::forward<decltype(t)>(t);
		}
		else
		{
			result.putValue(co_await std::forward<decltype(t)>(t));
		}
	}
	catch (...)
	{
		ctrl.exception_ = std::current_exception();
		co_return ctrl.pre_coro_;
	}

	ctrl.index_ = index;
	co_return ctrl.pre_coro_;
}

template <std::size_t... Is, typename... Ts>
Task<std::variant<typename AwaitableTraits<Ts>::NonVoidRetType...>> whenAnyImpl(
	std::index_sequence<Is...>, Ts&&... ts)
{
	WhenAnyCtlBlock ctrl{};
	std::tuple<Uninitialized<typename AwaitableTraits<Ts>::RetType>...> result;
	ReturnPreviousTask task_array[]{
		whenAnyHelper(ts, ctrl, std::get<Is>(result), Is)...};
	co_await WhenAnyAwaiter{ctrl, task_array};
	Uninitialized<std::variant<typename AwaitableTraits<Ts>::NonVoidRetType...>>
		var_result;
	((ctrl.index_ == Is &&
	  (var_result.putValue(std::in_place_index<Is>,
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

template <Awaitable T, typename Alloc = std::allocator<T>>
Task<typename AwaitableTraits<T>::RetType> when_any(
	const std::vector<T, Alloc>& tasks)
{
	WhenAnyCtlBlock ctrl{tasks.size()};
	Alloc alloc = tasks.get_allocator();
	Uninitialized<typename AwaitableTraits<T>::RetType> result;
	{
		std::vector<ReturnPreviousTask, Alloc> task_array{alloc};
		task_array.reserve(tasks.size());
		for (auto& task : tasks)
		{
			task_array.push_back(whenAllHelper(task, ctrl, result));
		}
		co_await WhenAnyAwaiter(ctrl, task_array);
	}
	co_return result.moveValue();
}
} // namespace corpc

#endif