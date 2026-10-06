#pragma once
#include "types.hpp"
namespace rocks {
struct Plane {Vec3 normal;double offset;bool operator==(const Plane&)const=default;};
struct Cutter {Vec3 center;double radius;bool operator==(const Cutter&)const=default;};
struct RockField {Recipe recipe;std::vector<Plane> planes;std::vector<Cutter> cutters;double amplitude;std::vector<std::vector<Plane>> fractures;std::vector<std::vector<Plane>> masses{};};
std::vector<std::string> validateRecipe(const Recipe&,const GridSpec&);
GridSpec makeGrid(const Recipe&,std::uint32_t);
RockField makeField(const Recipe&);
double planeValue(Plane,Vec3);
double fieldValue(const RockField&,Vec3,Stage);
double resistance(const RockField&,Vec3);
Sample evaluate(const RockField&,Vec3,Stage);
double erosionMask(const RockField&,Vec3);
double subtractFields(double,double);
double boundedAmplitude(double,double,double);
}
