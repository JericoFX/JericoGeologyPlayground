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
        Json result={{"assets",Json::array()}};
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
