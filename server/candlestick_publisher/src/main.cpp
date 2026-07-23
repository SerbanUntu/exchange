#include <server/candlestick_publisher/main.hpp>

#include "common/util/logging.hpp"

#include <spdlog/logger.h>

namespace exchange::server::candlestick
{
int main() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::util::LogService::getLogger(common::util::LogProducer::CANDLESTICK_PUBLISHER);
        LOG->info("Hello from the Candlestick Publisher.");
        return 0;
    }
    catch (const std::exception &err)
    {
        common::util::reportException(LOG, err);
        return 1;
    }
    catch (...)
    {
        common::util::reportUnknownException(LOG);
        return 1;
    }
}
} // namespace exchange::server::candlestick

int main()
{
    return exchange::server::candlestick::main();
}
