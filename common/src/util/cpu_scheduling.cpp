#include <algorithm>
#include <cstdint>
#include <thread>
#ifdef _WIN32
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

static uint32_t chooseCore()
{
    const uint32_t availableCores = std::max(1U, std::thread::hardware_concurrency());
    return std::min(2U, availableCores - 1);
}

bool pinToCore() noexcept
{
    const auto core = chooseCore();
#ifdef _WIN32
    return SetThreadAffinityMask(GetCurrentThread(), static_cast<DWORD_PTR>(1) << core) != 0;
#elif defined(__linux__)
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    return pthread_setaffinity_np(pthread_self(), sizeof(set), &set) == 0;
#elif defined(__APPLE__)
    thread_affinity_policy_data_t policy = {static_cast<int>(core) + 1};
    return thread_policy_set(pthread_mach_thread_np(pthread_self()), THREAD_AFFINITY_POLICY,
                             static_cast<thread_policy_t>(&policy), THREAD_AFFINITY_POLICY_COUNT) == KERN_SUCCESS;
#else
    return false;
#endif
}

} // namespace exchange::common