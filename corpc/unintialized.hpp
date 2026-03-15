#ifndef CORPC_UNINITIALIZED_HPP
#define CORPC_UNINITIALIZED_HPP

#include "corpc/non_void_helper.hpp"
#include <functional>
#include <memory>
#include <utility>

namespace corpc
{
template <typename T = void> struct Uninitialized
{
	union
	{
		T val;
	};

	template <typename... Args> void putValue(Args... args)
	{
		new (std::addressof(val)) T(std::forward<Args>(args)...);
	}

	T moveValue() noexcept
	{
		T ret = std::move(val);
		val.~T();
		return ret;
	}

	Uninitialized() noexcept
	{
	}

	Uninitialized(Uninitialized&&) = delete;

	~Uninitialized() noexcept
	{
	}
};

template <> struct Uninitialized<void>
{
	void putValue(NonVoidHelper<>) noexcept
	{
	}

	auto moveValue() noexcept
	{
		return NonVoidHelper<>{};
	}

	Uninitialized() = default;

	Uninitialized(Uninitialized&&) = delete;

	~Uninitialized() = default;
};

template <typename T> struct Uninitialized<T const> : public Uninitialized<T>
{
};

template <typename T>
struct Uninitialized<T&> : public Uninitialized<std::reference_wrapper<T>>
{
};

template <typename T> struct Uninitialized<T&&> : public Uninitialized<T>
{
};
} // namespace corpc

#endif
