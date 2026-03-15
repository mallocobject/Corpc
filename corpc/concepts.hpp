#ifndef CORPC_CONCEPTS_HPP
#define CORPC_CONCEPTS_HPP

#include "corpc/non_void_helper.hpp"
#include <coroutine>
#include <utility>

namespace corpc
{
template <typename A>
concept Awaiter = requires(A a, std::coroutine_handle<> h) {
	{ a.await_ready() };
	{ a.await_suspend(h) };
	{ a.await_resume() };
};

template <typename A>
concept Awaitable = Awaiter<A> || requires(A a) {
	{ a.operator co_await() } -> Awaiter;
};

template <typename A> struct AwaitableTraits;

template <Awaiter A> struct AwaitableTraits<A>
{
	using RetType =
		decltype(std::declval<A>().await_resume()); // co_await 最后会返回
	// await_resume() 的返回值
	using NonVoidRetType =
		NonVoidHelper<RetType>::Type; // RetType 可能是 void, 不可作为类型
};

template <typename A>
	requires(!Awaiter<A> && Awaitable<A>)
struct AwaitableTraits<A>
	: AwaitableTraits<decltype(std::declval<A>().operator co_await())>
{
};
} // namespace corpc

#endif
