#ifndef CORPC_CONCEPTS_HPP
#define CORPC_CONCEPTS_HPP

#include "corpc/utils.hpp"
#include <coroutine>
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
	using RetType = decltype(std::declval<A>().await_resume());
	using NonVoidRetType = NonVoidHelper<RetType>::Type;
};

template <typename A>
	requires(!Awaiter<A> && Awaitable<A>)
struct AwaitableTraits<A>
	: public AwaitableTraits<decltype(std::declval<A>().operator co_await())>
{
};
} // namespace corpc

#endif