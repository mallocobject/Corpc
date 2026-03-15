#ifndef CORPC_MAKE_AWAITABLE_HPP
#define CORPC_MAKE_AWAITABLE_HPP

#include "corpc/concepts.hpp"
#include "corpc/future.hpp"
#include <utility>

namespace corpc
{
template <Awaitable A> A&& make_awaitable(A&& a)
{
	return std::forward<A>(a);
}

template <typename A>
	requires(!Awaitable<A>)
Future<A> make_awaitable(A&& a)
{
	co_return std::forward<A>(a);
}
} // namespace corpc

#endif
