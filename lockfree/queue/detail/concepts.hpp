// std
#include <concepts>

namespace gremsnoort::lockfree::detail {

    template<typename T>
    concept __allocateable = requires(const std::size_t sz, const std::size_t align) {
        { typename T::allocate(sz, align) } -> std::same_as<void*>;
    };

    template<typename T>
    concept __deallocateable = requires(void* arg) {
        { typename T::deallocate(arg) } -> std::same_as<void>;
    };

    template<typename T>
    concept __initializeable = requires() {
        { typename T::initialize() } -> std::same_as<void>;
    };

    template<typename T>
    concept Allocator = __allocateable<T> && __deallocateable<T> && __initializeable<T>;

}
