#include <server/execution_publisher/main.hpp>

#include "common/util/logging.hpp"

#include <spdlog/logger.h>

namespace exchange::server::execution
{
int main() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::util::LogService::getLogger(common::util::LogProducer::EXECUTION_PUBLISHER);
        LOG->info("Hello from the Execution Publisher.");
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
} // namespace exchange::server::execution

int main()
{
    return exchange::server::execution::main();
}
