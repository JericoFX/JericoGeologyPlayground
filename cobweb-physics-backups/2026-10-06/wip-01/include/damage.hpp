#pragma once
#include "cobweb.hpp"
namespace web { double damage(V2 uv,double amount,uint64_t seed,int patch,bool irregular=false,int layer=0); bool damageMayIntersect(V2 a,V2 b,double amount,uint64_t seed,int patch,bool irregular=false,int layer=0); }
