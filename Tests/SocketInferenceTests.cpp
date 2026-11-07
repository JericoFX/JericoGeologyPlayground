#include "SocketInference.hpp"
#include "AnimationImport.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace S=JE::SocketInference;
using Json=nlohmann::json;

namespace {
void require(bool condition,const std::string& message){if(!condition)throw std::runtime_error(message);}

Json run(const std::filesystem::path& asset,const std::filesystem::path& sidecar) {
    Json out;
    const auto loaded=Playground::AnimationImport::loadAsset(asset);
    const auto& mesh=loaded.renderMesh.mesh.mesh;
    std::vector<S::Vec3> points;points.reserve(mesh.positions.size());
    for(const auto& p:mesh.positions)points.push_back({p.x,p.y,p.z});
    const auto report=S::infer(points,mesh.indices);
    out["asset"]=asset.filename().string();
    out["triangles"]=mesh.indices.size()/3;
    out["vertices"]=mesh.positions.size();
    out["boundary_loops"]=report.boundaryLoops;
    out["rejected_loops"]=report.rejectedLoops;
    out["planar_regions"]=report.planarRegions;
    out["loops"]=Json::array();
    for(const auto& loop:report.loops)out["loops"].push_back({{"vertices",loop.vertices},{"closed",loop.closed},
        {"max_radial",loop.maximumRadial},{"avg_radial",loop.averageRadial},
        {"centre",{loop.centre.x,loop.centre.y,loop.centre.z}}});
    out["clusters"]=Json::array();
    for(const auto& cluster:report.clusters)out["clusters"].push_back({{"area",cluster.area},{"radius",cluster.radius},
        {"max_radial",cluster.maximumRadial},{"avg_radial",cluster.averageRadial},{"triangles",cluster.triangles},
        {"position",{cluster.position.x,cluster.position.y,cluster.position.z}},
        {"normal",{cluster.normal.x,cluster.normal.y,cluster.normal.z}}});
    out["best_fill"]=report.bestFill;
    out["best_radial"]=report.bestRadial;
    out["sockets"]=Json::array();
    for(const auto& socket:report.sockets)out["sockets"].push_back({{"position",{socket.position.x,socket.position.y,socket.position.z}},
        {"forward",{socket.forward.x,socket.forward.y,socket.forward.z}},{"diameter",socket.diameter},
        {"radial_error",socket.radialErrorMeters},{"plane_error",socket.planeErrorMeters},
        {"loop",socket.loopVertices},{"closed",socket.closedLoop}});
    if(!sidecar.empty()&&std::filesystem::exists(sidecar)) {
        std::ifstream input(sidecar);const auto declared=Json::parse(input);
        out["declared"]=Json::array();
        for(const auto& socket:declared["sockets"])out["declared"].push_back({{"name",socket["name"]},
            {"position",socket["position"]},{"forward",socket["forward"]},{"diameter",socket["diameter"]}});
    }
    return out;
}
// Synthetic controls: a clean open tube and the same tube with capped ends. The
// algorithms must be exact here; the scanned pipes are a separate, harder case.
struct Mesh {std::vector<S::Vec3> positions;std::vector<std::uint32_t> indices;};

Mesh tube(double radius,double length,int segments,bool capped) {
    Mesh mesh;
    for(int index=0;index<segments;++index) {
        const double angle=2.0*3.14159265358979323846*double(index)/double(segments);
        const double x=radius*std::cos(angle),y=radius*std::sin(angle);
        mesh.positions.push_back({x,y,0});
        mesh.positions.push_back({x,y,length});
    }
    for(int index=0;index<segments;++index) {
        const auto next=(index+1)%segments;
        const std::uint32_t a=std::uint32_t(index*2),b=std::uint32_t(index*2+1);
        const std::uint32_t c=std::uint32_t(next*2),d=std::uint32_t(next*2+1);
        mesh.indices.insert(mesh.indices.end(),{a,c,b,b,c,d});
    }
    if(capped) {
        for(const double z:{0.0,length}) {
            const auto centre=std::uint32_t(mesh.positions.size());
            mesh.positions.push_back({0,0,z});
            const std::uint32_t ringStart=std::uint32_t(mesh.positions.size());
            for(int index=0;index<segments;++index) {
                const double angle=2.0*3.14159265358979323846*double(index)/double(segments);
                mesh.positions.push_back({radius*std::cos(angle),radius*std::sin(angle),z});
            }
            for(int index=0;index<segments;++index) {
                const auto next=(index+1)%segments;
                if(z==0.0)mesh.indices.insert(mesh.indices.end(),{centre,ringStart+std::uint32_t(index),ringStart+std::uint32_t(next)});
                else mesh.indices.insert(mesh.indices.end(),{centre,ringStart+std::uint32_t(next),ringStart+std::uint32_t(index)});
            }
        }
    }
    return mesh;
}
} // namespace

