#pragma once

// std
#include <atomic>
#include <array>
#include <cassert>

#include <iostream>

// detail
#include <lockfree/queue/detail/concepts.hpp>

// sdk
#include <sdk/forward/inplaced.hpp>

namespace gremsnoort::lockfree {

	template<class T, detail::Allocator alloc>
	class queue_t {

		using type_t = uint64_t;

		struct node_t {
			T payload;
			std::atomic<type_t> next = 0;

			node_t() = default;

			template<class ...Args,
				std::enable_if_t<std::is_constructible_v<T, Args&&...>, bool> = true>
			explicit node_t(Args&&... args)
				: payload(std::forward<Args&&>(args)...)
				, next(0)
			{}
		};

		template<class F>
		class wrap_t final {
			F f_;

		public:
			constexpr explicit wrap_t(F&& f) : f_(std::move(f)) {}

			wrap_t(wrap_t&&) = delete;
			wrap_t& operator=(wrap_t&&) = delete;

			constexpr operator std::invoke_result_t<F&&>()&& {
				return std::move(f_)();
			}
		};

		const std::size_t ringsz;
		sdk::inplaced_t<node_t> ringstorage;
		std::atomic_int64_t head_ = 0;

		std::atomic<type_t> push_end = 0;
		std::atomic<type_t> pop_end = 0;
		std::atomic<type_t> observed_last = 0;

		std::atomic_int64_t count_ = 0;

	public:
		using value_type = T;

		explicit queue_t(std::size_t sz = 1024)
			: ringsz(sz)
			, ringstorage(ringsz) {

			for (std::size_t i = 0; i < ringsz; ++i)
				ringstorage.add();
			///ringstorage.resize(sz * sizeof(node_t));
///			ringstorage.reserve(ringsz);
///			for (std::size_t i = 0; i < ringsz; ++i)
///				ringstorage.emplace_back(wrap_t([]() { return node_t(); }));
		}

		template<class ...Args,
			std::enable_if_t<std::is_constructible_v<T, Args&&...>, bool> = true>
		auto push(Args&&... args) {

			if (auto cnt = count_.load(std::memory_order_relaxed) >= ringsz / 4)
				return false;

			int64_t expected_head = -1, desired_head = -1;
			do {
				desired_head = expected_head + 1;
				if (desired_head >= ringsz) {
					desired_head = 0;
				}
				if (head_.compare_exchange_strong(expected_head, desired_head)) {
					assert(ringstorage.check_index(desired_head));
					auto& bufref = ringstorage[desired_head]; //ringstorage.data() + desired_head * sizeof(node_t);
					bufref.payload = value_type(std::forward<Args&&>(args)...);
					bufref.next.store(0, std::memory_order_seq_cst);
					//auto node = new ((&bufref.payload)) value_type(std::forward<Args&&>(args)...);
					auto node = &bufref;
					const auto desired = reinterpret_cast<type_t>(node);

					type_t expected = 0;
					if (push_end.compare_exchange_strong(expected/*0*/, desired)) {
						// push end is zero
						std::printf("PUSH END IS ZERO !!!\n");
						pop_end.store(desired);
						count_++;
						return true;
					}
					while (true) {
						// push end is non-zero
						const auto parent = expected;
						auto expnode = reinterpret_cast<node_t*>(expected);
						expected = 0;
						if (expnode && expnode->next.compare_exchange_strong(expected/*0*/, desired, std::memory_order_release)) {
							// its ours )))
							push_end.store(desired, std::memory_order_relaxed);
							count_++;
							return true;
						}
						// expected = next not null
						expected = push_end.load(std::memory_order_relaxed);
						//else {
						//	auto next = expected;
						//	expected = parent;
						//	push_end.compare_exchange_strong(expected, next);
						//}
					}
				}
			} while (true);

			assert(false); // unreacheable

			//auto node = alloc::allocate(sizeof(node_t), alignof(node_t));
//			node = new (node) node_t(std::forward<Args&&>(args)...);

			return false;
		}

		enum class state_e : int8_t {
			entry,
			pop_end_zero,
			observed_last_zero,
			exit,
		};

		auto pop(T& output) {
			
			type_t expected = pop_end.load(std::memory_order_relaxed);
			while (true) {
				
				if (expected == 0) { // pop_end is 0
					if (observed_last.compare_exchange_strong(expected/*0*/, 0, std::memory_order_acquire)) { // memory_order_acquire
						// 0->0 observed_last is 0
						return false;
					}
					else {
						// observed_last non-0
						assert(expected > 0);
						auto expnode = reinterpret_cast<node_t*>(expected);
						auto next = expnode->next.load();
						if (next > 0) {
							if (observed_last.compare_exchange_strong(expected, 0, std::memory_order_acquire)) { // set to 0 // memory_order_acquire
								//expnode->~node_t();
								//alloc::deallocate(reinterpret_cast<void*>(expnode), alignof(node_t));
								expected = 0;
								auto rr = pop_end.compare_exchange_strong(expected, next, std::memory_order_release);
								assert(rr);
								////////////////////expnode->next.store(0, std::memory_order_seq_cst);
								continue;
								//pop_end.store(next, std::memory_order_release); // memory_order_release
							}
						}
					}
					return false;
				}
				if (expected > 0) {
					auto expnode = reinterpret_cast<node_t*>(expected);
					auto next = expnode->next.load();
					if (pop_end.compare_exchange_strong(expected, next, std::memory_order_acquire)) { // take non-zero OR pass -1 // memory_order_acquire
						output = std::move(expnode->payload);
						if (next == 0) {
							while (true) {
								type_t obs_expected = 0;
								if (observed_last.compare_exchange_weak(obs_expected/*0*/, expected, std::memory_order_release)) {
									break;
								}
							}
							//observed_last.store(expected, std::memory_order_release); // memory_order_release
						}
						else {
							////////////////////expnode->next.store(0, std::memory_order_seq_cst);
							//expnode->~node_t();
							//alloc::deallocate(reinterpret_cast<void*>(expnode), alignof(node_t));
						}
						count_--;
						return true;
					}
				}
			}
			return false;
		}

	};

}

