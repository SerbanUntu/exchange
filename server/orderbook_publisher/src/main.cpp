#include <server/orderbook_publisher/main.hpp>

#include "common/util/logging.hpp"
#include "server/orderbook_publisher/orderbook_publisher.hpp"

#include <spdlog/logger.h>

namespace exchange::server
{
int orderbookPublisherMain() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::LogService::getLogger(common::LogProducer::ORDERBOOK_PUBLISHER);
        SharedMemory sharedMemory{false};
        ActionSpmcReceiver spmcReceiver{sharedMemory};
        OrderbookPublisher orderbookPublisher{(std::move(spmcReceiver))};
        while (true)
        {
            orderbookPublisher.loop();
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
