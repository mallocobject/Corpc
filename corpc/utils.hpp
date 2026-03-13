#ifndef CORPC_UTILS_HPP
#define CORPC_UTILS_HPP

#include <functional>
#include <memory>
#include <utility>
namespace corpc
{
template <typename T = void> struct NonVoidHelper
{
	using Type = T;
};

template <> struct NonVoidHelper<void>
{
	using Type = NonVoidHelper;

	explicit NonVoidHelper() = default;
};

template <typename T> struct Uninitialized
{
	union
	{
		T value;
	};

	Uninitialized() noexcept
	{
	}

	Uninitialized(Uninitialized&&) = default;

	~Uninitialized() noexcept
	{
	}

	T moveValue()
	{
		T ret{std::move(value)};
		value.~T();
		return ret;
	}

	template <typename... Args> void putValue(Args&&... args)
	{
		new (std::addressof(value)) T(std::forward<Args>(args)...);
	}
};

template <> struct Uninitialized<void>
{
	auto moveValue()
	{
		return NonVoidHelper<>{};
	}

	void pushValue(NonVoidHelper<>)
	{
	}
};

template <typename T> struct Uninitialized<const T> : public Uninitialized<T>
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