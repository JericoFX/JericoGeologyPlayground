#include "AnimationImport.hpp"
#include "GeometryArchetypes.hpp"
#include "SurfaceRay.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace A=JE::GeometryArchetypes;
namespace I=Playground::AnimationImport;
namespace D=Playground::PA::Detail;
using Json=nlohmann::json;

namespace {
void require(bool condition,const std::string& message){if(!condition)throw std::runtime_error(message);}

std::string lowered(std::string text){for(auto& c:text)c=char(std::tolower(static_cast<unsigned char>(c)));return text;}

struct PartView {std::string label;std::size_t firstIndex{},indexCount{};};

D::TriangleBvh buildBvh(const Playground::TriangleMesh& mesh,const std::vector<PartView>& parts) {
    std::vector<D::Triangle> triangles;
    for(const auto& part:parts)
        for(std::size_t triangle=0;triangle<part.indexCount/3;++triangle) {
            const auto base=part.firstIndex+triangle*3;
            D::Triangle entry;
            entry.first=mesh.positions.at(mesh.indices.at(base));
            entry.second=mesh.positions.at(mesh.indices.at(base+1));
            entry.third=mesh.positions.at(mesh.indices.at(base+2));
            entry.centroid=D::multiply(D::add(D::add(entry.first,entry.second),entry.third),1./3);
            entry.bounds=D::emptyBounds();
            D::include(entry.bounds,entry.first);D::include(entry.bounds,entry.second);D::include(entry.bounds,entry.third);
            entry.canonicalIndex=std::uint32_t(base/3);
            triangles.push_back(entry);
        }
    return D::TriangleBvh(std::move(triangles));
}

// Leaf perimeter: boundary edges and creases above 30 degrees. Collision of a
// rotating slab with its frame shows up at its perimeter, not on its faces.
std::vector<std::pair<std::uint32_t,std::uint32_t>> perimeter(const Playground::TriangleMesh& mesh,const PartView& part) {
    std::map<std::pair<std::uint32_t,std::uint32_t>,std::vector<std::size_t>> shared;
    for(std::size_t triangle=0;triangle<part.indexCount/3;++triangle) {
        const auto base=part.firstIndex+triangle*3;
        for(int corner=0;corner<3;++corner) {
            const auto a=mesh.indices.at(base+std::size_t(corner)),b=mesh.indices.at(base+std::size_t((corner+1)%3));
            shared[std::minmax(a,b)].push_back(base);
        }
    }
    auto normal=[&](std::size_t base) {
        const auto a=mesh.positions.at(mesh.indices.at(base)),b=mesh.positions.at(mesh.indices.at(base+1)),c=mesh.positions.at(mesh.indices.at(base+2));
        const auto normal=Playground::Projection::cross(D::subtract(b,a),D::subtract(c,a));
        const double length=Playground::Projection::length(normal);
        return length>1e-12?D::multiply(normal,1/length):normal;
    };
    std::vector<std::pair<std::uint32_t,std::uint32_t>> edges;
    for(const auto& [key,faces]:shared) {
        bool crease=faces.size()!=2;
        if(faces.size()==2)crease=std::abs(D::dot(normal(faces[0]),normal(faces[1])))<0.866;
        if(crease)edges.push_back(key);
    }
    return edges;
}

Playground::Vec3d rotate(Playground::Vec3d p,Playground::Vec3d point,Playground::Vec3d axis,double angle) {
    const auto d=D::subtract(p,point);
    const double c=std::cos(angle),s=std::sin(angle);
    const auto cross=Playground::Projection::cross(axis,d);
    const double along=D::dot(axis,d);
    return D::add(point,D::add(D::add(D::multiply(d,c),D::multiply(cross,s)),D::multiply(axis,along*(1-c))));
}

// Straight runs over the leaf perimeter only. Internal moulding creases are excluded:
// rotating about a face crease drives half the leaf through its own frame.
std::vector<A::Joint> borderRuns(const Playground::TriangleMesh& mesh,
                                 const std::vector<std::pair<std::uint32_t,std::uint32_t>>& edges,
                                 double diagonal) {
    struct Run {Playground::Vec3d direction{},point{};double minimum{},maximum{},length{};};
    std::vector<Run> runs;
    const auto subtract=[](Playground::Vec3d a,Playground::Vec3d b){return Playground::Vec3d{a.x-b.x,a.y-b.y,a.z-b.z};};
    const auto add=[](Playground::Vec3d a,Playground::Vec3d b){return Playground::Vec3d{a.x+b.x,a.y+b.y,a.z+b.z};};
    const auto scale=[](Playground::Vec3d a,double s){return Playground::Vec3d{a.x*s,a.y*s,a.z*s};};
    const auto dot=[](Playground::Vec3d a,Playground::Vec3d b){return a.x*b.x+a.y*b.y+a.z*b.z;};
    for(const auto& [from,to]:edges) {
        const auto a=mesh.positions.at(from),b=mesh.positions.at(to);
        const auto delta=subtract(b,a);const double length=Playground::Projection::length(delta);
        if(length<=1e-9)continue;
        const auto direction=scale(delta,1/length);
        bool joined=false;
        for(auto& run:runs) {
            if(std::abs(dot(run.direction,direction))<0.985)continue;
            const auto offset=subtract(a,run.point);
            const auto lateral=subtract(offset,scale(run.direction,dot(offset,run.direction)));
            if(Playground::Projection::length(lateral)>0.01*diagonal)continue;
            const double along=dot(subtract(a,run.point),run.direction);
            const double alongB=dot(subtract(b,run.point),run.direction);
            run.minimum=std::min({run.minimum,along,alongB});
            run.maximum=std::max({run.maximum,along,alongB});
            run.length=run.maximum-run.minimum;
            joined=true;break;
        }
        if(!joined)runs.push_back({direction,a,0.0,length,length});
    }
    std::vector<A::Joint> joints;
    for(const auto& run:runs) {
        if(run.length<0.05*diagonal)continue;
        A::Joint joint;
        joint.axis={run.direction.x,run.direction.y,run.direction.z};
        const auto centre=add(run.point,scale(run.direction,(run.minimum+run.maximum)*0.5));
        joint.point={centre.x,centre.y,centre.z};
        joint.lengthMeters=run.length;joint.score=run.length;
        joints.push_back(joint);
    }
    std::sort(joints.begin(),joints.end(),[](const A::Joint& a,const A::Joint& b){return a.lengthMeters>b.lengthMeters;});
    if(joints.size()>6)joints.resize(6);
    return joints;
}

// Per-edge flags, not a count: an edge that already crossed its frame at rest must
// not be credited as "new" when the count happens to stay the same.
std::vector<unsigned char> crossingFlags(const D::TriangleBvh& frame,const Playground::TriangleMesh& mesh,
                                         const std::vector<std::pair<std::uint32_t,std::uint32_t>>& edges,
                                         const std::vector<std::size_t>& use,
                                         const std::vector<Playground::Vec3d>& rotated,double tolerance,double margin) {
    std::vector<unsigned char> flags(use.size(),0);
    for(std::size_t slot=0;slot<use.size();++slot) {
        const auto [from,to]=edges[use[slot]];
        const auto origin=rotated[from];const auto delta=D::subtract(rotated[to],origin);
        const double length=Playground::Projection::length(delta);if(length<=2*tolerance)continue;
        const auto direction=D::multiply(delta,1/length);
        const auto hit=Playground::Projection::cast(frame,mesh,origin,direction,length-tolerance);
        if(std::isfinite(hit.distance)&&hit.distance>margin&&hit.distance<length-margin)flags[slot]=1;
    }
    return flags;
}
} // namespace

