#include "common/model/universe.hpp"
#include "common/util/cpu_scheduling.hpp"
#include "common/util/logging.hpp"
#include "server/common/collections/ipc/action_spmc_sender.hpp"
#include "server/common/collections/ipc/event_mpsc_queue.hpp"
#include "server/common/collections/ipc/shared_memory.hpp"
#include "server/common/model/action/action.hpp"
#include "server/common/model/action/order_added_action.hpp"
#include "server/common/model/action/order_removed_action.hpp"
#include "server/common/model/event/amend_limit_order_event.hpp"
#include "server/common/model/event/cancel_limit_order_event.hpp"
#include "server/common/model/event/event.hpp"
#include "server/common/model/event/new_limit_order_event.hpp"
#include "server/common/model/event/new_market_order_event.hpp"
#include "server/common/model/value_object/security_id.hpp"
#include "server/matching_engine/event_generator.hpp"
#include "server/matching_engine/matching_engine.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <format>
#include <iostream>
#include <limits>
#include <span>
#include <variant>
#include <vector>

namespace
{
using exchange::server::Action;
using exchange::server::ActionSpmcSender;
using exchange::server::AmendLimitOrderEvent;
using exchange::server::CancelLimitOrderEvent;
using exchange::server::Event;
using exchange::server::EventGenerator;
using exchange::server::EventMpscQueue;
using exchange::server::MatchingEngine;
using exchange::server::NewLimitOrderEvent;
using exchange::server::NewMarketOrderEvent;
using exchange::server::OrderAddedAction;
using exchange::server::OrderRemovedAction;
using exchange::server::SecurityId;
using exchange::server::SharedMemory;
using Clock = std::chrono::steady_clock;

constexpr uint64_t GENERATOR_SEED = 42;
constexpr std::size_t WARM_UP_EVENTS = 1'000'000;
constexpr std::size_t MEASURED_EVENTS = 4'000'000;
constexpr std::size_t CLOCK_OVERHEAD_SAMPLES = 1'000'000;
constexpr EventGenerator::Config BENCHMARK_CONFIG{
    .orderIdPool = std::numeric_limits<uint64_t>::max(),
    .minPrice = 1000,
    .priceCount = 200,
    .sidePriceOffset = 50,
    .newLimitWeight = 40000,
    .marketWeight = 5000,
    .amendWeight = 15000,
    .cancelWeightPerLiveOrder = 1,
};
constexpr std::array PERCENTILES{50.0, 90.0, 99.0, 99.9, 99.99};

struct BookStatistics
{
    int64_t restingOrders = 0;
    std::size_t noOpEvents = 0;
    std::size_t actions = 0;

