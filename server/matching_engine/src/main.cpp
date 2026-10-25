#include "server/matching_engine/main.hpp"
#include "common/util/logging.hpp"
#include "server/common/collections/ipc/event_mpsc_queue.hpp"
#include "server/common/collections/ipc/shared_memory.hpp"
#include "server/common/collections/ipc/action_spmc_sender.hpp"
#include "server/common/model/action/action.hpp"
#include "server/common/model/event/event.hpp"
#include "server/matching_engine/matching_engine.hpp"
#include "common/model/universe.hpp"

#include <boost/interprocess/managed_shared_memory.hpp>

#include <spdlog/logger.h>

namespace exchange::server
{

int matchingEngineMain() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::LogService::getLogger(common::LogProducer::MATCHING_ENGINE);
        LOG->info("Hello from the Matching Engine.");

        SharedMemory::Deleter sharedMemoryDeleter;
        SharedMemory sharedMemory(true);

        [[maybe_unused]] const auto *mpscBuffer =
            sharedMemory.get().construct<std::array<std::byte, SharedMemory::mpscTotalObjectSize>>(
                SharedMemory::mpscName)();
        [[maybe_unused]] const auto *spmcBuffer =
            sharedMemory.get().construct<std::array<std::byte, SharedMemory::spmcTotalObjectSize>>(
                SharedMemory::spmcName)();

        EventMpscQueue mpsc{sharedMemory};
        ActionSpmcSender spmcSender{sharedMemory};
        std::array<SecurityId, common::Universe::NUMBER_OF_SECURITIES> securityIds;
        const auto &securities = common::Universe::getSecurities();
        for (size_t i = 0; i < securities.size(); ++i)
        {
            securityIds[i] = SecurityId(securities[i].security_id());
        }

        MatchingEngine matchingEngine{(std::move(mpsc)), (std::move(spmcSender)), securityIds};
        while (true)
        {
            matchingEngine.loop();
        }

        return 0;
    }
    catch (const std::exception &err)
    {
        common::reportException(LOG, err);
        return 1;
    }
    catch (...)
    {
        common::reportUnknownException(LOG);
        return 1;
    }
}
} // namespace exchange::server
