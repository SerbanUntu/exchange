#include "server/matching_engine/fuzzing/fuzzing.hpp"
#include "server/matching_engine/fuzzing/event_encoding.hpp"

#include "server/common/collections/ipc/action_spmc_sender.hpp"
#include "server/common/collections/ipc/event_mpsc_queue.hpp"
#include "server/common/collections/ipc/shared_memory.hpp"
#include "server/matching_engine/matching_engine.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <span>
#include <stdexcept>
#include <vector>

namespace exchange::server
{

namespace
{
struct FuzzingSharedMemory
{
    SharedMemory memory{true, "FuzzMatchingEngineSharedMemory"};

    FuzzingSharedMemory()
    {
        [[maybe_unused]] const auto *mpscBytes =
            memory.get().construct<std::array<std::byte, SharedMemory::mpscTotalObjectSize>>(SharedMemory::mpscName)();
        [[maybe_unused]] const auto *spmcBytes =
            memory.get().construct<std::array<std::byte, SharedMemory::spmcTotalObjectSize>>(SharedMemory::spmcName)();
    }
};
} // namespace

void fuzzMatchingEngine(const std::span<const uint8_t> bytes)
{
    static FuzzingSharedMemory sharedMemory;
    MatchingEngine engine{EventMpscQueue{sharedMemory.memory}, ActionSpmcSender{sharedMemory.memory},
                          FUZZING_SECURITY_IDS};
    for (const auto &event : decodeEvents(bytes, FUZZING_SECURITY_IDS))
    {
        engine.processEvent(event);
    }
}

void runBinarySeed(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Cannot open " + path.string());
    const std::vector<char> bytes{std::istreambuf_iterator(file), std::istreambuf_iterator<char>()};
    fuzzMatchingEngine({reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size()});
}
} // namespace exchange::server