    void record(const std::vector<Action> &result)
    {
        actions += result.size();
        if (result.empty())
            ++noOpEvents;
        for (const auto &action : result)
        {
            if (std::holds_alternative<OrderAddedAction>(action.body))
                ++restingOrders;
            else if (std::holds_alternative<OrderRemovedAction>(action.body))
                --restingOrders;
        }
    }
};

struct ThroughputResult
{
    Clock::duration elapsed;
    std::size_t actions;
};

template <typename Body> double eventShare(const std::span<const Event> events)
{
    const auto count =
        std::ranges::count_if(events, [](const Event &event) { return std::holds_alternative<Body>(event.body); });
    return 100.0 * static_cast<double>(count) / static_cast<double>(events.size());
}

int64_t toNanoseconds(const Clock::duration duration)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

std::vector<SecurityId> securityIds()
{
    std::vector<SecurityId> ids;
    for (const auto &security : exchange::common::Universe::getSecurities())
        ids.emplace_back(security.security_id());
    return ids;
}

MatchingEngine createEngine(SharedMemory &sharedMemory, const std::span<const SecurityId> securities)
{
    return MatchingEngine{EventMpscQueue{sharedMemory}, ActionSpmcSender{sharedMemory}, securities};
}

std::vector<Event> generateEvents(SharedMemory &sharedMemory, const std::span<const SecurityId> securities)
{
    EventGenerator generator{GENERATOR_SEED, securities, BENCHMARK_CONFIG};
    auto scratchEngine = createEngine(sharedMemory, securities);
    std::vector<Event> events;
    events.reserve(WARM_UP_EVENTS + MEASURED_EVENTS);
    while (events.size() < WARM_UP_EVENTS + MEASURED_EVENTS)
    {
        const auto &event = events.emplace_back(generator.next());
        generator.observeActions(scratchEngine.processEvent(event));
    }
    return events;
}

BookStatistics warmUp(MatchingEngine &engine, const std::span<const Event> events)
{
    BookStatistics statistics;
    for (const auto &event : events)
        statistics.record(engine.processEvent(event));
    return statistics;
}

ThroughputResult measureThroughput(MatchingEngine &engine, const std::span<const Event> events)
{
    std::size_t actions = 0;
    const auto start = Clock::now();
    for (const auto &event : events)
        actions += engine.processEvent(event).size();
    return {.elapsed = Clock::now() - start, .actions = actions};
}

std::vector<int64_t> measureLatencies(MatchingEngine &engine, const std::span<const Event> events,
                                      BookStatistics &statistics)
{
    std::vector<int64_t> latencies(events.size());
    for (std::size_t i = 0; i < events.size(); ++i)
    {
        const auto start = Clock::now();
        const auto actions = engine.processEvent(events[i]);
        const auto end = Clock::now();
        latencies[i] = toNanoseconds(end - start);
        statistics.record(actions);
    }
    return latencies;
}

int64_t percentile(const std::span<const int64_t> sorted, const double percent)
{
    const auto rank = static_cast<std::size_t>(std::ceil(percent / 100.0 * static_cast<double>(sorted.size())));
    return sorted[std::clamp<std::size_t>(rank, 1, sorted.size()) - 1];
}

int64_t medianClockOverhead()
{
    std::vector<int64_t> samples(CLOCK_OVERHEAD_SAMPLES);
    for (auto &sample : samples)
    {
        const auto start = Clock::now();
        const auto end = Clock::now();
        sample = toNanoseconds(end - start);
    }
    std::ranges::sort(samples);
    return percentile(samples, 50.0);
}

int run()
{
    if (!exchange::common::pinToCore())
        std::cerr << "Warning: could not pin the benchmark to a single CPU core\n";

    SharedMemory sharedMemory{true, "BenchmarkMatchingEngineSharedMemory"};
    [[maybe_unused]] const auto *mpscBytes =
        sharedMemory.get().construct<std::array<std::byte, SharedMemory::mpscTotalObjectSize>>(
            SharedMemory::mpscName)();
    [[maybe_unused]] const auto *spmcBytes =
        sharedMemory.get().construct<std::array<std::byte, SharedMemory::spmcTotalObjectSize>>(
            SharedMemory::spmcName)();

    const auto securities = securityIds();
    const auto events = generateEvents(sharedMemory, securities);
    const auto warmUpEvents = std::span{events}.first(WARM_UP_EVENTS);
    const auto measuredEvents = std::span{events}.subspan(WARM_UP_EVENTS);

    ThroughputResult throughput{};
    {
        auto engine = createEngine(sharedMemory, securities);
        warmUp(engine, warmUpEvents);
        throughput = measureThroughput(engine, measuredEvents);
    }

    BookStatistics warmUpStatistics;
    BookStatistics measuredStatistics;
    std::vector<int64_t> latencies;
    {
        auto engine = createEngine(sharedMemory, securities);
        warmUpStatistics = warmUp(engine, warmUpEvents);
        measuredStatistics.restingOrders = warmUpStatistics.restingOrders;
        latencies = measureLatencies(engine, measuredEvents, measuredStatistics);
    }
    std::ranges::sort(latencies);

    const auto measuredCount = static_cast<double>(measuredEvents.size());
    const auto elapsedNanoseconds = static_cast<double>(toNanoseconds(throughput.elapsed));
    const auto eventsPerSecond = measuredCount / elapsedNanoseconds * 1e9;

    std::cout << std::format("Securities: {}\n", securities.size());
    std::cout << std::format("Price levels per security: {}\n", BENCHMARK_CONFIG.priceCount);
    std::cout << std::format("Warm-up events: {}\n", warmUpEvents.size());
    std::cout << std::format("Measured events: {}\n", measuredEvents.size());
    std::cout << std::format(
        "Event mix: {:.1f}% new limit, {:.1f}% new market, {:.1f}% amend, {:.1f}% cancel\n",
        eventShare<NewLimitOrderEvent>(measuredEvents), eventShare<NewMarketOrderEvent>(measuredEvents),
        eventShare<AmendLimitOrderEvent>(measuredEvents), eventShare<CancelLimitOrderEvent>(measuredEvents));
    std::cout << std::format("Resting orders after warm-up: {}\n", warmUpStatistics.restingOrders);
    std::cout << std::format("Resting orders after measured run: {}\n", measuredStatistics.restingOrders);
    std::cout << std::format("Events producing no actions: {:.1f}%\n",
                             100.0 * static_cast<double>(measuredStatistics.noOpEvents) / measuredCount);
    std::cout << std::format("Actions per event: {:.2f}\n",
                             static_cast<double>(measuredStatistics.actions) / measuredCount);
    std::cout << std::format("Passes produced identical actions: {}\n",
                             throughput.actions == measuredStatistics.actions ? "yes" : "no");
    std::cout << std::format("Throughput: {:.2f} million events/s\n", eventsPerSecond / 1e6);
    std::cout << std::format("Mean time per event: {:.1f} ns\n", elapsedNanoseconds / measuredCount);
    for (const auto percent : PERCENTILES)
        std::cout << std::format("Latency p{}: {} ns\n", percent, percentile(latencies, percent));
    std::cout << std::format("Latency max: {} ns\n", latencies.back());
    std::cout << std::format("Clock overhead (median): {} ns\n", medianClockOverhead());
    return 0;
}
} // namespace

int main()
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = exchange::common::LogService::getLogger(exchange::common::LogProducer::BENCHMARK);
        return run();
    }
    catch (const std::exception &err)
    {
        exchange::common::reportException(LOG, err);
        return 1;
    }
    catch (...)
    {
        exchange::common::reportUnknownException(LOG);
        return 1;
    }
}
