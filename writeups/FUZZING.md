# Fuzzing

- The matching engine is fuzzed with libFuzzer, a coverage-guided fuzzer that is included with Clang. libFuzzer mutates byte arrays, keeps the inputs that reach new code, and reports any input that crashes the process.
- The fuzz target decodes the bytes into a list of events and feeds them to `MatchingEngine::processEvent`, the same entry point the gateway events go through in production.
- Potential crashes come from the orderbook invariants in `orderbook_asserts.hpp` (enabled with `-DASSERTS=ON`), and from ASan and UBSan. A violated invariant calls `assert`, which aborts the process, and libFuzzer saves the input that caused it.
- **No issues were found.** A 10-minute run in CI executed almost 6 million event sequences without a single assertion failure or sanitizer report.

## Input encoding

- Like in the event generator of the differential tests, the domains are small so that the events interact: orders cross, partially fill, and sweep several price levels.
- Every byte value decodes to a valid event, so the fuzzer never wastes executions on inputs that get rejected while parsing.

## Seeds

- The initial list of seeds (`server/matching_engine/fuzzing/seeds`) has 10 inputs, each translated from an existing test, chosen to cover the important paths of the matching engine.
- The seeds are between 12 and 24 bytes (2 to 4 events). libFuzzer can grow these inputs up to its limit.

## Results of the 10-minute run

- 5,981,389 executions in 601 seconds, an average of about 10,000 per second, with Clang 18 with `-O1`, ASan, and UBSan.
- Edge coverage went from 592 with the seeds to 715, and **all 715 edges were reached 2.2 seconds into the run**, after 94,314 executions. The rest of the run only found new features (finer-grained signals like hit counts), which went from 908 to 4,265.
- The list of inputs grew from the original 10 seeds to 879 (new inputs were added whenever they took a new edge or produced a new feature).
- There were no crashes, assertion failures or sanitizer reports.

## Next steps

- Implementing and running the fuzzer on the whole system, using a sequence of client requests rather than matching engine inputs.