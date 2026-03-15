#ifndef CORPC_NON_VOID_HELPER_HPP
#define CORPC_NON_VOID_HELPER_HPP

namespace corpc {
template <typename T = void>
struct NonVoidHelper {
    using Type = T;
};

template <>
struct NonVoidHelper<void> {
    using Type = NonVoidHelper;

    explicit NonVoidHelper() = default;
};
} // namespace corpc

#endif
