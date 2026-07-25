#include <server/matching_engine/main.hpp>

#include "common/util/logging.hpp"
#include "server/common/collections/orderbook/orderbook.hpp"
#include "server/common/model/action/action.hpp"
#include "server/common/model/event/event.hpp"

#include <spdlog/logger.h>

namespace exchange::server::matcher
{

std::vector<common::model::Action> processEvent(const common::model::Event &event,
                                              common::collections::OrderBook &orderBook);

int main() noexcept
{
    namespace util = exchange::common::util;
    std::shared_ptr<spdlog::logger> LOG = nullptr;
    try
    {
        LOG = util::LogService::getLogger(util::LogProducer::MATCHING_ENGINE);
        LOG->info("Hello from the Matching Engine.");
        return 0;
    }
    catch (const std::exception &err)
    {
        util::reportException(LOG, err);
        return 1;
    }
    catch (...)
    {
        util::reportUnknownException(LOG);
        return 1;
    }
}
} // namespace exchange::server::matcher

int main()
{
    return exchange::server::matcher::main();
}
