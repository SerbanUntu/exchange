#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace exchange::common::util
{
/**
 * The components where logs can come from.
 */
enum class LogProducer : uint8_t
{
    CLIENT,
    ORDER_MANAGER
};

/**
 * Manager of spdlog::logger singletons.
 */
class LogService
{
    static std::string producerToString(LogProducer producer);

  public:
    /**
     * Retrieve or create a logger instance for a given producing component.
     *
     * @param producer The component where the log comes from, visible in the logs.
     * @return A logger that will specify its producer.
     */
    static std::shared_ptr<spdlog::logger> getLogger(LogProducer producer);
};
} // namespace exchange::common::util
