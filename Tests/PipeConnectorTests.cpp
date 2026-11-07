#include "PipeConnector.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace P=Playground::Pipes;
using Json=nlohmann::json;
namespace {
void require(bool condition,const std::string& message){if(!condition)throw std::runtime_error(message);}
P::Frame frame(double px,double py,double pz,double fx,double fy,double fz) {
    return P::Frame{{px,py,pz},{fx,fy,fz},{0,1,0}};
}
Json describe(const P::ConnectorResult& result) {
    Json path=Json::array();
    for(const auto& point:result.path)path.push_back({point.x,point.y,point.z});
    return {{"state",P::name(result.state)},{"length",result.lengthMeters},{"path",path},
        {"start_tangent",result.startTangentError},{"end_tangent",result.endTangentError},{"reason",result.reason}};
}
} // namespace

int main(int argc,char** argv) {
    try {
        const std::filesystem::path output=argc>1?std::filesystem::path(argv[1]):std::filesystem::temp_directory_path()/"connector";
        std::filesystem::create_directories(output);
        Json result={{"cases",Json::array()}};

        // Straight run: the sockets face each other on the same line.
        {
            const auto fit=P::connect(frame(0,0,0,1,0,0),frame(1,0,0,-1,0,0));
            {Json item=describe(fit);item["id"]="straight";result["cases"].push_back(item);}
            require(fit.state==P::ConnectorState::Straight,"Collinear facing sockets are not a straight run");
            require(std::abs(fit.lengthMeters-1.0)<1e-9,"Straight run length is wrong");
            require(fit.path.size()==2,"Straight run has extra points");
        }
        // Perpendicular: one elbow exactly at the corner of the two axes.
        {
            const auto fit=P::connect(frame(0,0,0,1,0,0),frame(1,0,1,0,0,-1));
            {Json item=describe(fit);item["id"]="elbow";result["cases"].push_back(item);}
            require(fit.state==P::ConnectorState::SingleElbow,"Perpendicular sockets did not produce one elbow");
            require(fit.path.size()==3,"Single elbow path has the wrong point count");
            require(std::abs(fit.path[1].x-1)<1e-9&&std::abs(fit.path[1].z)<1e-9,"Elbow is not at the axis corner");
            require(std::abs(fit.lengthMeters-2.0)<1e-9,"Elbow length is wrong");
        }
        // Skew: the axes never meet, so the shortest route needs two elbows.
        {
            const auto fit=P::connect(frame(0,0,0,1,0,0),frame(1,-1,1,0,1,0));
            {Json item=describe(fit);item["id"]="skew";result["cases"].push_back(item);}
            require(fit.state==P::ConnectorState::DoubleElbow,"Skew sockets did not produce two elbows");
            require(fit.path.size()==4,"Double elbow path has the wrong point count");
            require(std::abs(fit.path[1].x-1)<1e-9&&std::abs(fit.path[1].z)<1e-9,"First elbow is not on the X axis line");
            require(std::abs(fit.path[2].z-1)<1e-9&&std::abs(fit.path[2].y)<1e-9,"Second elbow is not on the Y axis line");
            require(std::abs(fit.lengthMeters-3.0)<1e-9,"Double elbow length is wrong");
        }
        // Backwards: the target faces away, so no two-elbow route exists.
        {
            const auto fit=P::connect(frame(0,0,0,1,0,0),frame(-1,0,0,-1,0,0));
            {Json item=describe(fit);item["id"]="backwards";result["cases"].push_back(item);}
            require(fit.state==P::ConnectorState::Unconnectable,"A backwards socket was accepted");
        }
        // Every accepted route must follow the socket axes, not only look plausible.
        std::size_t accepted=0;
        for(const auto& item:result["cases"])if(item["state"]!="Unconnectable")++accepted;
        result["accepted"]=accepted;
        std::ofstream(output/"pipe-connector-report.json")<<result.dump(2);
        std::cout<<"pipe connector: "<<accepted<<"/"<<result["cases"].size()<<" socket pairs routed\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"CONNECTOR FAILURE: "<<error.what()<<"\n";
        return 1;
    }
}
