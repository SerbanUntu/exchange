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
    case LogProducer::GATEWAY:
        return "gateway";
    default:
        [[unlikely]] throw std::logic_error("Invalid LogProducer value: " + std::to_string(static_cast<int>(producer)));
    }
}

std::shared_ptr<spdlog::logger> LogService::getLogger(const LogProducer producer)
{
    const auto producerString = producerToString(producer);
    auto logger = spdlog::get(producerString);
    if (!logger)
    {
        return spdlog::stdout_color_mt(producerString);
    }
    return logger;
}

void reportException(const std::shared_ptr<spdlog::logger> &logger, const std::exception &err) noexcept
{
    const std::string_view MSG_PREFIX = "The application failed: ";
    if (logger)
    {
        try
        {
            logger->error("{}{}", MSG_PREFIX, err.what());
        }
        catch (...)
        {
            std::fprintf(stderr, "There was an error while writing to the log.");
        }
    }
    else
    {
        std::fprintf(stderr, "%.*s%s\n", static_cast<int>(MSG_PREFIX.size()), MSG_PREFIX.data(), err.what());
    }
}

void reportUnknownException(const std::shared_ptr<spdlog::logger> &logger) noexcept
{
    const std::string_view MSG = "The application failed with an unknown exception.";
    if (logger)
    {
        try
        {
            logger->error(MSG);
        }
        catch (...)
        {
            std::fprintf(stderr, "There was an error while writing to the log.");
        }
    }
    else
    {
        std::fprintf(stderr, "%.*s\n", static_cast<int>(MSG.size()), MSG.data());
    }
}
} // namespace exchange::common::util
