#pragma once
#include "jerico/scene.hpp"
namespace jerico {
Json carve_panel(const Json&part,const Json&polygon_xz,std::string side="front",Json depth=nullptr);
Json irregular_polygon(V2 center_xz,V2 radii_xz,int64_t seed,int vertices=13,double jaggedness=.25,uint64_t positive_offset=0);
bool point_in_panel(const Json&part,V3 point);
Json crack_profiles(V2 center_xz,double length,double width,double depth,int64_t seed,V2 direction_xz={0,1},int branches=2,int segments=6,uint64_t positive_offset=0);
}
