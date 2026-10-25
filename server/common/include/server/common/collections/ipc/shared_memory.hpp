#pragma once
#include <boost/interprocess/managed_shared_memory.hpp>

namespace exchange::server
{

/**
 * Light wrapper around managing a shared memory segment.
 */
class SharedMemory
{
    boost::interprocess::managed_shared_memory shm;

    static constexpr auto segmentName{"SharedMemory"};
    static constexpr size_t segmentSize = 64 * 1024;

    static void deleteSegment();

  public:
    static constexpr auto spmcName{"SpmcQueue"};
    static constexpr auto mpscName{"MpscQueue"};

    /**
     * Constructor for the shared memory object.
     *
     * @param isOwner Whether the caller manages the lifecycle of the shared memory object.
     * The first call in the application should have isOwner = true.
     */
    explicit SharedMemory(bool isOwner);

    /**
     * Getter for the raw Boost shared memory object.
     * @return A reference to the raw Boost shared memory object.
     *
     * @throws std::logic_error If called before the segment is created.
     */
    boost::interprocess::managed_shared_memory &get();

    /**
     * RAII for destroying the shared memory segment.
     */
    class Deleter
    {
      public:
        ~Deleter();
    };
};

} // namespace exchange::server
