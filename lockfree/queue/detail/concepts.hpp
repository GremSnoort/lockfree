// std
#include <concepts>

namespace gremsnoort::lockfree::detail {

    template<typename T>
    concept __allocateable = requires(const std::size_t sz, const std::size_t align) {
        { typename T::allocate(sz, align) } -> std::same_as<void*>;
    };

    template<typename T>
    concept __deallocateable = requires(T& a, void* arg, const std::size_t align) {
        { std::decay_t<decltype(a)>::deallocate(arg, align) } -> std::same_as<void>;
    };

    template<typename T>
    concept __initializeable = requires(T& a) {
        { std::decay_t<decltype(a)>::initialize() } -> std::same_as<void>;
    };

    template<typename T>
    concept Allocator = __allocateable<T> && __deallocateable<T> && __initializeable<T>;

}
