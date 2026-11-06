#include "client/cli_parsing.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace exchange::client
{
namespace
{
const std::string DEFAULT_SERVER_URL = "localhost:8989";
const std::string ORDER_ID = "123e4567-e89b-12d3-a456-426614174000";

struct ParseOutcome
{
    ParseCommandResult result;
    std::string serverUrl;
};

ParseOutcome parse(std::vector<std::string> args)
{
    args.insert(args.begin(), "exchange");
    std::vector<char *> argv;
    argv.reserve(args.size() + 1);
    for (auto &arg : args)
    {
        argv.push_back(arg.data());
    }
    argv.push_back(nullptr);

    ParseOutcome outcome{.result = {}, .serverUrl = DEFAULT_SERVER_URL};
    outcome.result = parseCommand(static_cast<int>(args.size()), argv.data(), outcome.serverUrl);
    return outcome;
}

bool isRejected(const std::vector<std::string> &args)
{
    const auto [result, serverUrl] = parse(args);
    return std::holds_alternative<std::monostate>(result.command) && result.exitCode != 0;
}

std::vector<std::string> limitArgs(const std::string &side, const std::string &tif)
{
    return {"order", "limit", "--security", "1", "--side", side, "-q", "10", "-p", "100", "--tif", tif};
}
} // namespace

TEST(CliParsingTest, NoArgumentsIsRejected)
{
    EXPECT_TRUE(isRejected({}));
}

TEST(CliParsingTest, UnknownSubcommandIsRejected)
{
    EXPECT_TRUE(isRejected({"trade"}));
}

TEST(CliParsingTest, ParentSubcommandsWithoutALeafSubcommandAreRejected)
{
    EXPECT_TRUE(isRejected({"order"}));
    EXPECT_TRUE(isRejected({"data"}));
    EXPECT_TRUE(isRejected({"data", "book"}));
    EXPECT_TRUE(isRejected({"security"}));
}

TEST(CliParsingTest, HelpReturnsNoCommandWithASuccessfulExitCode)
{
    const auto [result, serverUrl] = parse({"--help"});

    EXPECT_TRUE(std::holds_alternative<std::monostate>(result.command));
    EXPECT_EQ(result.exitCode, 0);
}

TEST(CliParsingTest, HelpOnALeafSubcommandReturnsNoCommandWithASuccessfulExitCode)
{
    const auto [result, serverUrl] = parse({"order", "limit", "--help"});

    EXPECT_TRUE(std::holds_alternative<std::monostate>(result.command));
    EXPECT_EQ(result.exitCode, 0);
}

TEST(CliParsingTest, ServerUrlKeepsItsDefaultWhenNotSpecified)
{
    const auto [result, serverUrl] = parse({"security", "list"});

    EXPECT_EQ(serverUrl, DEFAULT_SERVER_URL);
}

TEST(CliParsingTest, ServerUrlIsOverriddenBeforeTheSubcommand)
{
    const auto [result, serverUrl] = parse({"--server", "example.com:1234", "security", "list"});

    EXPECT_TRUE(std::holds_alternative<SecurityListResult>(result.command));
    EXPECT_EQ(serverUrl, "example.com:1234");
}

TEST(CliParsingTest, ServerUrlFallsThroughFromANestedSubcommand)
{
    const auto [result, serverUrl] = parse({"data", "book", "l1", "--security", "2", "--server", "example.com:1234"});

    EXPECT_TRUE(std::holds_alternative<DataBookL1Result>(result.command));
    EXPECT_EQ(serverUrl, "example.com:1234");
}

TEST(CliParsingTest, ServerUrlWithoutAValueIsRejected)
{
    EXPECT_TRUE(isRejected({"security", "list", "--server"}));
}

TEST(CliParsingTest, OrderLimitParsesAllFields)
{
    const auto [result, serverUrl] = parse({"order", "limit", "--security", "3", "--side", "sell", "--quantity", "25",
                                            "--price-cents", "1999", "--tif", "fok"});

    const auto *command = std::get_if<OrderLimitResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->securityId, 3U);
    EXPECT_EQ(command->side, proto::Side::SIDE_SELL);
    EXPECT_EQ(command->quantity, 25U);
    EXPECT_EQ(command->priceCents, 1999U);
    EXPECT_EQ(command->tif, proto::TimeInForce::TIME_IN_FORCE_FOK);
    EXPECT_EQ(result.exitCode, 0);
}

