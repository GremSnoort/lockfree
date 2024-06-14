#pragma once

// std
#include <atomic>
#include <array>
#include <cassert>
#include <concepts>

namespace gremsnoort::lockfree {

	namespace internal {

		using type_t = uint64_t;

		static constexpr std::size_t min_capacity = 8;
		static constexpr std::size_t default_capacity = 1024;
	
		template<class T>
		struct node_t {
			T payload;
			std::atomic<type_t> next_p = 0;
			//std::atomic<type_t> next_c = 0;
			std::atomic_bool busy = false;
			std::atomic_bool consumed = true;

			node_t() = default;
		};
	
	}

	template<class T, class I, class ...Args>
	concept Inplaceable = requires(T & a, const std::size_t i, Args&&... args) {
		std::constructible_from<T, std::size_t>;
		{ a.add(std::forward<Args&&>(args)...) } -> std::same_as<bool>;
		{ a.at(i) } -> std::same_as<I&>;
	};

	template<class T, Inplaceable<internal::node_t<T>> Storage>
	class queue_t {	

		using type_t = internal::type_t;
		using node_t = internal::node_t<T>;

		const std::size_t ringsz;
		Storage ringstorage;
		std::atomic_int64_t head_ = 0;

		std::atomic<type_t> push_end = 0;
		std::atomic<type_t> pop_end = 0;
		std::atomic<type_t> observed_last = 0;

	public:
		using value_type = T;

		explicit queue_t(std::size_t sz = internal::default_capacity)
			: ringsz(sz < internal::min_capacity ? internal::min_capacity : sz)
			, ringstorage(ringsz) {

			for (std::size_t i = 0; i < ringsz; ++i) {
				ringstorage.add();
			}
		}

		template<class ...Args,
			std::enable_if_t<std::is_constructible_v<T, Args&&...>, bool> = true>
		auto push(Args&&... args) {

			int64_t head_expected = 0;

			do {
				auto head_desired = head_expected + 1;
				if (head_desired >= ringsz) {
					head_desired = 0;
				}
				if (head_.compare_exchange_weak(head_expected, head_desired, std::memory_order_relaxed)) {

					auto& bufref = ringstorage.at(head_expected);

					bool busy_expected = false;
					if (bufref.busy.compare_exchange_weak(busy_expected, true, std::memory_order_relaxed)) {
						// DO ----------------------------------------------------------------------------------------
						{
							bufref.payload = value_type(std::forward<Args&&>(args)...);
							bufref.next_p.store(0, std::memory_order_relaxed); // memory_order_seq_cst
							bufref.consumed.store(false, std::memory_order_relaxed); // memory_order_seq_cst
							//bufref.next_c.store(0, std::memory_order_relaxed); // memory_order_seq_cst
							auto node = &bufref;
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
								//
								if (expnode && expnode->next_p.compare_exchange_weak(expected/*0*/, desired, std::memory_order_relaxed)) { // memory_order_release
									// its ours )))
									//expnode->next_c = desired; ////////////////////////////////
									push_end.store(desired, std::memory_order_relaxed); // memory_order_seq_cst
									return true;
								}

								// expected = next not null
								expected = push_end.load(std::memory_order_relaxed);
							}
						}
						// DO ----------------------------------------------------------------------------------------
					}
					else {
						head_expected = head_desired;
					}
				}
			} while (true);

			assert(false); // unreacheable
			return false;
		}

		auto pop(T& output) {
			
			auto expected = pop_end.load(std::memory_order_relaxed);
			while (true) {
				
				if (expected == 0) { // pop_end is 0
					//if (observed_last.compare_exchange_weak(expected/*0*/, 0, std::memory_order_relaxed)) { // memory_order_acquire
					//	// 0->0 observed_last is 0
					//	return false;
					//}
					//else {
					//	// observed_last non-0
					//	assert(expected > 0);
					//	auto expnode = reinterpret_cast<node_t*>(expected);
					//	auto next = expnode->next_p.load();
					//	if (next > 0) {
					//		if (observed_last.compare_exchange_weak(expected, 0, std::memory_order_relaxed)) { // set to 0 // memory_order_acquire
					//			expected = 0;
					//			auto rr = pop_end.compare_exchange_weak(expected, next, std::memory_order_relaxed); // memory_order_release
					//			assert(rr);
					//			//expnode->next_p.store(0, std::memory_order_relaxed); // memory_order_seq_cst
					//			expnode->busy.store(false, std::memory_order_relaxed); // memory_order_seq_cst
					//			continue;
					//		}
					//	}
					//}
					return false;
				}
				if (expected > 0) {
					const auto orig = expected;
					auto expnode = reinterpret_cast<node_t*>(expected);
					auto next = expnode->next_p.load(std::memory_order_relaxed);
					auto consumed = expnode->consumed.load(std::memory_order_relaxed);

					if (next == 0 && consumed) {
						return false;
					}
					///
					///if (!next && !busy) {
					///	return false;
					///}

					if (pop_end.compare_exchange_weak(expected, next, std::memory_order_relaxed)) { // take non-zero OR pass -1 // memory_order_acquire
						bool consumed_expected = false;
						if (expnode->consumed.compare_exchange_strong(consumed_expected, true, std::memory_order_relaxed)) {
							output = std::move(expnode->payload);
						}
						if (next == 0) {
							expected = next;
							auto rr = pop_end.compare_exchange_strong(expected, orig, std::memory_order_relaxed);
							assert(rr);
							expected = orig;
						}
						else {
							bool busy_expected = true;
							auto rr = expnode->busy.compare_exchange_strong(busy_expected, false, std::memory_order_relaxed);
							assert(rr);
						}
						if (!consumed_expected) {
							return true;
						}

						///const auto busy = expnode->busy.load(std::memory_order_relaxed);
						///if (busy) {
						///	
						///	//while (true) {}
						///	bool busy_expected = true;
						///	auto rr = expnode->busy.compare_exchange_strong(busy_expected, false, std::memory_order_relaxed);
						///	assert(rr);
						///}
						///if (busy) return true;
						
						///if (next == 0) {
						///	while (true) {
						///		type_t obs_expected = 0;
						///		if (observed_last.compare_exchange_weak(obs_expected/*0*/, expected, std::memory_order_relaxed)) { // memory_order_release
						///			break;
						///		}
						///	}
						///}
						///else {
						///	//expnode->next_p.store(0, std::memory_order_relaxed); // memory_order_seq_cst
						///	expnode->busy.store(false, std::memory_order_relaxed); // memory_order_seq_cst
						///}
						///return true;
					}
				}
			}
			return false;
		}

	};

}

