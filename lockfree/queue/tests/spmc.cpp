// conan
#include <catch2/catch_all.hpp>

// std
#include <thread>
#include <map>
#include <vector>

// lockfree
#include <lockfree/queue/queue_traits.hpp>

using namespace gremsnoort::lockfree;

TEST_CASE("spmc") {

	// Single-Producer Multi-Consumer model

	static constexpr auto c_count = 8;

	SECTION("int64_t") {

		using type_t = int64_t;
		using queue_type = queue_traits<type_t>::queue_t;

		auto producer = [&](queue_type& source, const std::size_t count) {
			for (std::size_t i = 0; i < count; ++i) {
				while (!source.push(static_cast<type_t>(i)));
			}
		};
		auto consumer = [&](queue_type& source, std::atomic_int64_t& messages_all) {
			int64_t expected = 0, consumed = 0;
			type_t data = 0, prev = -1;
			do {
				if (messages_all.compare_exchange_strong(expected, expected - 1)) {
					if (expected == 0) {
						return;
					}
					while (!source.pop(data));
					REQUIRE(data > prev);
					prev = data;
					consumed++;
				}
			} while (expected > 0);
		};

		auto source = queue_type(1024 * 2);

		for (std::size_t i = 2; i < c_count; ++i) {
			std::printf("STARTED consumers = %zu\n", i);
			const std::size_t iters2prod = 64UL * i;
			std::atomic_int64_t messages_all = iters2prod;
			std::vector<std::thread> consumers;
			for (std::size_t j = 0; j < i; ++j) {
				consumers.emplace_back(consumer, std::ref(source), std::ref(messages_all));
			}
			std::thread p(producer, std::ref(source), iters2prod);
			p.join();
			for (auto& t : consumers) {
				t.join();
			}
		}
	}
}
