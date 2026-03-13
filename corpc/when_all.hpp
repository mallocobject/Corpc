#ifndef CORPC_WHEN_ALL_HPP
#define CORPC_WHEN_ALL_HPP

#include "corpc/concepts.hpp"
#include "corpc/return_previous_task.hpp"
#include "corpc/task.hpp"
#include "corpc/utils.hpp"
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
namespace corpc
{
struct WhenAllCtlBlock
{
	std::size_t count_{0};
	std::coroutine_handle<> pre_coro_;
	std::exception_ptr exception_;
};

struct WhenAllAwaiter
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

	WhenAllCtlBlock& ctrl_;
	std::span<const ReturnPreviousTask> tasks_;
};

template <typename T>
ReturnPreviousTask whenAllHelper(auto& t, WhenAllCtlBlock& ctrl,
								 Uninitialized<T>& result)
{
	try
	{
		result.putValue(co_await std::forward<decltype(t)>(t));
	}
	catch (...)
	{
		ctrl.exception_ = std::current_exception();
		co_return ctrl.pre_coro_;
	}

	--ctrl.count_;
	if (ctrl.count_ == 0)
	{
		co_return ctrl.pre_coro_;
	}
	co_return std::noop_coroutine();
}

template <typename T>
ReturnPreviousTask whenAllHelper(auto&& t, WhenAllCtlBlock& ctrl,
								 Uninitialized<void>&)
{
	try
	{
		co_await std::forward<decltype(t)>(t);
	}
	catch (...)
	{
		ctrl.exception_ = std::current_exception();
		co_return ctrl.pre_coro_;
	}

	--ctrl.count_;
	if (ctrl.count_ == 0)
	{
		co_return ctrl.pre_coro_;
	}
	co_return std::noop_coroutine();
}

template <std::size_t... Is, typename... Ts>
Task<std::tuple<typename AwaitableTraits<Ts>::NonVoidRetType...>> whenAllImpl(
	std::index_sequence<Is...>, Ts&&... ts)
{
	WhenAllCtlBlock ctrl{sizeof...(Ts)};
	std::tuple<Uninitialized<typename AwaitableTraits<Ts>::RetType>...> result;
	ReturnPreviousTask task_array[]{
		whenAllHelper(ts, ctrl, std::get<Is>(result))...};
	co_await WhenAllAwaiter{ctrl, task_array};
	co_return std::tuple<typename AwaitableTraits<Ts>::NonVoidRetType...>(
		std::get<Is>(result).moveValue()...);
}

template <Awaitable... Ts>
	requires(sizeof...(Ts) != 0)
auto when_all(Ts&&... ts)
{
	return whenAllImpl(std::make_index_sequence<sizeof...(Ts)>{},
					   std::forward<Ts>(ts)...);
}

template <Awaitable T, typename Alloc = std::allocator<T>>
Task<
	std::conditional_t<std::same_as<void, typename AwaitableTraits<T>::RetType>,
					   std::vector<typename AwaitableTraits<T>::RetType>, void>>
when_all(const std::vector<T, Alloc>& tasks)
{
	WhenAllCtlBlock ctrl{tasks.size()};
	Alloc alloc = tasks.get_allocator();
	std::vector<Uninitialized<typename AwaitableTraits<T>::RetType>, Alloc>
		result{tasks.size(), alloc};

	std::vector<ReturnPreviousTask, Alloc> task_array{alloc};
	task_array.reserve(tasks.size());
	for (std::size_t i = 0; i < tasks.size(); i++)
	{
		task_array.push_back(whenAllHelper(tasks[i], ctrl, result[i]));
	}
	co_await WhenAllAwaiter{ctrl, task_array};

	if constexpr (!std::same_as<void, typename AwaitableTraits<T>::RetType>)
	{
		std::vector<typename AwaitableTraits<T>::RetType, Alloc> res{alloc};
		res.reserve(tasks.size());
		for (auto& r : result)
		{
			res.push_back(r.moveValue());
		}
		co_return res;
	}
}
} // namespace corpc

#endif