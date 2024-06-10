// std
#include <cstdlib>

namespace gremsnoort::lockfree::allocator {

	struct default_t final {

		static auto allocate(const std::size_t sz, const std::size_t align) -> void* {
#if defined(_MSC_VER)
			// https://learn.microsoft.com/ru-ru/cpp/c-runtime-library/reference/aligned-malloc?view=msvc-170
			return _aligned_malloc(sz, align);
#else
			// https://en.cppreference.com/w/c/memory/aligned_alloc
			return std::aligned_alloc(align, sz);
#endif
		}

		static auto deallocate(void* arg) -> void {
#if defined(_MSC_VER)
			_aligned_free(arg);
#else
			std::free(arg);
#endif
		}

		static auto initialize() -> void {}
	};

}
