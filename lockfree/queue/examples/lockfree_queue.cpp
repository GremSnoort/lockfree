// lockfree
#include <lockfree/queue/lockfree_queue.hpp>
#include <lockfree/queue/allocator/default.hpp>

// sdk
#include <sdk/forward/program_options.hpp>
#include <sdk/benchmark/time/thread_timer.h>

// std
#include <vector>
#include <thread>

int main(int argc, char* argv[]) {

	gremsnoort::sdk::program_options_t desc(__FILE__, "One line description");
	static constexpr auto
		help_o = "help",
		prod_o = "p", prod_descr = "Producers count",
		cons_o = "c", cons_descr = "Consumers count",
		msgc_o = "m", msgc_descr = "Message count per producer",
		sleep_o = "s", sleep_descr = "Sleep on send per producer, in microseconds";

	desc.add_options()
		(help_o, "produce help message")
		(prod_o, prod_descr, cxxopts::value<std::size_t>())
		(cons_o, cons_descr, cxxopts::value<std::size_t>())
		(msgc_o, msgc_descr, cxxopts::value<std::size_t>())
		(sleep_o, sleep_descr, cxxopts::value<std::size_t>())
		;

	auto result = desc.parse(argc, argv);

	if (result.count(help_o)) {
		std::cout << desc.help() << "\n";
		return 1;
	}

	std::size_t
		producers_count = 4,
		consumers_count = 4,
		messages_count = 1000,
		sleep_on_send = 0;

	desc.retrieve_opt(producers_count, prod_o, prod_descr, result, false);
	desc.retrieve_opt(consumers_count, cons_o, cons_descr, result, false);
	desc.retrieve_opt(messages_count, msgc_o, msgc_descr, result, false);
	desc.retrieve_opt(sleep_on_send, sleep_o, sleep_descr, result, false);

	std::atomic_size_t messages_all = producers_count * messages_count;

	if (messages_all <= 0 || consumers_count <= 0) {
		return 0;
	}

	using type_t = uint64_t;
	using alloc_t = gremsnoort::lockfree::allocator::default_t;
	using queue_t = gremsnoort::lockfree::queue_t<type_t, alloc_t>;

	queue_t source;

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
	

	auto producer = [&source, &messages_count, &sleep_on_send]() {
		ThreadTimer timer(ThreadTimer::CreateProcessCpuTime());
		for (auto i = 0; i < messages_count; ++i) {
			if (sleep_on_send > 0) {
				std::this_thread::sleep_for(std::chrono::microseconds(sleep_on_send));
			}
			time_checker_t _(timer);
			source.push(type_t(i));
		}
		const auto cpu_time_used = static_cast<double>(timer.cpu_time_used()) / static_cast<double>(messages_count) * 1000000;
		const auto real_time_used = static_cast<double>(timer.real_time_used()) / static_cast<double>(messages_count) * 1000000;

		std::printf("%s\n", std::format("PRODUCER ({}) :: cpu_time_used = {} mcs, real_time_used = {} mcs", messages_count, cpu_time_used, real_time_used).data());
	};
	auto consumer = [&source, &messages_all]() {
		ThreadTimer timer(ThreadTimer::CreateProcessCpuTime());
		std::size_t expected = 0, consumed = 0;
		type_t data;
		do {
			if (messages_all.compare_exchange_weak(expected, expected - 1)) {
				if (expected == 0) {
					return;
				}
				time_checker_t _(timer);
				while (!source.pop(data));
				consumed++;
			}
		} while (expected > 0);

		const auto cpu_time_used = static_cast<double>(timer.cpu_time_used()) / static_cast<double>(consumed) * 1000000;
		const auto real_time_used = static_cast<double>(timer.real_time_used()) / static_cast<double>(consumed) * 1000000;

		std::printf("%s\n", std::format("CONSUMER ({}) :: cpu_time_used = {} mcs, real_time_used = {} mcs", consumed, cpu_time_used, real_time_used).data());
	};

	std::vector<std::thread> producers, consumers;

	for (auto i = 0; i < consumers_count; ++i) {
		consumers.emplace_back(consumer);
	}
	for (auto i = 0; i < producers_count; ++i) {
		producers.emplace_back(producer);
	}

	for (auto& p : producers) {
		p.join();
	}
	for (auto& c : consumers) {
		c.join();
	}

	return 0;
}
