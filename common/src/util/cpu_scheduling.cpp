#include <algorithm>
#include <cstdint>
#include <optional>
#include <thread>
#ifdef _WIN32
#include <limits>
#include <windows.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#elif defined(__APPLE__)
#include <pthread.h>
#include <mach/mach.h>
#include <mach/thread_policy.h>
#endif

#include "common/util/cpu_scheduling.hpp"

namespace exchange::common
{

constexpr uint32_t PREFERRED_CORE = 2;

template <typename IsAllowed> static std::optional<uint32_t> chooseCore(const uint32_t cpuCount, IsAllowed isAllowed)
{
    std::optional<uint32_t> lastAllowed;
    for (uint32_t cpu = 0; cpu < cpuCount; ++cpu)
    {
        if (!isAllowed(cpu))
            continue;
        if (cpu >= PREFERRED_CORE)
            return cpu;
        lastAllowed = cpu;
    }
    return lastAllowed;
}

bool pinToCore() noexcept
{
#ifdef _WIN32
    DWORD_PTR processMask = 0;
    DWORD_PTR systemMask = 0;
    if (GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask) == 0)
        return false;
    const auto core = chooseCore(std::numeric_limits<DWORD_PTR>::digits,
                                 [processMask](const uint32_t cpu) { return (processMask >> cpu & 1U) != 0; });
    if (!core)
        return false;
    return SetThreadAffinityMask(GetCurrentThread(), static_cast<DWORD_PTR>(1) << *core) != 0;
#elif defined(__linux__)
    cpu_set_t allowed;
    CPU_ZERO(&allowed);
    if (pthread_getaffinity_np(pthread_self(), sizeof(allowed), &allowed) != 0)
        return false;
    const auto core = chooseCore(CPU_SETSIZE, [&allowed](const uint32_t cpu) { return CPU_ISSET(cpu, &allowed) != 0; });
    if (!core)
        return false;
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(*core, &set);
    return pthread_setaffinity_np(pthread_self(), sizeof(set), &set) == 0;
#elif defined(__APPLE__)
    const auto core =
        chooseCore(std::max(1U, std::thread::hardware_concurrency()), [](const uint32_t) { return true; });
    thread_affinity_policy_data_t policy = {static_cast<int>(core.value_or(0)) + 1};
    return thread_policy_set(pthread_mach_thread_np(pthread_self()), THREAD_AFFINITY_POLICY, &policy.affinity_tag,
                             THREAD_AFFINITY_POLICY_COUNT) == KERN_SUCCESS;
#else
    return false;
#endif
}

} // namespace exchange::common
