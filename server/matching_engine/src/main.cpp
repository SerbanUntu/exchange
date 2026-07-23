#include <server/matching_engine/main.hpp>

#include "common/util/logging.hpp"

#include <spdlog/logger.h>

namespace exchange::server::matcher
{
int main() noexcept
{
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = common::util::LogService::getLogger(common::util::LogProducer::MATCHING_ENGINE);
        LOG->info("Hello from the Matching Engine.");
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
} // namespace exchange::server::matcher

int main()
{
    return exchange::server::matcher::main();
}
