# Differential testing

- The matching engine is checked against `ReferenceOrderBook` and `referenceProcessEvent`, a simple reference implementation of the orderbook and matching engine logic. The orderbook is a `std::vector<Order>` per security, with linear scans to find the best price. Time priority is the position in the vector.
- Every handwritten test in `test_matching_engine.cpp` runs the reference alongside the engine and diffs the two lists of actions. On top of that, a seeded randomized test replays generated event sequences through both implementations and fails on the first divergence.
- **No issues were found**. Neither the handwritten tests nor 1,000,000 random seeds of 40 events each  produced a single divergence, so the matching engine did not need any changes.
- The randomized test is fast. The default of 20,000 seeds (800k events) runs in about 0.6-1 s, and 1,000,000 seeds run in about 25-45 s locally.
- On failure, the test prints the seed, every event up to the divergence together with the actions it produced, and a positional diff of the expected and actual actions of the diverging event.

## Event generator

- Domains are deliberately small so that events interact: a pool of 16 order IDs, 5 prices (98-102), quantities 1-6, both sides, all time in force values (GTC weighted twice as much as IOC and FOK, so the book fills up) and all four amend variants (quantity, price, both, neither).
- Each event goes to a security picked uniformly from the list passed to the generator.
- New limit and market orders always get a new ID, because in production the gateway assigns order IDs and clients cannot reuse them. Once the ID pool is exhausted, the generator only produces amends and cancels.
- The event mix is 45% limit orders, 10% market orders, 30% amends and 15% cancels.
- Random numbers come from `std::mt19937_64() % n` instead of `std::uniform_int_distribution`. The output of `mt19937_64` is specified by the standard, but the distributions are implementation-defined, so a seed failing in the CI running on Linux reproduces exactly on Windows.
