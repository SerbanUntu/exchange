#include <server/candlestick_publisher/main.hpp>

#include "common/util/logging.hpp"
#include "server/candlestick_publisher/candlestick_publisher.hpp"
#include "server/common/collections/ipc/shared_memory.hpp"
#include "server/common/collections/ipc/spmc_queue.hpp"
#include "server/common/model/action/action.hpp"

#include <spdlog/logger.h>

namespace exchange::server
{
int candlestickPublisherMain() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::LogService::getLogger(common::LogProducer::CANDLESTICK_PUBLISHER);

        SharedMemory sharedMemory(false);
        auto [spmc, _] = sharedMemory.get().find<SPMCQueue<Action>>(SharedMemory::spmcName);

        CandlestickPublisher candlestickPublisher(*spmc);
        while (true)
        {
            candlestickPublisher.loop();
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
