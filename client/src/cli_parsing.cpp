#include "client/cli_parsing.hpp"

#include <CLI/CLI.hpp>
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/time_util.h>

#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

namespace exchange::client
{

namespace cli_option_names
{

const auto SERVER_URL = "--server";
const auto SECURITY_ID = "--security";
const auto SIDE = "--side";
const auto QUANTITY = "-q,--quantity";
const auto PRICE_CENTS = "-p,--price-cents";
const auto TIF = "--tif";
const auto ORDER_ID = "-o,--order";
const auto RESOLUTION_SECONDS = "--resolution-seconds";
const auto START_TIME = "--start";
const auto END_TIME = "--end";
const auto NUMBER_OF_LEVELS = "--number-of-levels";
const auto SYMBOL = "--symbol";

} // namespace cli_option_names

namespace cli_arg_descriptions
{

const auto SERVER_URL = "The host:port string corresponding to the exchange server's address";
const auto SECURITY_ID = "The numerical id of the security (1, 2, or 3)";
const auto SIDE = R"("buy" or "sell")";
const auto QUANTITY = "The number of units to purchase/sell";
const auto PRICE_CENTS = "The price in cents for the given order";
const auto TIF = R"(The time in force qualifier ("GTC", "IOC", or "FOK"))";
const auto ORDER_ID = "The UUID of the order";
const auto RESOLUTION_SECONDS = "How many seconds each candle should cover";
const auto START_TIME = "The beginning timestamp, in the RFC 3339 format (e.g. 2026-11-04T09:30:00Z)";
const auto END_TIME = "The end timestamp, in the RFC 3339 format (e.g. 2026-11-04T09:30:00Z)";
const auto NUMBER_OF_LEVELS = "The number of (price, quantity) pairs to provide for the best buys and sells";
const auto SYMBOL = "The stock ticker symbol of the security";

} // namespace cli_arg_descriptions

ParseCommandResult parseCommand(int argc, char **argv, std::string &serverUrl)
{
    namespace opt = cli_option_names;
    namespace desc = cli_arg_descriptions;

    const auto sideValidator = CLI::IsMember{std::vector<std::string>{"buy", "sell"}, CLI::ignore_case} &
                               CLI::Transformer{std::unordered_map<std::string, proto::Side>{
                                   {"buy", proto::Side::SIDE_BUY}, {"sell", proto::Side::SIDE_SELL}}};
    const auto tifValidator = CLI::IsMember{std::vector<std::string>{"gtc", "ioc", "fok"}, CLI::ignore_case} &
                              CLI::Transformer{std::unordered_map<std::string, proto::TimeInForce>{
                                  {"gtc", proto::TimeInForce::TIME_IN_FORCE_GTC},
                                  {"ioc", proto::TimeInForce::TIME_IN_FORCE_IOC},
                                  {"fok", proto::TimeInForce::TIME_IN_FORCE_FOK}}};
    const auto timestampValidator = CLI::Validator{
        [](const std::string &value) -> std::string {
            if (google::protobuf::Timestamp timestamp; google::protobuf::util::TimeUtil::FromString(value, &timestamp))
            {
                return {};
            }
            return "expected an RFC 3339 timestamp (YYYY-MM-DDTHH:MM:SS[.s](Z | +HH:MM | -HH:MM)), got '" + value + "'";
        },
        "TIMESTAMP"};
    const auto uuidValidator = CLI::Validator{
        [uuidPattern = std::regex{"[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}"}](
            const std::string &value) -> std::string {
            if (std::regex_match(value, uuidPattern))
            {
                return {};
            }
            return "expected a UUID, got '" + value + "'";
        },
        "UUID"};

    uint32_t securityId;
    proto::Side side;
    uint64_t quantity;
    uint64_t priceCents;
    proto::TimeInForce tif;
    std::string orderId;
    uint32_t resolutionSeconds;
    std::string startTime;
    std::string endTime;
    uint32_t numberOfLevels;
    std::string symbol;

    CLI::App app{"CLI-based client for the stock exchange."};
    app.fallthrough();
    app.require_subcommand(1);

    app.add_option(opt::SERVER_URL, serverUrl, desc::SERVER_URL)->capture_default_str();

    auto *orderSubcommand = app.add_subcommand("order", "Request to send, modify, or cancel an order, receiving an "
                                                        "acknowledgement that the request will be processed.")
                                ->require_subcommand(1);

    auto *limitOrderSubcommand = orderSubcommand->add_subcommand("limit", "Send a limit order.");
    limitOrderSubcommand->add_option(opt::SECURITY_ID, securityId, desc::SECURITY_ID)->required();
    limitOrderSubcommand->add_option(opt::SIDE, side, desc::SIDE)->required()->transform(sideValidator);
    limitOrderSubcommand->add_option(opt::QUANTITY, quantity, desc::QUANTITY)->required()->check(CLI::PositiveNumber);
    limitOrderSubcommand->add_option(opt::PRICE_CENTS, priceCents, desc::PRICE_CENTS)
        ->required()
        ->check(CLI::PositiveNumber);
    limitOrderSubcommand->add_option(opt::TIF, tif, desc::TIF)->required()->transform(tifValidator);

    auto *marketOrderSubcommand =
        orderSubcommand->add_subcommand("market", "Send a market order (price and TIF cannot be specified).");
    marketOrderSubcommand->add_option(opt::SECURITY_ID, securityId, desc::SECURITY_ID)->required();
    marketOrderSubcommand->add_option(opt::SIDE, side, desc::SIDE)->required()->transform(sideValidator);
    marketOrderSubcommand->add_option(opt::QUANTITY, quantity, desc::QUANTITY)->required()->check(CLI::PositiveNumber);

    auto *cancelOrderSubcommand =
        orderSubcommand->add_subcommand("cancel", "Request to cancel a previously sent order.");
    cancelOrderSubcommand->add_option(opt::ORDER_ID, orderId, desc::ORDER_ID)->required()->check(uuidValidator);

    auto *amendOrderSubcommand = orderSubcommand->add_subcommand(
        "amend", "Request to modify a previously sent order by updating its price or quantity (or both).");
    amendOrderSubcommand->add_option(opt::ORDER_ID, orderId, desc::ORDER_ID)->required()->check(uuidValidator);

    auto *amendOrderSubcommandChangesGroup = amendOrderSubcommand->add_option_group("changes");
    auto *amendOrderQuantityOption =
        amendOrderSubcommandChangesGroup->add_option(opt::QUANTITY, quantity, desc::QUANTITY)
            ->check(CLI::PositiveNumber);
    auto *amendOrderPriceCentsOption =
        amendOrderSubcommandChangesGroup->add_option(opt::PRICE_CENTS, priceCents, desc::PRICE_CENTS)
            ->check(CLI::PositiveNumber);
    amendOrderSubcommandChangesGroup->require_option();

    auto *dataSubcommand = app.add_subcommand("data", "Receive different views of the current or historic market data.")
                               ->require_subcommand(1);

    auto *candlesDataSubcommand =
        dataSubcommand->add_subcommand("candles", "Receive a list of OHLCV candles for a given security at a certain "
                                                  "resolution, over an optionally specified time period.");
    candlesDataSubcommand->add_option(opt::SECURITY_ID, securityId, desc::SECURITY_ID)->required();
    candlesDataSubcommand->add_option(opt::RESOLUTION_SECONDS, resolutionSeconds, desc::RESOLUTION_SECONDS)
        ->required()
        ->check(CLI::PositiveNumber);
    candlesDataSubcommand->add_option(opt::START_TIME, startTime, desc::START_TIME)->check(timestampValidator);
    candlesDataSubcommand->add_option(opt::END_TIME, endTime, desc::END_TIME)->check(timestampValidator);

    auto *bookDataSubcommand =
        dataSubcommand
            ->add_subcommand("book", "Receive data about the current state of the orderbook for a given security.")
            ->require_subcommand(1);

    auto *l1BookDataSubcommand =
        bookDataSubcommand->add_subcommand("l1", "Receive L1 orderbook data (highest buy and lowest sell prices).");
    l1BookDataSubcommand->add_option(opt::SECURITY_ID, securityId, desc::SECURITY_ID)->required();

    auto *l2BookDataSubcommand =
        bookDataSubcommand->add_subcommand("l2", "Receive L2 orderbook data (price and quantity for the top N highest "
                                                 "buys and lowest sells). N defaults to the maximum, which is 10.");
    l2BookDataSubcommand->add_option(opt::SECURITY_ID, securityId, desc::SECURITY_ID)->required();
    l2BookDataSubcommand->add_option(opt::NUMBER_OF_LEVELS, numberOfLevels, desc::NUMBER_OF_LEVELS)
        ->check(CLI::Range{1, 10});

    auto *l3BookDataSubcommand = bookDataSubcommand->add_subcommand(
        "l3", "Receive L3 orderbook data, which consists of a stream of updates to the orderbook. Can be used to "
              "reconstruct the orderbook client-side.");
    l3BookDataSubcommand->add_option(opt::SECURITY_ID, securityId, desc::SECURITY_ID)->required();

    auto *orderDataSubcommand =
        dataSubcommand->add_subcommand("order", "Receive data about the status of a single order.");
    orderDataSubcommand->add_option(opt::ORDER_ID, orderId, desc::ORDER_ID)->required()->check(uuidValidator);

    auto *executionDataSubcommand =
        dataSubcommand->add_subcommand("exec", "Receive historical data about trades that occurred.");
    executionDataSubcommand->add_option(opt::SECURITY_ID, securityId, desc::SECURITY_ID);
    executionDataSubcommand->add_option(opt::ORDER_ID, orderId, desc::ORDER_ID)->check(uuidValidator);
    executionDataSubcommand->add_option(opt::START_TIME, startTime, desc::START_TIME)->check(timestampValidator);
    executionDataSubcommand->add_option(opt::END_TIME, endTime, desc::END_TIME)->check(timestampValidator);

    auto *securitySubcommand =
        app.add_subcommand("security", "Inspect details about the securities that are traded on this exchange.")
            ->require_subcommand(1);

    auto *listSecuritySubcommand =
        securitySubcommand->add_subcommand("list", "List all of the securities traded on this exchange.");

    auto *viewSecuritySubcommand = securitySubcommand->add_subcommand(
        "view", "View the details of a single security, given either its id or symbol.");

    auto *viewSecuritySubcommandLookupGroup = viewSecuritySubcommand->add_option_group("lookup");
    viewSecuritySubcommandLookupGroup->add_option(opt::SECURITY_ID, securityId, desc::SECURITY_ID);
    viewSecuritySubcommandLookupGroup->add_option(opt::SYMBOL, symbol, desc::SYMBOL);
    viewSecuritySubcommandLookupGroup->require_option(1);

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError &e)
    {
        return {.command = std::monostate{}, .exitCode = app.exit(e)};
    }

