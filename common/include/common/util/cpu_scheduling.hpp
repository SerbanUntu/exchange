#pragma once

namespace exchange::common
{

/**
 * Attempts to pin the current thread to a single CPU core by changing the CPU affinity settings.
 * It picks the first core the process is allowed to run on that is at least core 2, which is likely to be a different
 * physical core than 0/1, which are often used for interrupts. If only cores 0/1 are allowed, it picks the highest one.
 *
 * @return True if pinning succeeded, false otherwise.
 */
bool pinToCore() noexcept;

} // namespace exchange::common