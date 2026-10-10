#pragma once
#include "rocks/mesh.hpp"
namespace rocks {
struct LocalPlane {double intercept=0,slopeX=0,slopeY=0,residual=0;};
struct AlveolarGuide {unsigned side=513;double size=2;std::vector<double> mask,distance,residual,slope;std::vector<unsigned char> protectedNodes;};
struct Alveolus {double x,y,radius,aspect,angle,exponent,floor,amplitude,left;};
struct AlveolarPatch {double x,y,radius,aspect,angle;std::vector<Alveolus> cavities;};
struct AlveolarStudy {AlveolarGuide guide;std::vector<AlveolarPatch> patches;};
LocalPlane fitAlveolarPlane(const std::vector<double>&,unsigned,unsigned,unsigned,unsigned,unsigned);
std::vector<double> euclideanProtectedDistance(const std::vector<unsigned char>&,unsigned,unsigned,double);
AlveolarGuide makeAlveolarGuide(const std::vector<double>&,unsigned,double,unsigned=13,unsigned=9);
double alveolarMaskAt(const AlveolarGuide&,double,double);
HeightfieldRecipe alveolarHostRecipe(const HeightfieldRecipe&);
void validateAlveolarRecipe(const HeightfieldRecipe&);
AlveolarStudy makeAlveolarStudy(const HeightfieldRecipe&);
double alveolarCoverage(const AlveolarStudy&,double,double);
double alveolarCut(const HeightfieldRecipe&,const AlveolarStudy&,double,double,double);
HeightfieldSamples alveolarSamples(const HeightfieldRecipe&,const HeightfieldSamples&,const AlveolarStudy&);
}
