#include "GeometryArchetypes.hpp"
#include "AnimationImport.hpp"
#include "AnimationRigFit.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace A=JE::GeometryArchetypes;
using Json=nlohmann::json;
namespace {
void require(bool condition,const std::string& message){if(!condition)throw std::runtime_error(message);}

struct Entry {std::string id,path;A::Class expected;bool gate;std::string note;};

std::vector<Entry> corpus() {
    const std::filesystem::path root=PLAYGROUND_ROOT;
    return {
        {"door-wooden",      "models/wooden+door+3d+model.glb",           A::Class::HingedPanel,true, "complete wooden door"},
        {"door-prison",      "models/rusty+prison+door+3d+model.glb",     A::Class::HingedPanel,true, "complete prison door"},
        {"door-vault",       "models/rusted+vault+door+3d+model.glb",     A::Class::HingedPanel,true, "complete vault door"},
        {"door-metal",       "models/rusty+metal+door+3d+model.glb",      A::Class::HingedPanel,true, "complete metal door"},
        {"leaf-vault",       "Assets/Doors/Repaired/vault/leaf.glb",      A::Class::HingedPanel,true, "leaf alone"},
        {"frame-vault",      "Assets/Doors/Repaired/vault/frame.glb",     A::Class::RigidFrame, true, "frame alone"},
        {"spider-monster",   "models/monster spider 3d model.glb",        A::Class::Arthropod,  true, "eight legs"},
        {"spider-realistic", "models/realistic spider 3d model.glb",      A::Class::Arthropod,  true, "eight legs"},
        {"spider-giant",     "models/giant+spider+3d+model.glb",          A::Class::Arthropod,  true, "eight legs"},
        {"spider-alien",     "models/alien spider 3d model.glb",          A::Class::Arthropod,  true, "eight legs"},
        {"larva",            "models/monster larva 3d model.glb",         A::Class::Axial,      true, "single column"},
        {"creature-alien",   "models/alien creature 3d model.glb",        A::Class::Unknown,    false,"organic, class unverified"},
        {"table",            "models/weathered+wooden+table+3d+model.glb",A::Class::RigidFrame, true, "four supports but furniture"},
        {"humanoid-philips", "models/Philips.fbx",                        A::Class::Biped,      true, "humanoid with arms at mid height"},
        {"staircase",        "models/wooden+staircase+3d+model.glb",      A::Class::Unknown,    false,"structure"},
        {"dock",             "models/wooden+dock+3d+model.glb",           A::Class::Unknown,    false,"structure"},
        {"valve",            "models/VALVE01_Gate_PBR_1K.glb",            A::Class::Unknown,    false,"mechanism must not be a creature"},
    };
}

Json evidenceJson(const A::Evidence& e) {
    Json panels=Json::array();
    for(const auto& p:e.panels)panels.push_back({{"area",p.area},{"normal",{p.normal.x,p.normal.y,p.normal.z}},
        {"offset",p.offset},{"longest_edge",p.longestEdgeMeters}});
    return {{"diagonal",e.diagonal},{"area",e.area},{"vertices",e.vertexCount},{"triangles",e.triangleCount},
        {"degenerate",e.degenerateTriangles},{"components",e.componentCount},{"boxiness",e.boxiness},
        {"planar_area_fraction",e.planarAreaFraction},{"thinness",e.thinness},{"elongation",e.elongation},
        {"symmetry_error",e.symmetryError},{"supports",e.supportCount},{"grounded_area_fraction",e.groundedAreaFraction},
        {"vertical_area_fraction",e.verticalPanelFraction},{"vertical_coverage",e.verticalCoverage},{"support_spread",e.supportSpread},{"grounded_supports",e.groundedSupportCount},{"thin_axis",{e.thinAxis.x,e.thinAxis.y,e.thinAxis.z}},{"regions",e.regionCount},{"largest_region",e.largestRegionFraction},
        {"face_step_debug",{{"mode",e.faceStepDepth},{"coverage",e.faceStepCoverage},{"offset_fraction",e.faceStepOffsetFraction}}},
        {"face_steps",[&]{Json list=Json::array();for(const auto& s:e.faceSteps)list.push_back({{"area",s.areaFraction},{"fill",s.fillRatio},{"height",s.heightFraction},{"depth",s.depthMeters},{"rect",s.rectangular}});return list;}()},
        {"joints",[&]{Json list=Json::array();for(const auto& j:e.joints)list.push_back({{"axis",{j.axis.x,j.axis.y,j.axis.z}},{"point",{j.point.x,j.point.y,j.point.z}},{"length",j.lengthMeters}});return list;}()},
        {"panels",panels}};
}

Json classifyJson(const Playground::TriangleMesh& mesh,Json* legacy) {
    std::vector<A::Vec3> points;points.reserve(mesh.positions.size());
    for(const auto& p:mesh.positions)points.push_back({p.x,p.y,p.z});
    const auto report=A::classify(points,mesh.indices);
    Json ranked=Json::array();
    for(const auto& hypothesis:report.ranked)ranked.push_back({{"class",A::name(hypothesis.type)},{"score",hypothesis.score},{"evidence",hypothesis.reason}});
    if(legacy) {
        // Recorded for the RED: the previous support-only inference applied to the same mesh.
        try {
            const auto fit=Playground::AnimationImport::fitWalkingRig(mesh,0,7);
            (*legacy)["walking_rig_supports"]=fit.legs.size();
        } catch(const std::invalid_argument& error) {(*legacy)["walking_rig_error"]=error.what();}
        try {
            const auto axial=Playground::AnimationImport::fitAxialRig(mesh,17);
            (*legacy)["axial_rig_points"]=axial.size();
        } catch(const std::invalid_argument& error) {(*legacy)["axial_rig_error"]=error.what();}
    }
    return {{"accepted",A::name(report.ranked.front().type)},{"ambiguous",report.ambiguous},
        {"evidence",evidenceJson(report.evidence)},{"ranked",ranked}};
}

// Synthetic control: a closed box, an inset drawer face and a leaf separated from
// its frame. These are exact geometry, so their expected class is not an opinion.
Playground::TriangleMesh box(double sizeX,double sizeY,double sizeZ,double offsetX=0,double offsetY=0,double offsetZ=0) {
    Playground::TriangleMesh mesh;
    const double x=sizeX*0.5,y=sizeY*0.5,z=sizeZ*0.5;
    const double corners[8][3]={{-x,-y,-z},{x,-y,-z},{x,y,-z},{-x,y,-z},{-x,-y,z},{x,-y,z},{x,y,z},{-x,y,z}};
    for(const auto& c:corners)mesh.positions.push_back({c[0]+offsetX,c[1]+offsetY,c[2]+offsetZ});
    const std::uint32_t faces[12][3]={{0,2,1},{0,3,2},{4,5,6},{4,6,7},{0,1,5},{0,5,4},{3,7,6},{3,6,2},{0,4,7},{0,7,3},{1,2,6},{1,6,5}};
    for(const auto& f:faces)for(const auto c:f)mesh.indices.push_back(c);
    return mesh;
}
Playground::TriangleMesh merge(Playground::TriangleMesh a,const Playground::TriangleMesh& b) {
    const auto offset=std::uint32_t(a.positions.size());
    for(const auto& p:b.positions)a.positions.push_back(p);
    for(const auto index:b.indices)a.indices.push_back(index+offset);
    return a;
}
} // namespace

