#include <boost/interprocess/managed_shared_memory.hpp>
#include <server/common/collections/ipc/shared_memory.hpp>

namespace exchange::server
{

void SharedMemory::deleteSegment() const
{
    boost::interprocess::shared_memory_object::remove(segmentName.c_str());
}

SharedMemory::SharedMemory(const bool isOwner, const std::string &segmentName)
    : isOwner(isOwner), segmentName(segmentName)
{
    if (isOwner)
    {
        deleteSegment();
        shm = boost::interprocess::managed_shared_memory(boost::interprocess::create_only, segmentName.c_str(),
                                                         segmentSize);
    }
    else
    {
        shm = boost::interprocess::managed_shared_memory(boost::interprocess::open_only, segmentName.c_str());
    }
}

boost::interprocess::managed_shared_memory &SharedMemory::get()
{
    if (shm.get_segment_manager() == nullptr)
        throw std::logic_error("Cannot access the shared memory before it is created.");
    return shm;
}

SharedMemory::~SharedMemory()
{
    if (isOwner)
    {
        deleteSegment();
    }
};

} // namespace exchange::server
