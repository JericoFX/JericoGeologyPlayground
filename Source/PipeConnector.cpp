// SPDX-License-Identifier: GPL-3.0-only
#include "PipeConnector.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Playground::Pipes {
namespace {
constexpr double kEpsilon=1e-12;
double length(Vec3d v){return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);}
Vec3d subtract(Vec3d a,Vec3d b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3d add(Vec3d a,Vec3d b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3d multiply(Vec3d a,double s){return {a.x*s,a.y*s,a.z*s};}
double dot(Vec3d a,Vec3d b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3d cross(Vec3d a,Vec3d b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Vec3d normalized(Vec3d a){const double l=length(a);return l>kEpsilon?multiply(a,1.0/l):Vec3d{};}
double angleBetween(Vec3d a,Vec3d b){return std::acos(std::clamp(dot(a,b),-1.0,1.0));}
} // namespace

const char* name(ConnectorState state) {
    switch(state) {
        case ConnectorState::Straight:return "Straight";
        case ConnectorState::SingleElbow:return "SingleElbow";
        case ConnectorState::DoubleElbow:return "DoubleElbow";
        default:return "Unconnectable";
    }
}

ConnectorResult connect(const Frame& from,const Frame& to,const ConnectorSettings& settings) {
    ConnectorResult result;
    const auto a=normalized(from.forward), arrival=multiply(normalized(to.forward),-1.0);
    if(length(a)<0.5||length(arrival)<0.5) {result.reason="Socket frame has no direction";return result;}
    result.startTangentError=0;
    const Vec3d start=from.position, end=to.position;
    if(length(subtract(end,start))<settings.minimumLegMeters) {result.reason="Sockets coincide";return result;}

    // Closest points between the outgoing line at the first socket and the line that
    // runs backwards from the second socket: P = start + a*t, Q = end + back*s, s >= 0.
    // Using the arrival direction there put every elbow on the far side of the socket.
    const Vec3d back=multiply(arrival,-1.0);
    const Vec3d w=subtract(start,end);
    const double aa=dot(a,a), bb=dot(back,back), ab=dot(a,back);
    const double aw=dot(a,w), bw=dot(back,w);
    const double denominator=aa*bb-ab*ab;
    double t=0,s=0;
    if(std::abs(denominator)>1e-10) {
        t=(ab*bw-bb*aw)/denominator;
        s=(aa*bw-ab*aw)/denominator;
    } else {
        // Parallel lines. If they are collinear and face each other, it is a straight run.
        const Vec3d offset=subtract(start,end);
        const double lateral=length(cross(a,offset));
        if(lateral<settings.minimumLegMeters&&dot(a,offset)<0) {
            result.state=ConnectorState::Straight;
            result.path={start,end};
            result.lengthMeters=length(offset);
            result.endTangentError=angleBetween(a,arrival);
            return result;
        }
        result.reason="Parallel sockets are neither collinear nor connectable with two elbows";
        return result;
    }

    const Vec3d p=add(start,multiply(a,t));
    const Vec3d q=add(end,multiply(back,s));
    const bool frontOfBoth=t>=0.0&&s>=0.0;
    if(!frontOfBoth) {
        result.reason="The elbow would sit behind a socket";
        return result;
    }

    std::vector<Vec3d> path{start};
    if(length(subtract(p,start))>=settings.minimumLegMeters)path.push_back(p);
    if(length(subtract(q,p))>=settings.minimumLegMeters)path.push_back(q);
    path.push_back(end);
    if(path.size()==2) {
        result.state=ConnectorState::Straight;
    } else if(path.size()==3) {
        result.state=ConnectorState::SingleElbow;
    } else if(path.size()==4) {
        result.state=ConnectorState::DoubleElbow;
    } else {
        result.reason="Unexpectedly many legs";
        return result;
    }

    result.path=path;
    for(std::size_t index=1;index<path.size();++index)result.lengthMeters+=length(subtract(path[index],path[index-1]));
    result.startTangentError=angleBetween(normalized(subtract(path[1],path[0])),a);
    result.endTangentError=angleBetween(normalized(subtract(path.back(),path[path.size()-2])),arrival);
    if(result.startTangentError>settings.tangentToleranceRadians||result.endTangentError>settings.tangentToleranceRadians) {
        result.reason="The fitted legs do not follow the socket axes";
        result.state=ConnectorState::Unconnectable;
        return result;
    }
    return result;
}
}
