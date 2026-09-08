#include "server/matching_engine/main.hpp"
#include "common/util/logging.hpp"

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
