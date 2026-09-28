#include "PhysicsRuntime.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

#include <mutex>

namespace Playground::Physics
{
namespace
{
std::mutex runtimeMutex;
unsigned runtimeUsers{};
}
RuntimeLease::RuntimeLease()
{
    std::lock_guard lock(runtimeMutex);
    if (runtimeUsers++==0)
    {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance=new JPH::Factory();
        JPH::RegisterTypes();
    }
}
RuntimeLease::~RuntimeLease()
{
    std::lock_guard lock(runtimeMutex);
    if (--runtimeUsers==0)
    {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance=nullptr;
    }
}
}
