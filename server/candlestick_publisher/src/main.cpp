#include <server/candlestick_publisher/main.hpp>

#include "common/util/logging.hpp"

#include <spdlog/logger.h>

namespace exchange::server
{
int candlestickPublisherMain() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::LogService::getLogger(common::LogProducer::CANDLESTICK_PUBLISHER);
        LOG->info("Hello from the Candlestick Publisher.");
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

int main()
{
    return exchange::server::candlestickPublisherMain();
}
