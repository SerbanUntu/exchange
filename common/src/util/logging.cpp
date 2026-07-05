#include <common/util/logging.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace exchange::common::util
{
std::string LogService::producerToString(const LogProducer producer)
{
    switch (producer)
    {
    case LogProducer::CLIENT:
        return "client";
    case LogProducer::ORDER_MANAGER:
        return "ord_man";
    default:
        [[unlikely]] throw std::logic_error("Invalid LogProducer value: " + std::to_string(static_cast<int>(producer)));
    }
}

std::shared_ptr<spdlog::logger> LogService::getLogger(const LogProducer producer)
{
    const auto producerString = producerToString(producer);
    const auto logger = spdlog::get(producerString);
    if (!logger)
    {
        return spdlog::stdout_color_mt(producerString);
    }
    return spdlog::get(producerString);
}
} // namespace exchange::common::util
