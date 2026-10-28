#include "server/matching_engine/fuzzing/fuzzing.hpp"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    exchange::server::fuzzMatchingEngine({data, size});
    return 0;
}
