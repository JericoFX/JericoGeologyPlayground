#pragma once
#include "scene.hpp"
namespace jerico {
// Independent closed, chamfered wood solid. Axis u is grain/length, v width,
// w thickness. a/b are centreline endpoints. Geometry variation is local to id.
Json& dock_timber(Builder& builder,const std::string& id,V3 a,V3 b,V3 width_axis,
 double width,double thickness,const std::string& role,int64_t seed,
 bool broken_start=false,bool broken_end=false,double wear=.6,bool horizontal_top=false);
}
