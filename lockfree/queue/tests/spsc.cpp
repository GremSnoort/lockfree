// conan
#include <catch2/catch_all.hpp>

// std
#include <thread>

// lockfree
#include <lockfree/queue/queue_traits.hpp>

using namespace gremsnoort::lockfree;

TEST_CASE("spsc") {

	SECTION("int64_t") {

		using type_t = int64_t;
		using queue_type = queue_traits<type_t>::queue_t;

		auto producer = [&](queue_type& source, const std::size_t count) {
			for (std::size_t i = 0; i < count; ++i) {
				while (!source.push(static_cast<type_t>(i)));
			}
		};
		auto consumer = [&](queue_type& source, const std::size_t count) {
			type_t data = 0, prev = -1;
			for (std::size_t i = 0; i < count; ++i) {
				while (!source.pop(data));
				REQUIRE(data - prev == 1);
				prev = data;
			}
		};
		auto source = queue_type(1024);
		for (std::size_t i = 1; i < 10000; ++i) {
			std::thread
				c(consumer, std::ref(source), i),
				p(producer, std::ref(source), i);
			p.join();
			c.join();
		}		
	}
}