TEST(CliParsingTest, OrderLimitAcceptsShortAndEqualsSyntax)
{
    const auto [result, serverUrl] =
        parse({"order", "limit", "--security=1", "--side=buy", "-q", "7", "-p8", "--tif=gtc"});

    const auto *command = std::get_if<OrderLimitResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->quantity, 7U);
    EXPECT_EQ(command->priceCents, 8U);
}

TEST(CliParsingTest, OrderLimitAcceptsOptionsInAnyOrder)
{
    const auto [result, serverUrl] =
        parse({"order", "limit", "--tif", "ioc", "-p", "5", "-q", "6", "--side", "buy", "--security", "2"});

    const auto *command = std::get_if<OrderLimitResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->securityId, 2U);
    EXPECT_EQ(command->side, proto::Side::SIDE_BUY);
    EXPECT_EQ(command->quantity, 6U);
    EXPECT_EQ(command->priceCents, 5U);
    EXPECT_EQ(command->tif, proto::TimeInForce::TIME_IN_FORCE_IOC);
}

TEST(CliParsingTest, OrderLimitSideIsCaseInsensitive)
{
    for (const auto &[input, expected] : std::vector<std::pair<std::string, proto::Side>>{
             {"buy", proto::Side::SIDE_BUY},
             {"BUY", proto::Side::SIDE_BUY},
             {"Sell", proto::Side::SIDE_SELL},
             {"sElL", proto::Side::SIDE_SELL},
         })
    {
        const auto [result, serverUrl] = parse(limitArgs(input, "gtc"));

        const auto *command = std::get_if<OrderLimitResult>(&result.command);
        ASSERT_NE(command, nullptr) << input;
        EXPECT_EQ(command->side, expected) << input;
    }
}

TEST(CliParsingTest, OrderLimitTifIsCaseInsensitive)
{
    for (const auto &[input, expected] : std::vector<std::pair<std::string, proto::TimeInForce>>{
             {"gtc", proto::TimeInForce::TIME_IN_FORCE_GTC},
             {"GTC", proto::TimeInForce::TIME_IN_FORCE_GTC},
             {"IoC", proto::TimeInForce::TIME_IN_FORCE_IOC},
             {"FOK", proto::TimeInForce::TIME_IN_FORCE_FOK},
         })
    {
        const auto [result, serverUrl] = parse(limitArgs("buy", input));

        const auto *command = std::get_if<OrderLimitResult>(&result.command);
        ASSERT_NE(command, nullptr) << input;
        EXPECT_EQ(command->tif, expected) << input;
    }
}

TEST(CliParsingTest, OrderLimitRejectsUnknownSides)
{
    EXPECT_TRUE(isRejected(limitArgs("hold", "gtc")));
    EXPECT_TRUE(isRejected(limitArgs("", "gtc")));
    EXPECT_TRUE(isRejected(limitArgs("buyy", "gtc")));
    EXPECT_TRUE(isRejected(limitArgs("1", "gtc")));
    EXPECT_TRUE(isRejected(limitArgs("2", "gtc")));
}

TEST(CliParsingTest, OrderLimitRejectsUnknownTifs)
{
    EXPECT_TRUE(isRejected(limitArgs("buy", "day")));
    EXPECT_TRUE(isRejected(limitArgs("buy", "")));
    EXPECT_TRUE(isRejected(limitArgs("buy", "gtcc")));
    EXPECT_TRUE(isRejected(limitArgs("buy", "1")));
    EXPECT_TRUE(isRejected(limitArgs("buy", "3")));
}

