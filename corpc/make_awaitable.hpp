#ifndef CORPC_MAKE_AWAITER_HPP
#define CORPC_MAKE_AWAITER_HPP

#include "corpc/concepts.hpp"
#include "corpc/task.hpp"
namespace corpc
{
template <Awaitable A> A&& make_awaitable(A&& a)
{
	return std::forward<A>(a);
}

template <class A>
	requires(!Awaitable<A>)
Task<A> make_awaitable(A&& a)
{
	co_return std::forward<A>(a);
}
} // namespace corpc

#endif