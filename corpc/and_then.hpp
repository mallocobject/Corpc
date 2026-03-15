#ifndef CORPC_AND_THEN_HPP
#define CORPC_AND_THEN_HPP

#include "corpc/concepts.hpp"
#include "corpc/future.hpp"
#include "corpc/make_awaitable.hpp"
#include <concepts>
#include <type_traits>
#include <utility>

namespace corpc
{
template <Awaitable A, std::invocable<typename AwaitableTraits<A>::RetType> F>
	requires(!std::same_as<void, typename AwaitableTraits<A>::RetType>)
Future<typename AwaitableTraits<
	std::invoke_result_t<F, typename AwaitableTraits<A>::RetType>>::Type>
and_then(A&& a, F&& f)
{
	co_return co_await make_awaitable(
		std::forward<F>(f)(co_await std::forward<A>(a)));
}

template <Awaitable A, std::invocable<> F>
	requires(std::same_as<void, typename AwaitableTraits<A>::RetType>)
Future<typename AwaitableTraits<
	std::invoke_result_t<F, typename AwaitableTraits<A>::RetType>>::Type>
and_then(A&& a, F&& f)
{
	co_await std::forward<A>(a);
	co_return co_await make_awaitable(std::forward<F>(f)());
}

template <Awaitable A, Awaitable F>
	requires(!std::invocable<F> &&
			 !std::invocable<F, typename AwaitableTraits<A>::RetType>)
Future<typename AwaitableTraits<F>::RetType> and_then(A&& a, F&& f)
{
	co_await std::forward<A>(a);
	co_return co_await std::forward<F>(f);
}
} // namespace corpc

#endif
