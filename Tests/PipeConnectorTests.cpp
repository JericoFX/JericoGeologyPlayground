#include "PipeConnector.hpp"
#include "SplineConstruction.hpp"
#include "SocketInference.hpp"
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
        // Sweep the routed centreline and read the sockets back out of the generated mesh:
        // the loop is only closed if the pipe that was built still ends on the sockets.
        {
            const auto straightFit=P::connect(frame(0,0,0,1,0,0),frame(1,0,0,-1,0,0));
            const auto elbowFit=P::connect(frame(0,0,0,1,0,0),frame(1,0,1,0,0,-1));
            auto sweep=[&](const P::ConnectorResult& fit,double radius) {
                Playground::PA::ProceduralSpline spline;
                // A deterministic, non-zero identity: the same route always builds the same mesh.
                spline.splineGuid.bytes={std::byte{0x1},std::byte{0x2},std::byte{0x3},std::byte{0x4},
                    std::byte{0x5},std::byte{0x6},std::byte{0x7},std::byte{0x8},
                    std::byte{0x9},std::byte{0xa},std::byte{0xb},std::byte{0xc},
                    std::byte{0xd},std::byte{0xe},std::byte{0xf},std::byte{0x10}};
                spline.name="connector";
                spline.interpolation=Playground::PA::SplineInterpolation::Polyline;
                for(const auto& point:fit.path) {
                    Playground::PA::SplineControlPoint control;
                    control.position=point;
                    control.pointGuid.bytes[0]=std::byte(std::uint8_t(spline.points.size()+1));
                    spline.points.push_back(control);
                }
                Playground::Construction::ConstructionSettings settings;
                settings.profile=Playground::Construction::ProfileKind::Circle;
                settings.radiusMeters=radius;
                // A connector is an open segment: its rims are the sockets themselves.
                settings.capStart=false;settings.capEnd=false;
                settings.maximumVertices=200000;
                return Playground::Construction::buildConstruction(spline,settings);
            };
            Json sweeps=Json::array();
            for(const auto& [id,fit,radius] : std::vector<std::tuple<std::string,P::ConnectorResult,double>>{
                    {"straight",straightFit,0.1},{"elbow",elbowFit,0.1}}) {
                const auto built=sweep(fit,radius);
                std::vector<JE::SocketInference::Vec3> points;
                points.reserve(built.mesh.mesh.positions.size());
                for(const auto& p:built.mesh.mesh.positions)points.push_back({p.x,p.y,p.z});
                const auto inferred=JE::SocketInference::infer(points,built.mesh.mesh.indices);
                Json entry={{"id",id},{"triangles",built.mesh.mesh.indices.size()/3},
                    {"inferred_sockets",inferred.sockets.size()},{"boundary_loops",inferred.boundaryLoops}};
                sweeps.push_back(entry);
                // Both routes must read back exactly two sockets: that is the loop closing.
                require(inferred.sockets.size()==2,"Sweep did not leave two readable sockets");
                for(const auto& socket:inferred.sockets) {
                    require(std::abs(socket.diameter-2*radius)<0.005,"Read back socket diameter is not the profile diameter");
                    const auto distanceTo=[&](const Playground::Vec3d& end) {
                        return std::sqrt(std::pow(socket.position.x-end.x,2)+std::pow(socket.position.y-end.y,2)
                            +std::pow(socket.position.z-end.z,2));
                    };
                    require(std::min(distanceTo(fit.path.front()),distanceTo(fit.path.back()))<0.01,
                        "Read back socket is not on a routed end");
                }
                std::cout<<"sweep "<<id<<": triangles="<<built.mesh.mesh.indices.size()/3
                         <<" inferred="<<inferred.sockets.size()<<" border_edges="<<inferred.boundaryEdges
                         <<" welded="<<inferred.vertices<<" loops="<<inferred.boundaryLoops<<" sizes=";
                for(const auto size:inferred.loopSizes)std::cout<<size<<",";
                std::cout<<"\n";

                // Direct geometric closure: the swept mesh must reach each routed end and its
                // rim there must have the profile radius, whatever the socket inference says.
                auto rimRadius=[&](const Playground::Vec3d& end,const Playground::Vec3d& axis) {
                    double radius=0;std::size_t reached=0;
                    for(const auto& p:built.mesh.mesh.positions) {
                        const double dx=p.x-end.x,dy=p.y-end.y,dz=p.z-end.z;
                        // Select by distance ALONG the axis, not to the point: rim vertices
                        // sit one radius away from the centre by construction.
                        const double along=dx*axis.x+dy*axis.y+dz*axis.z;
                        if(std::abs(along)>0.02)continue;
                        ++reached;
                        const double ax=dx-along*axis.x,ay=dy-along*axis.y,az=dz-along*axis.z;
                        radius=std::max(radius,std::sqrt(ax*ax+ay*ay+az*az));
                    }
                    return std::pair<double,std::size_t>{radius,reached};
                };
                const auto toAxis=[&](Playground::Vec3d a,Playground::Vec3d b) {
                    const double dx=b.x-a.x,dy=b.y-a.y,dz=b.z-a.z;
                    const double l=std::sqrt(dx*dx+dy*dy+dz*dz);
                    return Playground::Vec3d{dx/l,dy/l,dz/l};
                };
                const auto startRim=rimRadius(fit.path.front(),toAxis(fit.path.front(),fit.path[1]));
                const auto endRim=rimRadius(fit.path.back(),toAxis(fit.path[fit.path.size()-2],fit.path.back()));
                entry["start_rim_radius"]=startRim.first;entry["end_rim_radius"]=endRim.first;
                entry["start_vertices"]=startRim.second;entry["end_vertices"]=endRim.second;
                for(const auto& socket:inferred.sockets)
                    std::cout<<"  socket d="<<socket.diameter<<" at ("<<socket.position.x<<","<<socket.position.y
                             <<","<<socket.position.z<<") loop="<<socket.loopVertices<<" radial_err="<<socket.radialErrorMeters
                             <<" plane_err="<<socket.planeErrorMeters<<"\n";
                std::cout<<"  rim radius start="<<startRim.first<<" end="<<endRim.first
                         <<" (profile "<<radius<<", vertices "<<startRim.second<<"/"<<endRim.second<<")\n";
                require(std::abs(startRim.first-radius)<0.02,"Swept mesh does not reach the start socket with the profile radius");
                require(std::abs(endRim.first-radius)<0.02,"Swept mesh does not reach the end socket with the profile radius");
            }
            result["sweeps"]=sweeps;
        }
        // Inline valve: the run is cut and the two remaining pieces must stop exactly on the
        // valve faces, otherwise the "valve" floats inside the pipe.
        {
            const auto straightFit=P::connect(frame(0,0,0,1,0,0),frame(1,0,0,-1,0,0));
            const auto elbowFit=P::connect(frame(0,0,0,1,0,0),frame(1,0,1,0,0,-1));
            Json valves=Json::array();
            for(const auto& [id,route,fraction] : std::vector<std::tuple<std::string,P::ConnectorResult,double>>{
                    {"straight",straightFit,0.5},{"elbow",elbowFit,0.25}}) {
                const auto placement=P::insertValve(route,fraction,0.2,0.14);
                Json entry={{"id",id},{"placed",placement.placed},{"reason",placement.reason},
                    {"before_points",placement.before.size()},{"after_points",placement.after.size()},
                    {"length",placement.lengthMeters},{"radius",placement.radiusMeters}};
                valves.push_back(entry);
                std::cout<<"valve "<<id<<": placed="<<placement.placed<<" before="<<placement.before.size()
                         <<" after="<<placement.after.size()<<"\n";
                require(placement.placed,"Valve was not placed at the middle of the route");
                const auto gapTo=[&](const Playground::Vec3d& a,const Playground::Vec3d& b) {
                    return std::sqrt(std::pow(a.x-b.x,2)+std::pow(a.y-b.y,2)+std::pow(a.z-b.z,2));
                };
                const auto& valveStart=placement.valve.position;
                const auto half=0.5*placement.lengthMeters;
                const Playground::Vec3d faceA{valveStart.x-placement.valve.forward.x*half,
                    valveStart.y-placement.valve.forward.y*half,valveStart.z-placement.valve.forward.z*half};
                const Playground::Vec3d faceB{valveStart.x+placement.valve.forward.x*half,
                    valveStart.y+placement.valve.forward.y*half,valveStart.z+placement.valve.forward.z*half};
                require(gapTo(placement.before.back(),faceA)<1e-9,"First run does not stop on the valve face");
                require(gapTo(placement.after.front(),faceB)<1e-9,"Second run does not start on the valve face");
                require(gapTo(placement.before.front(),route.path.front())<1e-9,"First run lost its socket end");
                require(gapTo(placement.after.back(),route.path.back())<1e-9,"Second run lost its socket end");
            }
            // The guard must actually refuse: a valve centred on the elbow would span it.
            const auto spanning=P::insertValve(elbowFit,0.5,0.2,0.14);
            result["valve_on_elbow"]={{"placed",spanning.placed},{"reason",spanning.reason}};
            std::cout<<"valve on elbow: placed="<<spanning.placed<<" reason="<<spanning.reason<<"\n";
            require(!spanning.placed&&spanning.reason.find("elbow")!=std::string::npos,
                "A valve centred on an elbow was accepted");
            result["valves"]=valves;
        }
        std::ofstream(output/"pipe-connector-report.json")<<result.dump(2);
        std::cout<<"pipe connector: "<<accepted<<"/"<<result["cases"].size()<<" socket pairs routed\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"CONNECTOR FAILURE: "<<error.what()<<"\n";
        return 1;
    }
}
