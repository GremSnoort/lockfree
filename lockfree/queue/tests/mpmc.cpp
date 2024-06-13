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

	// Multi-Producer Multi-Consumer model

	SECTION("int64_t") {

		using type_t = int64_t;
		using queue_type = queue_traits<type_t>::queue_t;

		auto producer = [&](queue_type& source, const std::size_t count) {
			for (std::size_t i = 0; i < count; ++i) {
				while (!source.push(static_cast<type_t>(i)));
			}
		};
		auto consumer = [&](queue_type& source, std::atomic_int64_t& messages_all, std::atomic_size_t& consumed) {
			int64_t expected = 0;
			type_t data = 0;
			do {
				if (messages_all.compare_exchange_strong(expected, expected - 1)) {
					if (expected == 0) {
						return;
					}
					while (!source.pop(data));
					consumed++;
				}
			} while (expected > 0);
		};
		auto source = queue_type(1024 * 2);

		for (std::size_t p = 2; p < 24; ++p) {
			for (std::size_t c = 2; c < 24; ++c) {
				std::printf("STARTED p/c = %zu/%zu\n", p, c);
				const std::size_t iters2prod = 64UL * p * c;
				std::atomic_int64_t messages_all = iters2prod;
				std::atomic_size_t consumed = 0;
				std::vector<std::thread> producers, consumers;
				for (std::size_t j = 0; j < c; ++j) {
					consumers.emplace_back(consumer, std::ref(source), std::ref(messages_all), std::ref(consumed));
				}
				for (std::size_t j = 0; j < p; ++j) {
					producers.emplace_back(producer, std::ref(source), iters2prod / p);
				}
				for (auto& t : producers) {
					t.join();
				}
				for (auto& t : consumers) {
					t.join();
				}
				REQUIRE(consumed.load() == iters2prod);
			}
		}
	}
}
