#include <server/orderbook_publisher/main.hpp>

#include "common/util/logging.hpp"

#include <spdlog/logger.h>

namespace exchange::server
{
int orderbookPublisherMain() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::LogService::getLogger(common::LogProducer::ORDERBOOK_PUBLISHER);
        LOG->info("Hello from the Orderbook Publisher.");
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