int main(int argc,char** argv) {
    try {
        const std::filesystem::path output=argc>1?std::filesystem::path(argv[1]):std::filesystem::temp_directory_path()/"archetypes";
        std::filesystem::create_directories(output);
        Json result={{"corpus",Json::array()},{"synthetic",Json::array()}};
        std::size_t gated=0,passed=0;Json mismatches=Json::array();

        for(const auto& entry:corpus()) {
            const std::filesystem::path path=std::filesystem::path(PLAYGROUND_ROOT)/entry.path;
            if(!std::filesystem::exists(path))throw std::runtime_error("Missing corpus asset: "+path.string());
            const auto asset=Playground::AnimationImport::loadAsset(path);
            Json legacy;
            Json item={{"id",entry.id},{"path",entry.path},{"note",entry.note}};
            item.merge_patch(classifyJson(asset.renderMesh.mesh.mesh,&legacy));
            item["legacy"]=legacy;
            item["expected"]=A::name(entry.expected);
            item["gated"]=entry.gate;
            if(entry.gate) {
                ++gated;
                const auto accepted=item["accepted"].get<std::string>();
                const bool ok=accepted==A::name(entry.expected);
                item["pass"]=ok;
                if(ok)++passed;else mismatches.push_back({{"id",entry.id},{"expected",A::name(entry.expected)},{"accepted",accepted},
                    {"top_score",item["ranked"][0]["score"]},{"runner_up",item["ranked"][1]["class"]},{"runner_up_score",item["ranked"][1]["score"]}});
            }
            if(entry.gate&&entry.expected==A::Class::HingedPanel) {
                // A hinge candidate must be a long straight crease run that lies in the
                // panel plane. Orientation-free: these scans are not stored upright.
                const auto& evidence=item["evidence"];
                const auto& list=evidence["joints"];
                const auto thin=evidence["thin_axis"];
                const double thinX=thin[0].get<double>(),thinY=thin[1].get<double>(),thinZ=thin[2].get<double>();
                const double diagonal=evidence["diagonal"].get<double>();
                double longest=0,bestPlanarity=1;
                for(const auto& joint:list) {
                    const auto axis=joint["axis"];
                    const double planarity=std::abs(axis[0].get<double>()*thinX+axis[1].get<double>()*thinY+axis[2].get<double>()*thinZ);
                    const double runLength=joint["length"].get<double>();
                    longest=std::max(longest,runLength);
                    if(runLength>0.5*diagonal)bestPlanarity=std::min(bestPlanarity,planarity);
                }
                item["hinge_longest_run"]=longest;item["hinge_planarity"]=bestPlanarity;
                if(!(longest>0.5*diagonal&&bestPlanarity<0.25))
                    throw std::runtime_error("No in-plane hinge run for "+entry.id);
                std::cout<<"  hinge: longest="<<longest<<"m in-plane residual="<<bestPlanarity<<"\n";
            }
            result["corpus"].push_back(item);
            std::cout<<entry.id<<": "<<item["accepted"].get<std::string>()<<" (expected "<<A::name(entry.expected)<<")"
                <<" supports="<<item["evidence"]["supports"]<<" grounded="<<item["evidence"]["grounded_supports"]<<" planar="<<item["evidence"]["planar_area_fraction"]
                <<" thin="<<item["evidence"]["thinness"]<<" boxy="<<item["evidence"]["boxiness"]<<"\n";
        }
        result["gate"]={{"tracked",gated},{"passed",passed},{"mismatches",mismatches}};

        // Synthetic controls.
        const auto cube=box(1,1,1);
        const auto cubeReport=A::classify([&]{std::vector<A::Vec3> p;for(const auto& v:cube.positions)p.push_back({v.x,v.y,v.z});return p;}(),cube.indices);
        const auto cubePanel=box(1.0,1.2,0.06);
        const auto cubePanelReport=A::classify([&]{std::vector<A::Vec3> p;for(const auto& v:cubePanel.positions)p.push_back({v.x,v.y,v.z});return p;}(),cubePanel.indices);
        // Legacy control: a full-height proud plate, not a drawer front.
        const auto drawerFrame=box(1.0,1.0,0.5);
        const auto drawerFace=box(0.8,0.8,0.02,0,0,0.26);
        const auto drawer=merge(drawerFrame,drawerFace);
        const auto drawerReport=A::classify([&]{std::vector<A::Vec3> p;for(const auto& v:drawer.positions)p.push_back({v.x,v.y,v.z});return p;}(),drawer.indices);
        // Furniture controls: a carcass with a proud drawer front, and a plain wardrobe shell.
        const auto carcass=box(1.0,0.9,0.5);
        const auto drawerModule=merge(carcass,box(0.8,0.35,0.03,0,0,0.265));
        // Hollow carcass: five panels leave a real cavity behind the drawer front, so the
        // travel and depth are measured rather than assumed.
        auto hollow=box(1.0,0.9,0.05,0,0,-0.225);
        hollow=merge(hollow,box(0.05,0.9,0.5,-0.475,0,0));
        hollow=merge(hollow,box(0.05,0.9,0.5,0.475,0,0));
        hollow=merge(hollow,box(1.0,0.05,0.5,0,-0.425,0));
        hollow=merge(hollow,box(1.0,0.05,0.5,0,0.425,0));
        const auto hollowDrawer=merge(hollow,box(0.8,0.35,0.03,0,0,0.26));
        const auto hollowReport=A::classify([&]{std::vector<A::Vec3> p;for(const auto& v:hollowDrawer.positions)p.push_back({v.x,v.y,v.z});return p;}(),hollowDrawer.indices);
        const auto drawerModuleReport=A::classify([&]{std::vector<A::Vec3> p;for(const auto& v:drawerModule.positions)p.push_back({v.x,v.y,v.z});return p;}(),drawerModule.indices);
        const auto wardrobeReport=A::classify([&]{std::vector<A::Vec3> p;for(const auto& v:carcass.positions)p.push_back({v.x,v.y,v.z});return p;}(),carcass.indices);
        const auto doorFrame=box(1.0,2.0,0.12);
        const auto doorLeaf=box(0.9,1.9,0.05,0.02,0,0.09);
        const auto door=merge(doorFrame,doorLeaf);
        const auto doorReport=A::classify([&]{std::vector<A::Vec3> p;for(const auto& v:door.positions)p.push_back({v.x,v.y,v.z});return p;}(),door.indices);
        for(const auto& [id,report]:std::vector<std::pair<std::string,A::Report>>{{"cube",cubeReport},{"thin-panel",cubePanelReport},
                {"proud-plate",drawerReport},{"wardrobe",wardrobeReport},{"drawer-module",drawerModuleReport},{"hollow-drawer",hollowReport},{"door",doorReport}}) {
            result["synthetic"].push_back({{"id",id},{"accepted",A::name(report.ranked.front().type)},{"score",report.ranked.front().score},
                {"evidence",evidenceJson(report.evidence)}});
            std::cout<<"synthetic "<<id<<": "<<A::name(report.ranked.front().type)<<" score="<<report.ranked.front().score<<"\n";
        }

        const auto& hollowBox=hollowReport.evidence.drawer;
        const auto& solidBox=drawerModuleReport.evidence.drawer;
        result["drawer_reconstruction"]={{"hollow",{{"class",A::name(hollowReport.ranked.front().type)},{"cavity",hollowBox.cavity},
            {"width",hollowBox.width},{"height",hollowBox.height},{"depth",hollowBox.depth},{"travel",hollowBox.travel}}},
            {"solid",{{"cavity",solidBox.cavity},{"depth",solidBox.depth},{"travel",solidBox.travel}}}};
        // Skeleton: one chain per grounded contact, following the surface.
        auto skeletonOf=[&](const Playground::TriangleMesh& mesh) {
            std::vector<A::Vec3> p;for(const auto& v:mesh.positions)p.push_back({v.x,v.y,v.z});
            const auto report=A::classify(p,mesh.indices);
            return std::make_pair(report,A::skeleton(p,mesh.indices,report,4));
        };
        const auto philipsMesh=Playground::AnimationImport::loadAsset(std::filesystem::path(PLAYGROUND_ROOT)/"models/Philips.fbx").renderMesh.mesh.mesh;
        const auto [philipsReport,philipsSkeleton]=skeletonOf(philipsMesh);
        const auto spiderMesh=Playground::AnimationImport::loadAsset(std::filesystem::path(PLAYGROUND_ROOT)/"models/giant+spider+3d+model.glb").renderMesh.mesh.mesh;
        const auto [spiderReport,spiderSkeleton]=skeletonOf(spiderMesh);
        const auto larvaMesh=Playground::AnimationImport::loadAsset(std::filesystem::path(PLAYGROUND_ROOT)/"models/monster larva 3d model.glb").renderMesh.mesh.mesh;
        const auto [larvaReport,larvaSkeleton]=skeletonOf(larvaMesh);
        result["skeleton"]={{"humanoid",{{"class",A::name(philipsReport.ranked.front().type)},{"chains",philipsSkeleton.chains.size()},
            {"bones",philipsSkeleton.bones.size()},{"length",philipsSkeleton.totalLength}}},
            {"spider",{{"class",A::name(spiderReport.ranked.front().type)},{"chains",spiderSkeleton.chains.size()},
            {"bones",spiderSkeleton.bones.size()},{"length",spiderSkeleton.totalLength}}},
            {"larva",{{"class",A::name(larvaReport.ranked.front().type)},{"chains",larvaSkeleton.chains.size()},
            {"bones",larvaSkeleton.bones.size()},{"length",larvaSkeleton.totalLength}}}};
        std::cout<<"skeleton humanoid: chains="<<philipsSkeleton.chains.size()<<" bones="<<philipsSkeleton.bones.size()
                 <<" length="<<philipsSkeleton.totalLength<<"\n";
        std::cout<<"skeleton spider: chains="<<spiderSkeleton.chains.size()<<" bones="<<spiderSkeleton.bones.size()
                 <<" length="<<spiderSkeleton.totalLength<<"\n";
        // Each chain must start at the root and end on its own contact, not merely exist.
        auto chainGeometry=[&](const A::Skeleton& skeleton,const A::Report& report) {
            Json list=Json::array();
            for(const auto& chain:skeleton.chains) {
                const auto& first=skeleton.bones[chain.bones.front()];
                const auto& last=skeleton.bones[chain.bones.back()];
                const auto head=first.head, tail=last.tail;
                const double headError=std::hypot(std::hypot(head.x-skeleton.root.x,head.y-skeleton.root.y),head.z-skeleton.root.z);
                double contactError=0;
                if(chain.contact!=A::noContact&&chain.contact<report.evidence.supportPoints.size()) {
                    const auto& contact=report.evidence.supportPoints[chain.contact];
                    contactError=std::hypot(std::hypot(tail.x-contact.x,tail.y-contact.y),tail.z-contact.z);
                }
                list.push_back({{"bones",chain.bones.size()},{"length",chain.length},
                    {"head_error",headError},{"contact_error",contactError}});
                require(headError<0.01,"Skeleton chain does not start at the root");
                require(contactError<0.02,"Skeleton chain does not end on its contact");
                require(chain.length>0.05,"Skeleton chain is degenerate");
            }
            return list;
        };
        // Skinning: rest pose must be exact, and rotating one limb must not move the rest.
        auto skinCheck=[&](const Playground::TriangleMesh& mesh,const std::string& label) {
            std::vector<A::Vec3> p;for(const auto& v:mesh.positions)p.push_back({v.x,v.y,v.z});
            const auto report=A::classify(p,mesh.indices);
            const auto skeleton=A::skeleton(p,mesh.indices,report,4);
            const auto skinning=A::skin(p,skeleton,4);
            double maximumWeightError=0;
            for(const auto& row:skinning.weights) {
                const double sum=row[0]+row[1]+row[2]+row[3];
                maximumWeightError=std::max(maximumWeightError,std::abs(sum-1.0));
            }
            std::vector<A::BoneTransform> identity(skeleton.bones.size());
            const auto rest=A::deform(p,skinning,skeleton,identity);
            double maximumRestError=0;
            for(std::size_t index=0;index<p.size();++index)
                maximumRestError=std::max(maximumRestError,std::hypot(std::hypot(rest[index].x-p[index].x,
                    rest[index].y-p[index].y),rest[index].z-p[index].z));
            // Rotate the whole first chain about its first bone head by 30 degrees.
            auto transforms=identity;
            const double angle=30.0*3.14159265358979323846/180.0;
            const double cosine=std::cos(angle),sine=std::sin(angle);
            const auto& chain=skeleton.chains.front();
            for(const auto bone:chain.bones)
                transforms[bone].rotation={1,0,0,0,cosine,-sine,0,sine,cosine};
            const auto posed=A::deform(p,skinning,skeleton,transforms);
            double movedInChain=0,maximumOutside=0;
            for(std::size_t index=0;index<p.size();++index) {
                const double moved=std::hypot(std::hypot(posed[index].x-p[index].x,posed[index].y-p[index].y),
                    posed[index].z-p[index].z);
                bool influencedByChain=false;
                for(std::size_t slot=0;slot<4;++slot)
                    if(skinning.weights[index][slot]>0)
                        for(const auto bone:chain.bones)
                            if(skinning.bones[index][slot]==bone)influencedByChain=true;
                if(influencedByChain)movedInChain=std::max(movedInChain,moved);
                else maximumOutside=std::max(maximumOutside,moved);
            }
            result["skinning"][label]={{"bones",skeleton.bones.size()},{"vertices",p.size()},
                {"maximum_weight_error",maximumWeightError},{"maximum_rest_error",maximumRestError},
                {"moved_in_chain",movedInChain},{"maximum_outside_motion",maximumOutside}};
            std::cout<<"skinning "<<label<<": weight_error="<<maximumWeightError<<" rest_error="<<maximumRestError
                     <<" moved="<<movedInChain<<" outside="<<maximumOutside<<"\n";
            require(maximumWeightError<1e-9,"Skinning weights do not sum to one");
            require(maximumRestError<1e-9,"Rest pose does not reproduce the mesh exactly");
            require(movedInChain>0.01,"Rotating a limb did not move any of its vertices");
            require(maximumOutside<1e-9,"Rotating a limb moved vertices outside that chain");
        };
        result["skinning"]=Json::object();
        skinCheck(philipsMesh,"humanoid");
        skinCheck(spiderMesh,"spider");
        // Large-angle joint test: rotating only the distal part of a chain stresses the blend
        // at the joint. Linear blend skinning pinches there; dual quaternion keeps the girth.
        auto jointTest=[&](const Playground::TriangleMesh& mesh,const std::string& label,double degrees) {
            std::vector<A::Vec3> p;for(const auto& v:mesh.positions)p.push_back({v.x,v.y,v.z});
            const auto report=A::classify(p,mesh.indices);
            const auto skeleton=A::skeleton(p,mesh.indices,report,4);
            const auto skinning=A::skin(p,skeleton,4);
            const auto& chain=skeleton.chains.front();
            if(chain.bones.size()<2)return;
            const auto joint=skeleton.bones[chain.bones[1]].head;
            // The triangle set is fixed at rest. Selecting triangles from the posed mesh would
            // move the goalposts: a shrinking band would simply drop out of the sample.
            Playground::Vec3d low{1e30,1e30,1e30},high{-1e30,-1e30,-1e30};
            for(const auto& v:p){low={std::min(low.x,v.x),std::min(low.y,v.y),std::min(low.z,v.z)};
                high={std::max(high.x,v.x),std::max(high.y,v.y),std::max(high.z,v.z)};}
            const double diagonal=std::hypot(std::hypot(high.x-low.x,high.y-low.y),high.z-low.z);
            const double radius=0.12*diagonal;
            std::vector<std::size_t> band;
            for(std::size_t triangle=0;triangle+2<mesh.indices.size();triangle+=3) {
                bool inside=true;
                for(int corner=0;corner<3;++corner) {
                    const auto& v=p[mesh.indices[triangle+std::size_t(corner)]];
                    if(std::hypot(std::hypot(v.x-joint.x,v.y-joint.y),v.z-joint.z)>radius){inside=false;break;}
                }
                if(inside)band.push_back(triangle);
            }
            auto bandArea=[&](const std::vector<A::Vec3>& posed) {
                double area=0;
                for(const auto triangle:band) {
                    const auto a=posed[mesh.indices[triangle]],b=posed[mesh.indices[triangle+1]],c=posed[mesh.indices[triangle+2]];
                    const Playground::Vec3d ab{b.x-a.x,b.y-a.y,b.z-a.z},ac{c.x-a.x,c.y-a.y,c.z-a.z};
                    const Playground::Vec3d cross{ab.y*ac.z-ab.z*ac.y,ab.z*ac.x-ab.x*ac.z,ab.x*ac.y-ab.y*ac.x};
                    area+=0.5*std::hypot(std::hypot(cross.x,cross.y),cross.z);
                }
                return area;
            };
            const auto restArea=bandArea(p);
            std::vector<A::BoneTransform> identity(skeleton.bones.size());
            const double angle=degrees*3.14159265358979323846/180.0;
            const double cosine=std::cos(angle),sine=std::sin(angle);
            auto transforms=identity;
            for(std::size_t index=1;index<chain.bones.size();++index)
                transforms[chain.bones[index]].rotation={1,0,0,0,cosine,-sine,0,sine,cosine};
            auto bend=[&](bool dual) {
                A::DeformSettings settings;settings.dualQuaternion=dual;
                const auto posed=A::deform(p,skinning,skeleton,transforms,settings);
                return bandArea(posed);
            };
            const auto linear=bend(false),dual=bend(true);
            result["joint_blend"][label]={{"degrees",degrees},{"rest_area",restArea},
                {"linear_ratio",restArea>0?linear/restArea:0},{"dual_quaternion_ratio",restArea>0?dual/restArea:0}};
            std::cout<<"joint "<<label<<" @"<<degrees<<"deg: rest="<<restArea<<" linear="<<(restArea>0?linear/restArea:0)
                     <<" dqs="<<(restArea>0?dual/restArea:0)<<"\n";
            require(restArea>0,"Joint band has no area");
            // Measured, not assumed: linear blending pinches the joint band by under 15 % at
            // 90 degrees on these assets. Dual quaternion blending is recorded but rejected:
            // as implemented it expands the band by 60 %, so it is not adopted.
            require(linear/restArea>0.85,"Linear blending pinched the joint band beyond the measured bound");
            result["joint_blend"][label]["triangles"]=band.size();
            result["joint_blend"][label]["dual_quaternion_adopted"]=false;
        };
        result["joint_blend"]=Json::object();
        jointTest(philipsMesh,"humanoid-90",90.0);
        jointTest(spiderMesh,"spider-90",90.0);
        result["skeleton"]["humanoid"]["chains_detail"]=chainGeometry(philipsSkeleton,philipsReport);
        result["skeleton"]["spider"]["chains_detail"]=chainGeometry(spiderSkeleton,spiderReport);
        require(philipsSkeleton.chains.size()==philipsReport.evidence.groundedSupportCount,"Humanoid skeleton does not have one chain per grounded contact");
        require(spiderSkeleton.chains.size()==spiderReport.evidence.groundedSupportCount,"Spider skeleton does not have one chain per grounded contact");
        require(philipsSkeleton.bones.size()>=4&&spiderSkeleton.bones.size()>=8,"Skeleton produced too few bones");
        require(larvaSkeleton.chains.size()==1,"Axial body did not produce a single vertebral chain");
        std::ofstream(output/"archetype-report.json")<<result.dump(2);
        require(A::name(hollowReport.ranked.front().type)=="Drawer","Hollow carcass with a drawer front was not classified as Drawer");
        require(hollowBox.cavity,"Hollow carcass drawer reported no cavity");
        require(std::abs(hollowBox.depth-0.465)<0.03,"Measured drawer cavity depth is wrong");
        require(std::abs(hollowBox.travel-(hollowBox.depth-0.01))<1e-9,"Drawer travel is not derived from the measured cavity");
        require(std::abs(hollowBox.width-0.8)<0.05&&std::abs(hollowBox.height-0.35)<0.05,"Drawer front size was not measured");
        require(!solidBox.cavity,"Solid block produced a drawer cavity that does not exist");
        require(A::name(drawerModuleReport.ranked.front().type)=="Drawer","Furniture carcass with a proud drawer front was not classified as Drawer");
        require(A::name(wardrobeReport.ranked.front().type)=="RigidFrame","Plain wardrobe shell was not classified as RigidFrame");
        std::cout<<"gated "<<passed<<"/"<<gated<<" archetype expectations\n";
        if(passed!=gated)throw std::runtime_error("Archetype expectations failed: "+mismatches.dump());
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"ARCHETYPE FAILURE: "<<error.what()<<"\n";
        return 1;
    }
}
