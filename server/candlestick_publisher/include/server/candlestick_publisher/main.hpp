#pragma once

namespace exchange::server
{
/**
 * The starting point of the exchange candlestick data publisher.
 *
 * @return The exit code of the program.
 */
int candlestickPublisherMain() noexcept;
} // namespace exchange::server