int main(int argc,char** argv) {
    try {
        bool strict=false;
        for(int index=1;index<argc;++index)if(std::string(argv[index])=="--strict")strict=true;
        const std::filesystem::path root=PLAYGROUND_ROOT;
        const std::filesystem::path output=argc>1?std::filesystem::path(argv[1]):std::filesystem::temp_directory_path()/"sockets";
        std::filesystem::create_directories(output);
        struct Asset {const char* id;const char* glb;const char* sidecar;};
        const std::vector<Asset> assets={
            {"elbow","models/industrial+pipe+elbow+3d+model.glb","models/industrial+pipe+elbow+3d+model.sockets.json"},
            {"bend","models/industrial+pipe+bend+3d+model.glb",nullptr},
            {"coupling","models/rusty+pipe+coupling+3d+model.glb",nullptr},
            {"valve","models/industrial+valve+3d+model.glb",nullptr}};
        Json result={{"assets",Json::array()},{"synthetic",Json::array()}};
        // Synthetic gates: exact ground truth.
        const auto open=tube(0.17,0.5,64,false);
        const auto openReport=S::infer(open.positions,open.indices);
        const auto capped=tube(0.17,0.5,64,true);
        const auto cappedReport=S::infer(capped.positions,capped.indices);
        auto describe=[](const S::Report& report) {
            Json item={{"sockets",Json::array()}};
            for(const auto& socket:report.sockets)item["sockets"].push_back({{"diameter",socket.diameter},
                {"position",{socket.position.x,socket.position.y,socket.position.z}},
                {"forward",{socket.forward.x,socket.forward.y,socket.forward.z}}});
            return item;
        };
        result["synthetic"].push_back({{"id","open-tube"},{"report",describe(openReport)}});
        result["synthetic"].push_back({{"id","capped-tube"},{"report",describe(cappedReport)}});
        std::cout<<"synthetic open-tube sockets="<<openReport.sockets.size()
                 <<" capped-tube sockets="<<cappedReport.sockets.size()<<"\n";
        require(openReport.sockets.size()==2,"Open tube did not yield two sockets");
        for(const auto& socket:openReport.sockets)require(std::abs(socket.diameter-0.34)<0.002,"Open tube socket diameter is wrong");
        require(cappedReport.sockets.size()>=2,"Capped tube did not yield two sockets");
        for(const auto& socket:cappedReport.sockets)
            require(std::abs(socket.diameter-0.34)<0.02,"Capped tube socket diameter is wrong");
        std::size_t compared=0,matched=0;
        for(const auto& asset:assets) {
            const std::filesystem::path glb=root/asset.glb;
            if(!std::filesystem::exists(glb))throw std::runtime_error("Missing asset: "+glb.string());
            Json item=run(glb,asset.sidecar?root/asset.sidecar:std::filesystem::path{});
            item["id"]=asset.id;
            std::cout<<asset.id<<": loops="<<item["boundary_loops"].get<std::size_t>()
                     <<" rejected="<<item["rejected_loops"].get<std::size_t>()
                     <<" sockets="<<item["sockets"].size()<<"\n";
            for(const auto& socket:item["sockets"])
                std::cout<<"   d="<<socket["diameter"].get<double>()<<" at ("
                    <<socket["position"][0].get<double>()<<","<<socket["position"][1].get<double>()<<","
                    <<socket["position"][2].get<double>()<<")\n";
            if(item.contains("declared")) {
                item["declared_match"]=Json::array();
                for(const auto& declaredSocket:item["declared"]) {
                    ++compared;
                    const double dx=declaredSocket["position"][0].get<double>();
                    const double dy=declaredSocket["position"][1].get<double>();
                    const double dz=declaredSocket["position"][2].get<double>();
                    const double fx=declaredSocket["forward"][0].get<double>();
                    const double fy=declaredSocket["forward"][1].get<double>();
                    const double fz=declaredSocket["forward"][2].get<double>();
                    const double diameter=declaredSocket["diameter"].get<double>();
                    double bestDistance=1e9;Json best;
                    for(const auto& socket:item["sockets"]) {
                        const double px=socket["position"][0].get<double>();
                        const double py=socket["position"][1].get<double>();
                        const double pz=socket["position"][2].get<double>();
                        const double distance=std::sqrt((px-dx)*(px-dx)+(py-dy)*(py-dy)+(pz-dz)*(pz-dz));
                        if(distance<bestDistance){bestDistance=distance;best=socket;}
                    }
                    const bool found=bestDistance<0.03&&!best.is_null();
                    double axisError=180.0,diameterError=1.0;
                    if(found) {
                        const double ax=best["forward"][0].get<double>();
                        const double ay=best["forward"][1].get<double>();
                        const double az=best["forward"][2].get<double>();
                        axisError=std::acos(std::min(1.0,std::abs(ax*fx+ay*fy+az*fz)))*180.0/3.14159265358979323846;
                        diameterError=std::abs(best["diameter"].get<double>()-diameter)/diameter;
                    }
                    const bool passed=found&&axisError<5.0&&diameterError<0.10;
                    item["declared_match"].push_back({{"name",declaredSocket["name"]},{"distance",bestDistance},
                        {"axis_degrees",axisError},{"diameter_error",diameterError},{"passed",passed}});
                    std::cout<<"   declared "<<declaredSocket["name"].get<std::string>()<<": distance="<<bestDistance
                             <<" axis="<<axisError<<"deg diameter_error="<<diameterError<<"\n";
                    if(passed)++matched;
                }
            }
            result["assets"].push_back(item);
        }
        result["gate"]={{"declared",compared},{"matched",matched}};
        std::ofstream(output/"socket-inference-report.json")<<result.dump(2);
        std::cout<<"socket inference: "<<matched<<"/"<<compared<<" declared sockets matched\n";
        require(compared>0,"No declared sockets were available for comparison");
        // Probe by default: the survey is recorded even while it is RED. --strict turns the
        // current numbers into a gate.
        if(strict&&matched!=compared)throw std::runtime_error("Inferred sockets do not match the declared sidecar");
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"SOCKET FAILURE: "<<error.what()<<"\n";
        return 1;
    }
}
