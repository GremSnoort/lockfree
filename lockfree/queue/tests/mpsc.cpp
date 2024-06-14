// conan
#include <catch2/catch_all.hpp>

// std
#include <thread>
#include <map>
#include <vector>

// lockfree
#include <lockfree/queue/queue_traits.hpp>

using namespace gremsnoort::lockfree;

TEST_CASE("mpsc") {

	// Multi-Producer Single-Consumer model

	static constexpr auto p_count = 64;
	static constexpr auto iters_per_prod = 1024 * 4;

	SECTION("int64_t") {

		using type_t = int64_t;
		using queue_type = queue_traits<type_t>::queue_t;

		auto producer = [&](queue_type& source, const std::size_t count) {
			for (std::size_t i = 0; i < count; ++i) {
				while (!source.push(static_cast<type_t>(i)));
			}
		};
		auto consumer = [&](queue_type& source, const std::size_t count, const std::size_t pcount) {
			type_t data = 0;
			std::map<type_t, std::size_t> cache;
			for (std::size_t i = 0; i < count; ++i) {
				while (!source.pop(data));
				cache[data]++;
			}
			if (!(cache.size() == pcount)) {
				std::fprintf(stderr, "!!! count = %zu; pcount = %zu\n", count, pcount);
			}
			REQUIRE(cache.size() == pcount);
			const auto producers_c = count / pcount;
			for (const auto& [d, cached] : cache) {
				if (!(cached == producers_c)) {
					std::fprintf(stderr, "!!! cached %zu producers_c %zu :: %zu\n", cached, producers_c, d);
				}
				REQUIRE(cached == producers_c);
			}
			//std::printf("CONSUMER EXIT\n");
		};

		auto source = queue_type(1024 * 2);

		for (std::size_t i = 2; i < p_count; ++i) {
			std::printf("STARTED producers = %zu\n", i);
			std::thread c(consumer, std::ref(source), iters_per_prod * i, iters_per_prod);
			std::vector<std::thread> producers;
			for (std::size_t j = 0; j < i; ++j) {
				producers.emplace_back(producer, std::ref(source), iters_per_prod);
			}
			for (auto& t : producers) {
				t.join();
			}
			c.join();
		}
	}
}
