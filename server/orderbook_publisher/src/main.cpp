#include <server/orderbook_publisher/main.hpp>

#include "common/util/logging.hpp"

#include <spdlog/logger.h>

namespace exchange::server::book
{
int main() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::util::LogService::getLogger(common::util::LogProducer::ORDERBOOK_PUBLISHER);
        LOG->info("Hello from the Orderbook Publisher.");
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
} // namespace exchange::server::book

int main()
{
    return exchange::server::book::main();
}