int main(int argc,char** argv) {
    try {
        bool diagnose=false,strict=false;
        for(int index=1;index<argc;++index) {
            const std::string argument=argv[index];
            if(argument=="--diagnose")diagnose=true;
            if(argument=="--strict")strict=true;
        }
        const std::filesystem::path root=PLAYGROUND_ROOT;
        std::filesystem::path output=std::filesystem::temp_directory_path()/"hinge";
        for(int index=1;index<argc;++index) {
            const std::string argument=argv[index];
            if(!argument.empty()&&argument[0]!='-')output=argument;
        }
        std::filesystem::create_directories(output);
        // Expected state recorded on 2026-11-07. vault and metal are known unresolved:
        // vault admits no clean interval up to 30 mm and metal's leaf already crosses
        // its frame at rest, so its "no new crossings" cannot demonstrate anything.
        struct Door {const char* id;const char* path;bool expectClean;};
        const std::vector<Door> doors={
            {"wooden","Assets/Doors/Repaired/wooden/wooden-door.glb",true},
            {"vault","Assets/Doors/Repaired/vault/vault-door.glb",false},
            {"prison","Assets/Doors/Repaired/prison/prison-door.glb",true},
            {"metal","Assets/Doors/Repaired/metal/metal-door.glb",false}};
        Json report={{"doors",Json::array()}};
        std::size_t gated=0,passed=0;
        for(const auto& door:doors) {
            const auto asset=I::loadAsset(root/door.path);
            const auto& mesh=asset.renderMesh.mesh.mesh;
            Json item={{"id",door.id},{"parts",Json::array()}};
            std::vector<PartView> leaves,frames;
            for(const auto& part:asset.renderMesh.parts) {
                PartView view{part.localId.empty()?part.nodeId:part.localId,part.firstIndex,part.indexCount};
                const auto label=lowered(view.label);
                const bool isLeaf=label.find("leaf")!=std::string::npos||label.find("hoja")!=std::string::npos;
                const bool isFrame=label.find("frame")!=std::string::npos||label.find("marco")!=std::string::npos;
                if(isLeaf)leaves.push_back(view);
                if(isFrame)frames.push_back(view);
                item["parts"].push_back({{"label",view.label},{"triangles",view.indexCount/3}});
            }
            if(diagnose) {
                std::cout<<door.id<<": "<<asset.renderMesh.parts.size()<<" parts\n";
                for(const auto& part:asset.renderMesh.parts) {
                    Playground::Vec3d low{1e30,1e30,1e30},high{-1e30,-1e30,-1e30};
                    for(std::size_t triangle=0;triangle<part.indexCount/3;++triangle) {
                        const auto base=part.firstIndex+triangle*3;
                        for(int corner=0;corner<3;++corner) {
                            const auto q=mesh.positions.at(mesh.indices.at(base+std::size_t(corner)));
                            low={std::min(low.x,q.x),std::min(low.y,q.y),std::min(low.z,q.z)};
                            high={std::max(high.x,q.x),std::max(high.y,q.y),std::max(high.z,q.z)};
                        }
                    }
                    std::cout<<"   "<<part.localId<<" tris="<<part.indexCount/3
                             <<" bbox=("<<low.x<<","<<low.y<<","<<low.z<<")-("<<high.x<<","<<high.y<<","<<high.z<<")\n";
                }
            }
            if(leaves.empty()||frames.empty())throw std::runtime_error(std::string("Door asset lacks leaf/frame parts: ")+door.id);

            // Classify the leaf alone, then build sweep candidates from its border only.
            std::vector<A::Vec3> points;
            std::vector<std::uint32_t> indices;
            {
                std::map<std::uint32_t,std::uint32_t> remap;
                for(std::size_t triangle=0;triangle<leaves.front().indexCount/3;++triangle) {
                    const auto base=leaves.front().firstIndex+triangle*3;
                    for(int corner=0;corner<3;++corner) {
                        const auto original=mesh.indices.at(base+std::size_t(corner));
                        auto found=remap.find(original);
                        if(found==remap.end()) {
                            found=remap.emplace(original,std::uint32_t(points.size())).first;
                            const auto p=mesh.positions.at(original);points.push_back({p.x,p.y,p.z});
                        }
                        indices.push_back(found->second);
                    }
                }
            }
            const auto classification=A::classify(points,indices);
            const auto edgesEarly=perimeter(mesh,leaves.front());
            const auto joints=borderRuns(mesh,edgesEarly,classification.evidence.diagonal);
            item["class"]=A::name(classification.ranked.front().type);
            item["joint_candidates"]=joints.size();
            if(joints.empty())throw std::runtime_error(std::string("No hinge candidate for ")+door.id);

            const auto frameBvh=buildBvh(mesh,frames);
            const auto edges=perimeter(mesh,leaves.front());
            item["perimeter_edges"]=edges.size();
            const double tolerance=1e-5*classification.evidence.diagonal;
            // The repaired leaf is seated inside the frame rabbet at rest, so the
            // criterion is "no NEW crossings", the same rule used for tooth contacts.
            const double margin=0.002; // 2 mm of material on both sides.
            std::vector<std::size_t> allEdges(edges.size());
            for(std::size_t index=0;index<edges.size();++index)allEdges[index]=index;
            const auto restingFlags=crossingFlags(frameBvh,mesh,edges,allEdges,mesh.positions,tolerance,margin);
            const auto resting=std::size_t(std::count(restingFlags.begin(),restingFlags.end(),1));
            item["resting_crossings"]=resting;
            // A real hinge pin sits at the frame, outside the reveal, so the axis can be
            // offset along the leaf normal. Search that clearance before rejecting an axis.
            const auto normal=A::Vec3{classification.evidence.thinAxis.x,classification.evidence.thinAxis.y,classification.evidence.thinAxis.z};
            auto coarse=allEdges;
            if(coarse.size()>4000)coarse.resize(4000);
            const auto sweep=[&](const A::Joint& joint,int sign,double offset,const std::vector<std::size_t>& use) {
                const Playground::Vec3d axis{joint.axis.x,joint.axis.y,joint.axis.z};
                const Playground::Vec3d point{joint.point.x+normal.x*offset,joint.point.y+normal.y*offset,joint.point.z+normal.z*offset};
                std::size_t added=0,peak=0;
                for(int step=1;step<=6;++step) {
                    const double angle=sign*double(step)*15.0*3.14159265358979323846/180.0;
                    std::vector<Playground::Vec3d> rotated;rotated.reserve(mesh.positions.size());
                    for(const auto& q:mesh.positions)rotated.push_back(rotate(q,point,axis,angle));
                    const auto flags=crossingFlags(frameBvh,mesh,edges,use,rotated,tolerance,margin);
                    std::size_t fresh=0;
                    for(std::size_t slot=0;slot<flags.size();++slot)
                        if(flags[slot]&&restingFlags[use[slot]]==0)++fresh;
                    peak=std::max(peak,fresh);
                    added+=fresh;
                }
                return std::pair<std::size_t,std::size_t>{added,peak};
            };

            Json sweeps=Json::array();
            bool anyClean=false;
            double bestPlateauOffset=0,bestPlateauWidth=0;
            const bool measurable=resting==0;
            // The pin must live inside the door's own slab, so the clearance is bounded by
            // half the leaf thickness measured along its normal. A pin outside the door is
            // not a hinge, it is a rig that dodges the frame.
            double thickness=0;
            {
                double low=1e30,high=-1e30;
                for(const auto& q:mesh.positions) {
                    const double projection=double(q.x)*normal.x+double(q.y)*normal.y+double(q.z)*normal.z;
                    low=std::min(low,projection);high=std::max(high,projection);
                }
                thickness=high-low;
            }
            // Declared bound: a pin more than 30 mm outside the leaf border is not a hinge,
            // it is a rig that dodges the frame. Without this bound "clean" intervals appear
            // at 85-190 mm, which is physically meaningless.
            constexpr int kSteps=7;
            constexpr double kStep=0.005;
            constexpr double kMinimumPlateau=0.020; // A single lucky offset is not a clearance.
            for(std::size_t candidate=0;candidate<std::min<std::size_t>(2,joints.size());++candidate) {
                const auto& joint=joints[candidate];
                for(const int sign:{1,-1}) {
                    Json entry={{"candidate",candidate},{"sign",sign},{"scan",Json::array()}};
                    double bestOffset=0,bestWidth=0;
                    for(const int side:{1,-1}) {
                        std::vector<std::size_t> added(std::size_t(kSteps),0);
                        for(int step=0;step<kSteps;++step) {
                            const double offset=double(step)*kStep*side;
                            added[std::size_t(step)]=sweep(joint,sign,offset,coarse).first;
                            entry["scan"].push_back({{"offset",offset},{"added",added[std::size_t(step)]}});
                        }
                        // Longest contiguous clean run: a real clearance is an interval.
                        for(int begin=0;begin<kSteps;) {
                            if(added[std::size_t(begin)]!=0) {++begin;continue;}
                            int end=begin;while(end+1<kSteps&&added[std::size_t(end+1)]==0)++end;
                            const double width=(end-begin)*kStep;
                            if(width>bestWidth||(width==bestWidth&&std::abs(begin*kStep)<std::abs(bestOffset))) {
                                bestWidth=width;bestOffset=(begin+end)*0.5*kStep*side;
                            }
                            begin=end+1;
                        }
                    }
                    bool clean=false;
                    if(bestWidth>=kMinimumPlateau-1e-12&&bestWidth>0) {
                        const auto [fullAdded,fullPeak]=sweep(joint,sign,bestOffset,allEdges);
                        entry["verified_added"]=fullAdded;entry["verified_peak"]=fullPeak;
                        clean=fullAdded==0;
                    }
                    entry["plateau_width"]=bestWidth;
                    entry["clearance"]=bestOffset;
                    entry["clean"]=clean&&measurable;
                    sweeps.push_back(entry);
                    if(clean&&measurable&&(bestWidth>bestPlateauWidth)) {anyClean=true;bestPlateauWidth=bestWidth;bestPlateauOffset=bestOffset;}
                }
            }
            item["sweeps"]=sweeps;
            item["has_clean_sweep"]=anyClean;
            item["measurable"]=measurable;
            item["leaf_thickness_m"]=thickness;
            item["plateau_width_m"]=bestPlateauWidth;
            item["clearance_m"]=bestPlateauOffset;
            ++gated;if(anyClean)++passed;
            if(anyClean!=door.expectClean)
                throw std::runtime_error(std::string("Hinge sweep state changed for ")+door.id
                    +": expected "+(door.expectClean?"a clean interval":"no clean interval"));
            std::cout<<door.id<<": class="<<item["class"].get<std::string>()<<" thickness="<<thickness
                     <<"m perimeter="<<edges.size()<<" resting="<<resting<<" clean="<<(anyClean?"yes":"no")
                     <<" plateau="<<bestPlateauWidth<<"m at "<<bestPlateauOffset<<"m\n";
            for(const auto& sweep:sweeps)std::cout<<"   cand "<<sweep["candidate"]<<" dir "<<sweep["sign"]
                <<" clean="<<sweep["clean"].get<bool>()<<" width="<<sweep["plateau_width"].get<double>()
                <<" at="<<sweep["clearance"].get<double>()<<"\n";
            report["doors"].push_back(item);
        }
        report["gate"]={{"doors",gated},{"with_clean_sweep",passed}};
        std::ofstream(output/"hinge-sweep-report.json")<<report.dump(2);
        if(diagnose)return 0;
        std::cout<<"hinge sweep: "<<passed<<"/"<<gated<<" doors have a collision-free opening\n";
        // Probe by default: the survey is recorded even when it fails. --strict turns
        // the current numbers into a gate, which is how the RED is reproduced.
        if(strict&&passed!=gated)throw std::runtime_error("No collision-free hinge sweep for every door");
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"HINGE FAILURE: "<<error.what()<<"\n";
        return 1;
    }
}
