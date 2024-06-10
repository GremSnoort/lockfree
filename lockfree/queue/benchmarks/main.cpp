// std
#include <vector>
#include <thread>

// conan
#include <benchmark/benchmark.h>
#include <boost/lockfree/queue.hpp>

// lockfree
#include <lockfree/queue/lockfree_queue.hpp>
#include <lockfree/queue/allocator/default.hpp>
#include <lockfree/queue/allocator/mimalloc.hpp>


using type_t = uint64_t;

static const auto threadsCount = 16;

template<typename Q>
class FixtureQueue : public ::benchmark::Fixture {

public:
	std::string payload;
	Q queue;

	std::atomic_bool is_running = true;
	std::vector<std::thread> routines;

	FixtureQueue()
		: queue(1024)
	{}

	auto startUp() {
		is_running = true;
		for (auto i = 0; i < threadsCount; ++i) {
			routines.emplace_back([this]() {
				type_t data;
				while (is_running) {
					//queue.pop(data);
					queue.push(type_t(std::rand()));
				}
			});
		}
	}

	auto tearDown() {
		is_running = false;
		for (auto& r : routines) {
			r.join();
		}
		routines.clear();
	}

	inline auto process(auto& data) {
		while (!queue.pop(data));
	}

	void SetUp(::benchmark::State& state) {
		std::srand(std::time(nullptr)); // use current time as seed for random generator
		payload.resize(1024);
		std::generate(payload.begin(), payload.end(), []() { return std::rand() % 26 + 'a'; });
	}

	void TearDown(::benchmark::State& state) {
	}
};

BENCHMARK_TEMPLATE_DEFINE_F(FixtureQueue, BOOST_QUEUE, ::boost::lockfree::queue<type_t>)(::benchmark::State& st) {
	if (st.thread_index() == 0) {
		startUp();
		// Setup code here.
	}
	type_t data;
	for (auto _ : st) {
		process(data);
	}
	if (st.thread_index() == 0) {
		// Teardown code here.
		tearDown();
	}
}

using namespace ::gremsnoort::lockfree::allocator;

BENCHMARK_TEMPLATE_DEFINE_F(FixtureQueue, GS_DEF_QUEUE, ::gremsnoort::lockfree::queue_t<type_t, default_t>)(::benchmark::State& st) {
	if (st.thread_index() == 0) {
		startUp();
		// Setup code here.
	}
	type_t data;
	for (auto _ : st) {
		process(data);
	}
	if (st.thread_index() == 0) {
		// Teardown code here.
		tearDown();
	}
}

BENCHMARK_TEMPLATE_DEFINE_F(FixtureQueue, GS_MIM_QUEUE, ::gremsnoort::lockfree::queue_t<type_t, mimalloc_t>)(::benchmark::State& st) {
	if (st.thread_index() == 0) {
		startUp();
		// Setup code here.
	}
	type_t data;
	for (auto _ : st) {
		process(data);
	}
	if (st.thread_index() == 0) {
		// Teardown code here.
		tearDown();
	}
}

BENCHMARK_REGISTER_F(FixtureQueue, BOOST_QUEUE)->Threads(threadsCount);
BENCHMARK_REGISTER_F(FixtureQueue, GS_DEF_QUEUE)->Threads(threadsCount);
BENCHMARK_REGISTER_F(FixtureQueue, GS_MIM_QUEUE)->Threads(threadsCount);

int main(int argc, char** argv) {
	::benchmark::Initialize(&argc, argv);
	if (::benchmark::ReportUnrecognizedArguments(argc, argv))
		return 1;
	::benchmark::RunSpecifiedBenchmarks();
	::benchmark::Shutdown();
}