TEST(CliParsingTest, OrderLimitRejectsEachMissingRequiredOption)
{
    EXPECT_TRUE(isRejected({"order", "limit", "--side", "buy", "-q", "10", "-p", "100", "--tif", "gtc"}));
    EXPECT_TRUE(isRejected({"order", "limit", "--security", "1", "-q", "10", "-p", "100", "--tif", "gtc"}));
    EXPECT_TRUE(isRejected({"order", "limit", "--security", "1", "--side", "buy", "-p", "100", "--tif", "gtc"}));
    EXPECT_TRUE(isRejected({"order", "limit", "--security", "1", "--side", "buy", "-q", "10", "--tif", "gtc"}));
    EXPECT_TRUE(isRejected({"order", "limit", "--security", "1", "--side", "buy", "-q", "10", "-p", "100"}));
}

TEST(CliParsingTest, OrderLimitRejectsAnOptionWithoutAValue)
{
    EXPECT_TRUE(isRejected({"order", "limit", "--security", "1", "--side", "buy", "-q", "10", "-p", "100", "--tif"}));
}

TEST(CliParsingTest, OrderLimitRejectsNonPositiveQuantities)
{
    EXPECT_TRUE(
        isRejected({"order", "limit", "--security", "1", "--side", "buy", "-q", "0", "-p", "100", "--tif", "gtc"}));
    EXPECT_TRUE(
        isRejected({"order", "limit", "--security", "1", "--side", "buy", "-q", "-5", "-p", "100", "--tif", "gtc"}));
}

TEST(CliParsingTest, OrderLimitRejectsNonPositivePrices)
{
    EXPECT_TRUE(
        isRejected({"order", "limit", "--security", "1", "--side", "buy", "-q", "10", "-p", "0", "--tif", "gtc"}));
    EXPECT_TRUE(
        isRejected({"order", "limit", "--security", "1", "--side", "buy", "-q", "10", "-p", "-1", "--tif", "gtc"}));
}

TEST(CliParsingTest, OrderLimitRejectsNonIntegralNumbers)
{
    EXPECT_TRUE(
        isRejected({"order", "limit", "--security", "1", "--side", "buy", "-q", "abc", "-p", "100", "--tif", "gtc"}));
    EXPECT_TRUE(
        isRejected({"order", "limit", "--security", "1", "--side", "buy", "-q", "10", "-p", "1.5", "--tif", "gtc"}));
    EXPECT_TRUE(
        isRejected({"order", "limit", "--security", "one", "--side", "buy", "-q", "10", "-p", "100", "--tif", "gtc"}));
}

TEST(CliParsingTest, OrderLimitAcceptsTheMaximumUnsignedValues)
{
    const auto [result, serverUrl] = parse({"order", "limit", "--security", "4294967295", "--side", "buy", "-q",
                                            "18446744073709551615", "-p", "18446744073709551615", "--tif", "gtc"});

    const auto *command = std::get_if<OrderLimitResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->securityId, 4294967295U);
    EXPECT_EQ(command->quantity, 18446744073709551615U);
    EXPECT_EQ(command->priceCents, 18446744073709551615U);
}

TEST(CliParsingTest, OrderLimitRejectsValuesThatOverflowTheirType)
{
    EXPECT_TRUE(isRejected(
        {"order", "limit", "--security", "4294967296", "--side", "buy", "-q", "10", "-p", "100", "--tif", "gtc"}));
    EXPECT_TRUE(isRejected({"order", "limit", "--security", "1", "--side", "buy", "-q", "18446744073709551616", "-p",
                            "100", "--tif", "gtc"}));
}

TEST(CliParsingTest, OrderLimitRejectsARepeatedOption)
{
    EXPECT_TRUE(isRejected({"order", "limit", "--security", "1", "--side", "buy", "--side", "sell", "-q", "10", "-p",
                            "100", "--tif", "gtc"}));
}

TEST(CliParsingTest, OrderLimitRejectsUnexpectedPositionalArguments)
{
    auto args = limitArgs("buy", "gtc");
    args.emplace_back("extra");

    EXPECT_TRUE(isRejected(args));
}

TEST(CliParsingTest, OrderLimitRejectsOptionsOfOtherSubcommands)
{
    auto args = limitArgs("buy", "gtc");
    args.insert(args.end(), {"--order", ORDER_ID});

    EXPECT_TRUE(isRejected(args));
}

