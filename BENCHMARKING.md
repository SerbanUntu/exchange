# Benchmarking

## Overview

The matching engine processes about **7.5 million order events per second on a single core**, with a **median latency of 124 ns** and a **p99 latency of about 0.5 μs** per event.

### Results

Median of 5 runs, with the lowest and highest value seen across the runs.

| Metric              | Median            | Range across runs     |
|---------------------|-------------------|-----------------------|
| Throughput          | 7.55 M events/s   | 7.39 to 8.00 M        |
| Mean time per event | 132.5 ns          | 124.9 to 135.3 ns     |
| Latency p50         | 124 ns            | 124 to 128 ns         |
| Latency p90         | 274 ns            | 269 to 285 ns         |
| Latency p99         | 508 ns            | 496 to 562 ns         |
| Latency p99.9       | 1,003 ns          | 965 to 1,296 ns       |
| Latency p99.99      | 11,250 ns         | 10,491 to 21,022 ns   |
| Latency max         | 374,853 ns        | 260,541 to 576,035 ns |

### What is measured

- `MatchingEngine::processEvent` is called directly, without shared memory or gRPC communication. Each call matches the event against the order book and builds the list of resulting actions (trades, additions, removals, upsizes and downsizes).
- Throughput is the time taken for an **untimed** loop over the measured events.
- Latency is measured during a second pass over the same events on a newly instantiated engine, timing each call with `std::chrono::steady_clock`. Freeing the returned action list is counted in the throughput but not in the latency.
- The latency figures include the cost of reading the clock, which has a median of 15 ns per pair of reads.

### Workload

- Events are generated before timing starts by `EventGenerator` (`server/matching_engine/src/event_generator.cpp`) with a fixed seed.
- There are 3 securities with 200 price levels each. Buy prices are shifted 50 ticks down and sell prices 50 ticks up, so 100 of the 200 levels overlap and orders landing there can cross the spread.
- Limit orders are 50% GTC, 25% IOC and 25% FOK, with quantities from 1 to 6.
- Event weights are 40,000 for new limit orders, 5,000 for market orders, 15,000 for amends and 1 per resting order for cancels. Cancels scale with the size of the book, as in the order book model of Cont, Stoikov and Talreja (2010), so the book settles at a steady depth instead of draining or growing forever.
- The resulting mix in the measured run is 59.1% new limit orders, 7.4% market orders, 22.1% amends and 11.4% cancels.
- Amends and cancels target orders that are actually resting. The generator tracks them by observing the actions of a scratch engine while the events are generated. 20% of amends and cancels target unknown order ids instead, to exercise rejections.
- 1,000,000 warm-up events are processed before measuring 4,000,000 events.
- **37.5% of events produce no actions**, mostly IOC and FOK orders that do not cross and rejected amends and cancels. The average is 0.98 actions per event.

### Environment

- CPU: Intel Core Ultra 7 155U (15 W laptop part with hybrid performance and efficiency cores)
- OS: Docker Desktop on Windows 11, WSL2 kernel 6.6.87.2, `ubuntu:24.04` container
- Compiler: GCC 13, C++20, CMake Release build (`-O3`)
- The container is limited to CPU 2 with `--cpuset-cpus=2` and the benchmark thread is pinned to it with `pinToCore()`. Inside WSL2 this pins a virtual CPU, so the host can still move it between physical cores.

### Reproducing

Make sure Docker is installed, then run this from a Unix-compatible shell (i.e., not PowerShell):
```shell
just bench
```
