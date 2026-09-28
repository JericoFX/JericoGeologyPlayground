#pragma once

namespace Playground::Physics
{
class RuntimeLease
{
  public:
    RuntimeLease();
    ~RuntimeLease();
    RuntimeLease(const RuntimeLease &)=delete;
    RuntimeLease &operator=(const RuntimeLease &)=delete;
};
}
