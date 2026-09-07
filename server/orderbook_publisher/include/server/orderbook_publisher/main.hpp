#pragma once

namespace exchange::server
{
/**
 * The starting point of the Orderbook data publisher.
 *
 * @return The exit code of the program.
 */
int orderbookPublisherMain() noexcept;
} // namespace exchange::server