#pragma once

// std
#include <atomic>
#include <array>
#include <cassert>

// detail
#include <lockfree/queue/detail/concepts.hpp>

//#include <memory_resource>

//#define JEMALLOC_NO_RENAME
//#include <jemalloc/jemalloc.h>
//#include <mimalloc.h>

namespace gremsnoort::lockfree {

	template<class T, detail::Allocator alloc>
	class queue_t {

//#ifdef USE_PMR
//		std::array<std::byte, 1024UL * 1024UL * 1024UL * 4UL> buffer;
//		std::pmr::monotonic_buffer_resource mbr{ buffer.data(), buffer.size() };
//#endif

		using type_t = uint64_t;

		struct node_t {
			T payload;
			std::atomic<type_t> next = 0;

			template<class ...Args,
				std::enable_if_t<std::is_constructible_v<T, Args&&...>, bool> = true>
			explicit node_t(Args&&... args)
				: payload(std::forward<Args&&>(args)...)
				, next(0)
			{}
		};

		std::atomic<type_t> push_end = 0;
		std::atomic<type_t> pop_end = 0;
		std::atomic<type_t> observed_last = 0;

	public:
		using value_type = T;

		queue_t() = default;
		explicit queue_t(std::size_t) {}

		template<class ...Args,
			std::enable_if_t<std::is_constructible_v<T, Args&&...>, bool> = true>
		auto push(Args&&... args) {

			auto node = alloc::allocate(sizeof(node_t), alignof(node_t));

//#ifdef USE_PMR
//			auto node = mbr.allocate(sizeof(node_t), alignof(node_t));// new node_t(std::forward<Args&&>(args)...);
//#else
//			auto node = mi_malloc(sizeof(node_t));
//#endif
			node = new (node) node_t(std::forward<Args&&>(args)...);

			const auto desired = reinterpret_cast<type_t>(node);

			type_t expected = 0;
			if (push_end.compare_exchange_weak(expected/*0*/, desired)) {
				// push end is zero
				pop_end.store(desired);
				return true;
			}
			while (true) {				
				// push end is non-zero
				const auto parent = expected;
				auto expnode = reinterpret_cast<node_t*>(expected);
				expected = 0;
				if (expnode && expnode->next.compare_exchange_weak(expected/*0*/, desired)) {
					// its ours )))
					push_end.store(desired, std::memory_order_relaxed);
					return true;
				}
				// expected = next not null
				expected = push_end.load(std::memory_order_relaxed);
				//else {
				//	auto next = expected;
				//	expected = parent;
				//	push_end.compare_exchange_weak(expected, next);
				//}
			}
			return false;
		}

		auto pop(T& output) {

			static const type_t VAL = UINT64_MAX;
			
			type_t expected = pop_end.load(std::memory_order_relaxed);
			while (true) {
				
				if (expected == 0) {
					if (observed_last.compare_exchange_weak(expected/*0*/, 0, std::memory_order_relaxed)) { // memory_order_acquire
						// 0->0
						return false;
					}
					else {
						// non-0
						assert(expected > 0);
						auto expnode = reinterpret_cast<node_t*>(expected);
						if (auto next = expnode->next.load(); next > 0) {
							if (observed_last.compare_exchange_weak(expected, 0, std::memory_order_relaxed)) { // set to 0 // memory_order_acquire
								expnode->~node_t();
								alloc::deallocate(reinterpret_cast<void*>(expnode), alignof(node_t));
//#ifdef USE_PMR
//								mbr.deallocate(reinterpret_cast<void*>(expnode), sizeof(node_t), alignof(node_t));
//#else
//								mi_free(reinterpret_cast<void*>(expnode));
//#endif
								//delete expnode;
								pop_end.store(next, std::memory_order_relaxed); // memory_order_release
							}
						}
						///if (observed_last.compare_exchange_weak(expected, 0, std::memory_order_relaxed)) { // set to 0 // memory_order_acquire
						///	auto expnode = reinterpret_cast<node_t*>(expected);
						///	auto next = expnode->next.load();
						///	if (next == 0) {
						///		observed_last.store(expected, std::memory_order_relaxed); // memory_order_release
						///	}
						///	else {
						///		delete expnode;
						///		pop_end.store(next, std::memory_order_relaxed); // memory_order_release
						///	}
						///}
					}
					return false;
				}
				if (expected > 0) {
					auto expnode = reinterpret_cast<node_t*>(expected);
					auto next = expnode->next.load();
					if (pop_end.compare_exchange_weak(expected, next, std::memory_order_relaxed)) { // take non-zero OR pass -1 // memory_order_acquire
						output = std::move(expnode->payload);
						if (next == 0) {
							observed_last.store(expected, std::memory_order_relaxed); // memory_order_release
						}
						else {
							expnode->~node_t();
							alloc::deallocate(reinterpret_cast<void*>(expnode), alignof(node_t));
//#ifdef USE_PMR
//							mbr.deallocate(reinterpret_cast<void*>(expnode), sizeof(node_t), alignof(node_t));
//#else
//							mi_free(reinterpret_cast<void*>(expnode));
//#endif
							//delete expnode;
						}
						return true;
					}
				}
			}
			return false;
		}

	};

}