    if (limitOrderSubcommand->parsed())
    {
        return {
            .command = OrderLimitResult{
                .securityId = securityId, .priceCents = priceCents, .quantity = quantity, .side = side, .tif = tif}};
    }
    if (marketOrderSubcommand->parsed())
    {
        return {.command = OrderMarketResult{.securityId = securityId, .side = side, .quantity = quantity}};
    }
    if (cancelOrderSubcommand->parsed())
    {
        return {.command = OrderCancelResult{orderId}};
    }
    if (amendOrderSubcommand->parsed())
    {
        std::optional<uint64_t> priceCentsOptional;
        std::optional<uint64_t> quantityOptional;
        if (amendOrderPriceCentsOption->count() != 0)
        {
            priceCentsOptional = priceCents;
        }
        if (amendOrderQuantityOption->count() != 0)
        {
            quantityOptional = quantity;
        }
        return {.command = OrderAmendResult{
                    .orderId = orderId, .priceCents = priceCentsOptional, .quantity = quantityOptional}};
    }
    if (candlesDataSubcommand->parsed())
    {
        std::optional<std::string> startTimeOptional;
        std::optional<std::string> endTimeOptional;
        if (candlesDataSubcommand->count(opt::START_TIME) != 0)
        {
            startTimeOptional = startTime;
        }
        if (candlesDataSubcommand->count(opt::END_TIME) != 0)
        {
            endTimeOptional = endTime;
        }
        return {.command = DataCandlesResult{.securityId = securityId,
                                             .resolutionSeconds = resolutionSeconds,
                                             .startTime = startTimeOptional,
                                             .endTime = endTimeOptional}};
    }
    if (l1BookDataSubcommand->parsed())
    {
        return {.command = DataBookL1Result{securityId}};
    }
    if (l2BookDataSubcommand->parsed())
    {
        std::optional<uint32_t> numberOfLevelsOptional;
        if (l2BookDataSubcommand->count(opt::NUMBER_OF_LEVELS) != 0)
        {
            numberOfLevelsOptional = numberOfLevels;
        }
        return {.command = DataBookL2Result{.securityId = securityId, .numberOfLevels = numberOfLevelsOptional}};
    }
    if (l3BookDataSubcommand->parsed())
    {
        return {.command = DataBookL3Result{securityId}};
    }
    if (orderDataSubcommand->parsed())
    {
        return {.command = DataOrderResult{orderId}};
    }
    if (executionDataSubcommand->parsed())
    {
        std::optional<uint32_t> securityIdOptional;
        std::optional<std::string> orderIdOptional;
        std::optional<std::string> startTimeOptional;
        std::optional<std::string> endTimeOptional;
        if (executionDataSubcommand->count(opt::SECURITY_ID) != 0)
        {
            securityIdOptional = securityId;
        }
        if (!orderId.empty())
        {
            orderIdOptional = orderId;
        }
        if (executionDataSubcommand->count(opt::START_TIME) != 0)
        {
            startTimeOptional = startTime;
        }
        if (executionDataSubcommand->count(opt::END_TIME) != 0)
        {
            endTimeOptional = endTime;
        }
        return {.command = DataExecResult{.securityId = securityIdOptional,
                                          .orderId = orderIdOptional,
                                          .startTime = startTimeOptional,
                                          .endTime = endTimeOptional}};
    }
    if (listSecuritySubcommand->parsed())
    {
        return {.command = SecurityListResult{}};
    }
    if (viewSecuritySubcommand->parsed())
    {
        if (viewSecuritySubcommand->count(opt::SECURITY_ID) != 0)
        {
            return {.command = SecurityViewResult{securityId}};
        }
        return {.command = SecurityViewResult{symbol}};
    }
    return {.command = std::monostate{}, .exitCode = 1};
}

} // namespace exchange::client