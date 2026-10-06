#pragma once
#include "field.hpp"
namespace rocks {
struct Mesh {std::vector<Vec3> positions,normals;std::vector<double> resistance;std::vector<std::uint32_t> indices;std::vector<Vec3> cornerNormals;};
struct Extraction {Mesh mesh;std::uint64_t ambiguousCells=0;bool boundaryTouched=false;double fieldMs=0,extractNormalsMs=0;std::uint64_t coreBufferBytes=0;double surfaceDetailMs=0,maxDisplacement=0,jacobianLowerBound=1;std::uint64_t flippedTriangles=0,preservedSharpVertices=0,contractedEdges=0,contractedFaces=0;double maxContractionDistance=0;};
struct Report {bool accepted=false;std::vector<std::string> errors;std::uint64_t components=0,degenerateFaces=0,duplicateFaces=0,badEdges=0,badVertices=0;double signedVolume=0;};
struct HeightfieldSpec {std::uint32_t width=0,height=0,cells=128;double size=2,relief=.5,backDepth=.3;bool fixedMapping=false;};
// Versioned procedural recipe: scalar codes always map [0,65535] to [0,relief] metres.
struct HeightfieldRecipe {std::uint32_t seed=48,width=512,height=512,cells=128,bedCount=12;double size=2,relief=.9,backDepth=.3,thicknessVariation=1,tiltDegrees=3.148096099562,fractures=1,irregularity=1;bool fragmentedContacts=false;std::string profile="stratified";double directionDegrees=0,jointSpacing=.24,jointDepth=.65,blockScale=.28;double layerSpacingMeters=.045,plateLengthMeters=.22,channelSpacingMeters=.20,channelWidthMeters=.055,pocketRadiusMeters=.05;std::uint32_t pocketCount=8;};
struct HeightfieldSamples {std::vector<std::uint16_t> samples;std::uint64_t clippedLow=0,clippedHigh=0;double rawMin=1,rawMax=0;};
void validateHeightfieldRecipe(const HeightfieldRecipe&);
HeightfieldSamples proceduralHeightfield(const HeightfieldRecipe&);
Extraction generateHeightfield(const HeightfieldRecipe&);
std::vector<std::uint16_t> proceduralCliffHeightSamples(std::uint32_t width,std::uint32_t height,std::uint32_t seed,std::uint32_t bedCount=5,bool fragmentedContacts=false);
Extraction heightfieldSolid(const std::vector<std::uint16_t>&,const HeightfieldSpec&);
struct SurfaceDifference{double maxDistance=0,rmsDistance=0;std::uint64_t sampleCount=0;};
SurfaceDifference compareSurfaces(const Mesh&,const Mesh&);
bool ambiguousCase(unsigned);
Extraction extract(const RockField&,const GridSpec&,Stage);
// Bounded MC cleanup; throws if a required contraction is unsafe.
void contractSubresolutionEdges(Extraction&,double size);
Report validate(const Mesh&,double size);
void applySurfaceDetail(Extraction&,const RockField&);
void addEdgeChips(RockField&);
Extraction extractPolyhedral(const RockField&,Stage);
}
