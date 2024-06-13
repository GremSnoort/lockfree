#pragma once

// sdk
#include <gremsnoort/sdk/forward/inplaced.hpp>

// lockfree
#include <lockfree/queue/lockfree_queue_v2.hpp>

namespace gremsnoort::lockfree {

	template<class T>
	class queue_traits {
	private:
		using type_t = T;
		using node_t = internal::node_t<type_t>;
		using inplaced_t = sdk::inplaced_t<node_t>;

	public:
		using queue_t = queue_t<type_t, inplaced_t>;
	};

}
