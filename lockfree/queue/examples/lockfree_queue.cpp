// sdk
#include <gremsnoort/sdk/forward/program_options.hpp>
#include <gremsnoort/sdk/benchmark/time/thread_timer.h>

// lockfree
#include <lockfree/queue/queue_traits.hpp>

// std
#include <vector>
#include <thread>

using ThreadTimer = gremsnoort::sdk::benchmark::ThreadTimer;
struct time_checker_t final {
	ThreadTimer& ref;

	explicit time_checker_t(ThreadTimer& timer)
		: ref(timer) {
		ref.StartTimer();
	}
	~time_checker_t() {
		ref.StopTimer();
	}
};

struct options_t {
	std::size_t producers_count = 4;
	std::size_t consumers_count = 4;
	std::size_t messages_count = 1000;
	std::size_t sleep_on_send = 0;
};

template<class T>
class process_t {
	using type_t = T;
	using queue_t = gremsnoort::lockfree::queue_traits<type_t>::queue_t;

	queue_t source;

	std::atomic_int64_t messages_all;
	std::vector<std::thread> producers, consumers;

public:
	process_t() : source() {}

	auto start(const options_t& opts) {
		messages_all = opts.producers_count * opts.messages_count;

		auto producer = [this, &opts]() {
			ThreadTimer timer(ThreadTimer::CreateProcessCpuTime());
			for (std::size_t i = 0; i < opts.messages_count; ++i) {
				if (opts.sleep_on_send > 0) {
					std::this_thread::sleep_for(std::chrono::microseconds(opts.sleep_on_send));
				}
				time_checker_t _(timer);
				while (!source.push(type_t(i)));
			}
			const auto cpu_time_used = static_cast<double>(timer.cpu_time_used()) / static_cast<double>(opts.messages_count) * 1000000;
			const auto real_time_used = static_cast<double>(timer.real_time_used()) / static_cast<double>(opts.messages_count) * 1000000;

			std::printf("%s\n", std::format("PRODUCER ({}) :: cpu_time_used = {} mcs, real_time_used = {} mcs", opts.messages_count, cpu_time_used, real_time_used).data());
		};
		auto consumer = [this, &opts]() {
			ThreadTimer timer(ThreadTimer::CreateProcessCpuTime());
			int64_t expected = 0, consumed = 0;
			type_t data;
			do {
				if (messages_all.compare_exchange_strong(expected, expected - 1)) {
					if (expected == 0) {
						return;
					}
					if (expected < 2)
						std::printf("Expected %zu\n", expected);
					time_checker_t _(timer);
					while (!source.pop(data));
					consumed++;
				}
			} while (expected > 0);

			const auto cpu_time_used = static_cast<double>(timer.cpu_time_used()) / static_cast<double>(consumed) * 1000000;
			const auto real_time_used = static_cast<double>(timer.real_time_used()) / static_cast<double>(consumed) * 1000000;

			std::printf("%s\n", std::format("CONSUMER ({}) :: cpu_time_used = {} mcs, real_time_used = {} mcs", consumed, cpu_time_used, real_time_used).data());
		};

		for (std::size_t i = 0; i < opts.consumers_count; ++i) {
			consumers.emplace_back(consumer);
		}
		for (std::size_t i = 0; i < opts.producers_count; ++i) {
			producers.emplace_back(producer);
		}
	}

	auto stop() {
		for (auto& p : producers) {
			p.join();
		}
		producers.clear();
		for (auto& c : consumers) {
			c.join();
		}
		consumers.clear();
	}
};

int main(int argc, char* argv[]) {

	gremsnoort::sdk::program_options_t desc(std::string(__FILE__), std::string("One line description"));
	static constexpr auto
		help_o = "help",
		prod_o = "p", prod_descr = "Producers count",
		cons_o = "c", cons_descr = "Consumers count",
		msgc_o = "m", msgc_descr = "Message count per producer",
		alloc_o = "a", alloc_descr = "Allocator (default/mimalloc)",
		sleep_o = "s", sleep_descr = "Sleep on send per producer, in microseconds";

	desc.add_options()
		(help_o, "produce help message")
		(prod_o, prod_descr, cxxopts::value<std::size_t>())
		(cons_o, cons_descr, cxxopts::value<std::size_t>())
		(msgc_o, msgc_descr, cxxopts::value<std::size_t>())
		(alloc_o, alloc_descr, cxxopts::value<std::string>())
		(sleep_o, sleep_descr, cxxopts::value<std::size_t>())
		;

	auto result = desc.parse(argc, argv);

	if (result.count(help_o)) {
		std::cout << desc.help() << "\n";
		return 1;
	}

	options_t opts;
	std::string allocator = "default";

	desc.retrieve_opt(opts.producers_count, prod_o, prod_descr, result, false);
	desc.retrieve_opt(opts.consumers_count, cons_o, cons_descr, result, false);
	desc.retrieve_opt(opts.messages_count, msgc_o, msgc_descr, result, false);
	desc.retrieve_opt(allocator, alloc_o, alloc_descr, result, false);
	desc.retrieve_opt(opts.sleep_on_send, sleep_o, sleep_descr, result, false);

	if (opts.producers_count <= 0 || opts.consumers_count <= 0 || opts.messages_count <= 0) {
		return 0;
	}

	using type_t = uint64_t;

	auto process = [&opts](auto&& p) {
		p.start(opts);
		p.stop();
	};

	for (auto i = 0; i < 100; ++i) {
		if (allocator == "default") {
			process(process_t<type_t>());
		}
		else if (allocator == "mimalloc") {
			process(process_t<type_t>());
		}
		//else if (allocator == "jemalloc") {
		//	process(process_t<type_t, gremsnoort::lockfree::allocator::jemalloc_t>());
		//}
		else {
			std::fprintf(stderr, "!!! Unsupported alloc `%s` !!!\n", allocator.data());
		}
	}	

	return 0;
}
