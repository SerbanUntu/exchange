#pragma once
#include <boost/interprocess/managed_shared_memory.hpp>
#include <concurrent/aeron_broadcast_descriptor.h>
#include <concurrent/aeron_rb.h>

#include <string>

namespace exchange::server
{

/**
 * Light wrapper around managing a shared memory segment.
 */
class SharedMemory
{
    boost::interprocess::managed_shared_memory shm;

    static constexpr auto defaultSegmentName{"SharedMemory"};
    static constexpr size_t segmentSize = size_t{1024} * 1024;

    bool isOwner;
    std::string segmentName;

    void deleteSegment() const;

  public:
    static constexpr auto spmcName{"SpmcQueue"};
    static constexpr auto mpscName{"MpscQueue"};
    static constexpr size_t spmcCapacity = size_t{256} * 1024;
    static constexpr size_t spmcTotalObjectSize = spmcCapacity + AERON_BROADCAST_BUFFER_TRAILER_LENGTH;
    static constexpr size_t mpscCapacity = size_t{256} * 1024;
    static constexpr size_t mpscTotalObjectSize = mpscCapacity + AERON_RB_TRAILER_LENGTH;

    /**
     * Constructor for the shared memory object.
     *
     * @param isOwner Whether the caller manages the lifecycle of the shared memory object.
     * The first call in the application should have isOwner = true.
     * @param segmentName The name of the shared memory segment. Should only be set to a different value when testing.
     */
    explicit SharedMemory(bool isOwner, const std::string &segmentName = defaultSegmentName);

    ~SharedMemory();

    /**
     * Getter for the raw Boost shared memory object.
     * @return A reference to the raw Boost shared memory object.
     *
     * @throws std::logic_error If called before the segment is created.
     */
    boost::interprocess::managed_shared_memory &get();
};

} // namespace exchange::server
