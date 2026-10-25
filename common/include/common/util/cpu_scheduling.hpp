#pragma once

namespace exchange::common
{

/**
 * Attempts to pin the current thread to a single CPU core by changing the CPU affinity settings.
 * It prefers core 2, which is guaranteed to be a different physical core than 0/1, which is often used for interrupts.
 *
 * @return True if pinning succeeded, false otherwise.
 */
bool pinToCore() noexcept;

} // namespace exchange::common