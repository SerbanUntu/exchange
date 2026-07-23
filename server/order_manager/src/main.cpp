#include <server/order_manager/main.hpp>

#include "common/util/logging.hpp"

#include <spdlog/logger.h>

namespace exchange::server::order
{
int main() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::util::LogService::getLogger(common::util::LogProducer::ORDER_MANAGER);
        LOG->info("Hello from the Order Manager.");
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
} // namespace exchange::server::order

int main()
{
    return exchange::server::order::main();
}