TEST(CliParsingTest, OrderLimitFollowedByASiblingSubcommandIsRejected)
{
    auto args = limitArgs("buy", "gtc");
    args.insert(args.end(), {"cancel", "--order", ORDER_ID});

    EXPECT_TRUE(isRejected(args));
}

TEST(CliParsingTest, OrderMarketParsesAllFields)
{
    const auto [result, serverUrl] = parse({"order", "market", "--security", "2", "--side", "BUY", "-q", "40"});

    const auto *command = std::get_if<OrderMarketResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->securityId, 2U);
    EXPECT_EQ(command->side, proto::Side::SIDE_BUY);
    EXPECT_EQ(command->quantity, 40U);
}

TEST(CliParsingTest, OrderMarketRejectsAPrice)
{
    EXPECT_TRUE(isRejected({"order", "market", "--security", "2", "--side", "buy", "-q", "40", "-p", "100"}));
}

TEST(CliParsingTest, OrderMarketRejectsATif)
{
    EXPECT_TRUE(isRejected({"order", "market", "--security", "2", "--side", "buy", "-q", "40", "--tif", "ioc"}));
}

TEST(CliParsingTest, OrderMarketRejectsAZeroQuantity)
{
    EXPECT_TRUE(isRejected({"order", "market", "--security", "2", "--side", "buy", "-q", "0"}));
}

TEST(CliParsingTest, OrderMarketRejectsMissingRequiredOptions)
{
    EXPECT_TRUE(isRejected({"order", "market", "--side", "buy", "-q", "40"}));
    EXPECT_TRUE(isRejected({"order", "market", "--security", "2", "-q", "40"}));
    EXPECT_TRUE(isRejected({"order", "market", "--security", "2", "--side", "buy"}));
}

TEST(CliParsingTest, OrderCancelParsesTheOrderId)
{
    const auto [result, serverUrl] = parse({"order", "cancel", "-o", ORDER_ID});

    const auto *command = std::get_if<OrderCancelResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->orderId, ORDER_ID);
}

TEST(CliParsingTest, OrderCancelAcceptsUppercaseAndMixedCaseUuids)
{
    for (const std::string orderId : {"123E4567-E89B-12D3-A456-426614174000", "aBcDeF01-2345-6789-AbCd-eF0123456789"})
    {
        const auto [result, serverUrl] = parse({"order", "cancel", "--order", orderId});

        const auto *command = std::get_if<OrderCancelResult>(&result.command);
        ASSERT_NE(command, nullptr) << orderId;
        EXPECT_EQ(command->orderId, orderId);
    }
}

TEST(CliParsingTest, OrderCancelRejectsMalformedUuids)
{
    for (const std::string orderId : {
             "",
             "123e4567e89b12d3a456426614174000",
             "123e4567-e89b-12d3-a456-42661417400",
             "123e4567-e89b-12d3-a456-4266141740000",
             "123e4567-e89b-12d3-a456-42661417400g",
             "{123e4567-e89b-12d3-a456-426614174000}",
             " 123e4567-e89b-12d3-a456-426614174000",
             "123e4567-e89b-12d3-a456-426614174000 ",
             "123e4567_e89b_12d3_a456_426614174000",
             "123e456-7e89b-12d3-a456-426614174000",
         })
    {
        EXPECT_TRUE(isRejected({"order", "cancel", "--order", orderId})) << orderId;
    }
}

TEST(CliParsingTest, OrderCancelRejectsAMissingOrderId)
{
    EXPECT_TRUE(isRejected({"order", "cancel"}));
}

TEST(CliParsingTest, OrderAmendWithOnlyAQuantityLeavesThePriceEmpty)
{
    const auto [result, serverUrl] = parse({"order", "amend", "-o", ORDER_ID, "-q", "15"});

    const auto *command = std::get_if<OrderAmendResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->orderId, ORDER_ID);
    EXPECT_EQ(command->quantity, 15U);
    EXPECT_FALSE(command->priceCents.has_value());
}

TEST(CliParsingTest, OrderAmendWithOnlyAPriceLeavesTheQuantityEmpty)
{
    const auto [result, serverUrl] = parse({"order", "amend", "-o", ORDER_ID, "-p", "250"});

    const auto *command = std::get_if<OrderAmendResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->priceCents, 250U);
    EXPECT_FALSE(command->quantity.has_value());
}

