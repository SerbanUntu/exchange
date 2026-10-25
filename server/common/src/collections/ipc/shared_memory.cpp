#include <boost/interprocess/managed_shared_memory.hpp>
#include <server/common/collections/ipc/shared_memory.hpp>

namespace exchange::server
{

void SharedMemory::deleteSegment()
{
    boost::interprocess::shared_memory_object::remove(segmentName);
}

SharedMemory::SharedMemory(const bool isOwner)
{
    if (isOwner)
    {
        deleteSegment();
        shm = boost::interprocess::managed_shared_memory(boost::interprocess::create_only, segmentName, segmentSize);
    }
    else
    {
        shm = boost::interprocess::managed_shared_memory(boost::interprocess::open_only, segmentName);
    }
}

boost::interprocess::managed_shared_memory &SharedMemory::get()
{
    if (shm.get_segment_manager() == nullptr)
        throw std::logic_error("Cannot access the shared memory before it is created.");
    return shm;
}

SharedMemory::Deleter::~Deleter()
{
    deleteSegment();
};

} // namespace exchange::server
