// conan
#include <mimalloc.h>

namespace gremsnoort::lockfree::allocator {

	struct mimalloc_t final {

		static auto allocate(const std::size_t sz, const std::size_t align) -> void* {
			return mi_malloc_aligned(sz, align);
		}

		static auto deallocate(void* arg, const std::size_t align) -> void {
			mi_free_aligned(arg, align);
		}

		static auto initialize() -> void {}
	};

}