TEST(CliParsingTest, OrderAmendWithBothChangesParsesBoth)
{
    const auto [result, serverUrl] = parse({"order", "amend", "-p", "250", "-q", "15", "-o", ORDER_ID});

    const auto *command = std::get_if<OrderAmendResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->orderId, ORDER_ID);
    EXPECT_EQ(command->priceCents, 250U);
    EXPECT_EQ(command->quantity, 15U);
}

TEST(CliParsingTest, OrderAmendWithoutAnyChangeIsRejected)
{
    EXPECT_TRUE(isRejected({"order", "amend", "-o", ORDER_ID}));
}

TEST(CliParsingTest, OrderAmendWithoutAnOrderIdIsRejected)
{
    EXPECT_TRUE(isRejected({"order", "amend", "-q", "15"}));
}

TEST(CliParsingTest, OrderAmendRejectsNonPositiveChanges)
{
    EXPECT_TRUE(isRejected({"order", "amend", "-o", ORDER_ID, "-q", "0"}));
    EXPECT_TRUE(isRejected({"order", "amend", "-o", ORDER_ID, "-p", "0"}));
    EXPECT_TRUE(isRejected({"order", "amend", "-o", ORDER_ID, "-q", "5", "-p", "0"}));
}

TEST(CliParsingTest, OrderAmendRejectsAMalformedOrderId)
{
    EXPECT_TRUE(isRejected({"order", "amend", "-o", "not-a-uuid", "-q", "15"}));
}

TEST(CliParsingTest, DataCandlesWithoutTimesLeavesBothEmpty)
{
    const auto [result, serverUrl] = parse({"data", "candles", "--security", "1", "--resolution-seconds", "60"});

    const auto *command = std::get_if<DataCandlesResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->securityId, 1U);
    EXPECT_EQ(command->resolutionSeconds, 60U);
    EXPECT_FALSE(command->startTime.has_value());
    EXPECT_FALSE(command->endTime.has_value());
}

TEST(CliParsingTest, DataCandlesWithOnlyAStartLeavesTheEndEmpty)
{
    const auto [result, serverUrl] =
        parse({"data", "candles", "--security", "1", "--resolution-seconds", "60", "--start", "2026-11-04T09:30:00Z"});

    const auto *command = std::get_if<DataCandlesResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->startTime, "2026-11-04T09:30:00Z");
    EXPECT_FALSE(command->endTime.has_value());
}

TEST(CliParsingTest, DataCandlesWithOnlyAnEndLeavesTheStartEmpty)
{
    const auto [result, serverUrl] =
        parse({"data", "candles", "--security", "1", "--resolution-seconds", "60", "--end", "2026-11-04T16:00:00Z"});

    const auto *command = std::get_if<DataCandlesResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_FALSE(command->startTime.has_value());
    EXPECT_EQ(command->endTime, "2026-11-04T16:00:00Z");
}

TEST(CliParsingTest, DataCandlesKeepsTimestampsVerbatim)
{
    const auto [result, serverUrl] =
        parse({"data", "candles", "--security", "1", "--resolution-seconds", "60", "--start",
               "2026-11-04T09:30:00.123456789+02:00", "--end", "2026-11-04T16:00:00.5-05:30"});

    const auto *command = std::get_if<DataCandlesResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->startTime, "2026-11-04T09:30:00.123456789+02:00");
    EXPECT_EQ(command->endTime, "2026-11-04T16:00:00.5-05:30");
}

TEST(CliParsingTest, DataCandlesRejectsMalformedTimestamps)
{
    for (const std::string timestamp : {
             "",
             "2026-11-04",
             "2026-11-04T09:30:00",
             "2026-11-04 09:30:00Z",
             "2026-13-04T09:30:00Z",
             "2026-11-32T09:30:00Z",
             "2026-11-04T25:30:00Z",
             "2026-11-04T09:30:00+2:00",
             "1730712600",
             "yesterday",
         })
    {
        EXPECT_TRUE(
            isRejected({"data", "candles", "--security", "1", "--resolution-seconds", "60", "--start", timestamp}))
            << timestamp;
        EXPECT_TRUE(
            isRejected({"data", "candles", "--security", "1", "--resolution-seconds", "60", "--end", timestamp}))
            << timestamp;
    }
}

TEST(CliParsingTest, DataCandlesRejectsAZeroResolution)
{
    EXPECT_TRUE(isRejected({"data", "candles", "--security", "1", "--resolution-seconds", "0"}));
}

TEST(CliParsingTest, DataCandlesRejectsMissingRequiredOptions)
{
    EXPECT_TRUE(isRejected({"data", "candles", "--security", "1"}));
    EXPECT_TRUE(isRejected({"data", "candles", "--resolution-seconds", "60"}));
}

TEST(CliParsingTest, DataBookL1ParsesTheSecurity)
{
    const auto [result, serverUrl] = parse({"data", "book", "l1", "--security", "3"});

    const auto *command = std::get_if<DataBookL1Result>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->securityId, 3U);
}

TEST(CliParsingTest, DataBookL2WithoutLevelsLeavesThemEmpty)
{
    const auto [result, serverUrl] = parse({"data", "book", "l2", "--security", "3"});

    const auto *command = std::get_if<DataBookL2Result>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->securityId, 3U);
    EXPECT_FALSE(command->numberOfLevels.has_value());
}

TEST(CliParsingTest, DataBookL2AcceptsTheBoundaryLevels)
{
    for (const uint32_t levels : {1U, 10U})
    {
        const auto [result, serverUrl] =
            parse({"data", "book", "l2", "--security", "3", "--number-of-levels", std::to_string(levels)});

        const auto *command = std::get_if<DataBookL2Result>(&result.command);
        ASSERT_NE(command, nullptr) << levels;
        EXPECT_EQ(command->numberOfLevels, levels);
    }
}

TEST(CliParsingTest, DataBookL2RejectsLevelsOutsideTheRange)
{
    EXPECT_TRUE(isRejected({"data", "book", "l2", "--security", "3", "--number-of-levels", "0"}));
    EXPECT_TRUE(isRejected({"data", "book", "l2", "--security", "3", "--number-of-levels", "11"}));
    EXPECT_TRUE(isRejected({"data", "book", "l2", "--security", "3", "--number-of-levels", "-1"}));
}

TEST(CliParsingTest, DataBookL3ParsesTheSecurity)
{
    const auto [result, serverUrl] = parse({"data", "book", "l3", "--security", "1"});

    const auto *command = std::get_if<DataBookL3Result>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->securityId, 1U);
}

TEST(CliParsingTest, DataBookLevelsRejectAMissingSecurity)
{
    EXPECT_TRUE(isRejected({"data", "book", "l1"}));
    EXPECT_TRUE(isRejected({"data", "book", "l2"}));
    EXPECT_TRUE(isRejected({"data", "book", "l3"}));
}

TEST(CliParsingTest, DataBookL1RejectsTheL2LevelsOption)
{
    EXPECT_TRUE(isRejected({"data", "book", "l1", "--security", "1", "--number-of-levels", "5"}));
}

TEST(CliParsingTest, DataOrderIsDistinctFromTheOrderSubcommand)
{
    const auto [result, serverUrl] = parse({"data", "order", "--order", ORDER_ID});

    const auto *command = std::get_if<DataOrderResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->orderId, ORDER_ID);
}

TEST(CliParsingTest, DataOrderRejectsAMissingOrMalformedOrderId)
{
    EXPECT_TRUE(isRejected({"data", "order"}));
    EXPECT_TRUE(isRejected({"data", "order", "--order", "123"}));
}

TEST(CliParsingTest, DataExecWithoutFiltersLeavesAllEmpty)
{
    const auto [result, serverUrl] = parse({"data", "exec"});

    const auto *command = std::get_if<DataExecResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_FALSE(command->securityId.has_value());
    EXPECT_FALSE(command->orderId.has_value());
    EXPECT_FALSE(command->startTime.has_value());
    EXPECT_FALSE(command->endTime.has_value());
}

TEST(CliParsingTest, DataExecParsesAllFilters)
{
    const auto [result, serverUrl] = parse({"data", "exec", "--security", "2", "--order", ORDER_ID, "--start",
                                            "2026-11-04T09:30:00Z", "--end", "2026-11-04T16:00:00Z"});

    const auto *command = std::get_if<DataExecResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->securityId, 2U);
    EXPECT_EQ(command->orderId, ORDER_ID);
    EXPECT_EQ(command->startTime, "2026-11-04T09:30:00Z");
    EXPECT_EQ(command->endTime, "2026-11-04T16:00:00Z");
}

TEST(CliParsingTest, DataExecWithASecurityIdOfZeroStillSetsTheFilter)
{
    const auto [result, serverUrl] = parse({"data", "exec", "--security", "0"});

    const auto *command = std::get_if<DataExecResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_EQ(command->securityId, 0U);
}

TEST(CliParsingTest, DataExecWithOnlyAnOrderIdLeavesTheOtherFiltersEmpty)
{
    const auto [result, serverUrl] = parse({"data", "exec", "-o", ORDER_ID});

    const auto *command = std::get_if<DataExecResult>(&result.command);
    ASSERT_NE(command, nullptr);
    EXPECT_FALSE(command->securityId.has_value());
    EXPECT_EQ(command->orderId, ORDER_ID);
    EXPECT_FALSE(command->startTime.has_value());
    EXPECT_FALSE(command->endTime.has_value());
}

TEST(CliParsingTest, DataExecRejectsMalformedFilters)
{
    EXPECT_TRUE(isRejected({"data", "exec", "--order", ""}));
    EXPECT_TRUE(isRejected({"data", "exec", "--order", "not-a-uuid"}));
    EXPECT_TRUE(isRejected({"data", "exec", "--start", "2026-11-04"}));
    EXPECT_TRUE(isRejected({"data", "exec", "--end", "noon"}));
    EXPECT_TRUE(isRejected({"data", "exec", "--security", "-1"}));
}

TEST(CliParsingTest, SecurityListParses)
{
    const auto [result, serverUrl] = parse({"security", "list"});

    EXPECT_TRUE(std::holds_alternative<SecurityListResult>(result.command));
    EXPECT_EQ(result.exitCode, 0);
}

TEST(CliParsingTest, SecurityListRejectsUnexpectedArguments)
{
    EXPECT_TRUE(isRejected({"security", "list", "--security", "1"}));
    EXPECT_TRUE(isRejected({"security", "list", "extra"}));
}

TEST(CliParsingTest, SecurityViewByIdParsesTheId)
{
    const auto [result, serverUrl] = parse({"security", "view", "--security", "2"});

    const auto *command = std::get_if<SecurityViewResult>(&result.command);
    ASSERT_NE(command, nullptr);
    const auto *securityId = std::get_if<SecurityViewResult::SecurityId>(&command->key);
    ASSERT_NE(securityId, nullptr);
    EXPECT_EQ(*securityId, 2U);
}

TEST(CliParsingTest, SecurityViewBySymbolParsesTheSymbol)
{
    const auto [result, serverUrl] = parse({"security", "view", "--symbol", "AAPL"});

    const auto *command = std::get_if<SecurityViewResult>(&result.command);
    ASSERT_NE(command, nullptr);
    const auto *symbol = std::get_if<SecurityViewResult::Symbol>(&command->key);
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(*symbol, "AAPL");
}

TEST(CliParsingTest, SecurityViewBySymbolKeepsANumericSymbolAsASymbol)
{
    const auto [result, serverUrl] = parse({"security", "view", "--symbol", "123"});

    const auto *command = std::get_if<SecurityViewResult>(&result.command);
    ASSERT_NE(command, nullptr);
    const auto *symbol = std::get_if<SecurityViewResult::Symbol>(&command->key);
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(*symbol, "123");
}

TEST(CliParsingTest, SecurityViewRequiresExactlyOneLookupKey)
{
    EXPECT_TRUE(isRejected({"security", "view"}));
    EXPECT_TRUE(isRejected({"security", "view", "--security", "2", "--symbol", "AAPL"}));
}

} // namespace exchange::client
