// SPDX-License-Identifier: GPL-3.0-only
#include "SocketInference.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace JE::SocketInference {
namespace {
constexpr double kEpsilon=1e-12;
struct Cell {std::int64_t x{},y{},z{};bool operator==(const Cell&)const=default;};
struct CellHash {
    std::size_t operator()(const Cell& c)const noexcept {
        std::size_t h=std::hash<std::int64_t>{}(c.x);
        h^=std::hash<std::int64_t>{}(c.y)+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);
        h^=std::hash<std::int64_t>{}(c.z)+0x9e3779b97f4a7c15ull+(h<<6)+(h>>2);
        return h;
    }
};
Cell cellOf(Vec3 p,double size){return {std::int64_t(std::floor(p.x/size)),std::int64_t(std::floor(p.y/size)),std::int64_t(std::floor(p.z/size))};}
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 mul(Vec3 a,double s){return {a.x*s,a.y*s,a.z*s};}
double dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double length(Vec3 a){return std::sqrt(dot(a,a));}
Vec3 normalized(Vec3 a){const double scale=length(a);return scale>kEpsilon?mul(a,1.0/scale):Vec3{};}

struct Weld {
    std::vector<std::uint32_t> remap;
    std::vector<Vec3> points;
};
Weld weld(std::span<const Vec3> positions,double diagonal) {
    const double cell=std::max(1e-9,1e-5*diagonal);
    std::unordered_map<Cell,std::vector<std::uint32_t>,CellHash> grid;
    Weld result;result.remap.resize(positions.size());
    for(std::uint32_t index=0;index<positions.size();++index) {
        const auto base=cellOf(positions[index],cell);
        std::uint32_t found=UINT32_MAX;
        for(int dx=-1;dx<=1&&found==UINT32_MAX;++dx)for(int dy=-1;dy<=1&&found==UINT32_MAX;++dy)for(int dz=-1;dz<=1&&found==UINT32_MAX;++dz) {
            const auto it=grid.find({base.x+dx,base.y+dy,base.z+dz});
            if(it==grid.end())continue;
            for(const auto candidate:it->second)if(length(sub(positions[candidate],positions[index]))<=cell){found=candidate;break;}
        }
        if(found==UINT32_MAX) {
            found=std::uint32_t(result.points.size());result.points.push_back(positions[index]);
            grid[base].push_back(found);
        }
        result.remap[index]=found;
    }
    return result;
}
} // namespace

Report infer(std::span<const Vec3> positions,std::span<const std::uint32_t> indices,const Settings& settings) {
    if(indices.empty()||indices.size()%3)throw std::invalid_argument("Socket inference needs a triangle list");
    Report report;
    Vec3 minimum{positions[0]},maximum{positions[0]},sum{};
    for(const auto p:positions) {
        minimum={std::min({minimum.x,p.x}),std::min({minimum.y,p.y}),std::min({minimum.z,p.z})};
        maximum={std::max({maximum.x,p.x}),std::max({maximum.y,p.y}),std::max({maximum.z,p.z})};
        sum=add(sum,p);
    }
    const double diagonal=length(sub(maximum,minimum));
    const Vec3 centroid=mul(sum,1.0/double(positions.size()));
    const auto welded=weld(positions,diagonal);

    // Edge use counts: an edge used once is a border.
    std::unordered_map<std::uint64_t,int> useCount;
    useCount.reserve(indices.size());
    for(std::size_t triangle=0;triangle+2<indices.size();triangle+=3)
        for(int corner=0;corner<3;++corner) {
            const auto a=welded.remap[indices[triangle+std::size_t(corner)]];
            const auto b=welded.remap[indices[triangle+std::size_t((corner+1)%3)]];
            if(a==b)continue;
            const auto low=std::min(a,b),high=std::max(a,b);
            ++useCount[(std::uint64_t(high)<<32)|low];
        }
    std::unordered_map<std::uint64_t,std::vector<std::size_t>> edgeTriangles;
    for(std::size_t triangle=0;triangle+2<indices.size();triangle+=3)
        for(int corner=0;corner<3;++corner) {
            const auto a=welded.remap[indices[triangle+std::size_t(corner)]];
            const auto b=welded.remap[indices[triangle+std::size_t((corner+1)%3)]];
            if(a==b)continue;
            const auto low=std::min(a,b),high=std::max(a,b);
            edgeTriangles[(std::uint64_t(high)<<32)|low].push_back(triangle/3);
        }
    std::vector<std::pair<std::uint32_t,std::uint32_t>> borders;
    for(const auto& [key,count]:useCount)if(count==1)borders.emplace_back(std::uint32_t(key&0xffffffffu),std::uint32_t(key>>32));

    // Group border edges into loops through shared vertices.
    std::unordered_map<std::uint32_t,std::vector<std::uint32_t>> adjacency;
    for(const auto& [a,b]:borders){adjacency[a].push_back(b);adjacency[b].push_back(a);}
    std::vector<unsigned char> visited(welded.points.size(),0);
    for(const auto& [start,neighbours]:adjacency) {
        if(visited[start])continue;
        std::vector<std::uint32_t> loop;std::vector<std::uint32_t> stack{start};
        visited[start]=1;bool closed=true;
        while(!stack.empty()) {
            const auto current=stack.back();stack.pop_back();loop.push_back(current);
            if(adjacency[current].size()!=2)closed=false;
            for(const auto next:adjacency[current])if(!visited[next]){visited[next]=1;stack.push_back(next);}
        }
        if(loop.size()<settings.minimumLoopVertices)continue;
        ++report.boundaryLoops;
        // Plane fit through the loop, then a 2D least-squares circle.
        Vec3 centre{};
        for(const auto vertex:loop)centre=add(centre,welded.points[vertex]);
        centre=mul(centre,1.0/double(loop.size()));
        Vec3 normalSum{};
        for(std::size_t index=0;index<loop.size();++index) {
            const auto a=welded.points[loop[index]],b=welded.points[loop[(index+1)%loop.size()]];
            normalSum=add(normalSum,cross(sub(a,centre),sub(b,centre)));
        }
        const Vec3 normal=normalized(normalSum);
        if(length(normal)<0.5){++report.rejectedLoops;continue;}
        const Vec3 helper=std::abs(normal.y)<0.9?Vec3{0,1,0}:Vec3{1,0,0};
        const Vec3 u=normalized(cross(helper,normal));
        const Vec3 v=cross(normal,u);
        double sxx=0,sxy=0,syy=0,sxz=0,syz=0,szz=0;
        Vec3 planeDeviationOrigin=centre;
        double maximumPlaneDeviation=0;
        for(const auto vertex:loop) {
            const auto delta=sub(welded.points[vertex],centre);
            const double x=dot(delta,u),y=dot(delta,v),z=dot(delta,normal);
            maximumPlaneDeviation=std::max(maximumPlaneDeviation,std::abs(z));
            sxx+=x*x;sxy+=x*y;syy+=y*y;sxz+=x*x*x+x*y*y;syz+=y*y*y+x*x*y;szz+=x*x+y*y;
        }
        const double determinant=sxx*syy-sxy*sxy;
        if(std::abs(determinant)<1e-18){++report.rejectedLoops;continue;}
        const double cx=(sxz*syy-syz*sxy)/determinant*0.5;
        const double cy=(syz*sxx-sxz*sxy)/determinant*0.5;
        double radialSum=0,radialMax=0;
        for(const auto vertex:loop) {
            const auto delta=sub(welded.points[vertex],centre);
            const double x=dot(delta,u),y=dot(delta,v);
            const double radial=std::hypot(x-cx,y-cy);
            radialSum+=radial;radialMax=std::max(radialMax,radial);
        }
        const double radius=radialSum/double(loop.size());
        if(radius<=kEpsilon){++report.rejectedLoops;continue;}
        SocketCandidate socket;
        socket.position=add(centre,add(mul(u,cx),mul(v,cy)));
        socket.forward=normal;
        // Point the socket away from the body.
        if(dot(socket.forward,sub(socket.position,centroid))<0)socket.forward=mul(socket.forward,-1.0);
        socket.diameter=radius*2;
        socket.radialErrorMeters=radialMax-radius;
        socket.planeErrorMeters=maximumPlaneDeviation;
        socket.loopVertices=loop.size();
        socket.closedLoop=closed;
        if(socket.radialErrorMeters>settings.maximumRadialErrorRelative*socket.diameter) {++report.rejectedLoops;continue;}
        if(socket.planeErrorMeters>settings.maximumPlaneErrorRelative*socket.diameter) {++report.rejectedLoops;continue;}
        report.sockets.push_back(socket);
    }

    // Stage 2: capped pipes. The end of a pipe is a planar disk whose border is a circle,
    // and a scanned or decimated pipe has no clean boundary loop to catch it.
    {
        struct Region {std::vector<std::size_t> triangles;Vec3 normal{};double offset{},area{};};
        std::vector<Vec3> normals;std::vector<double> areas;
        normals.reserve(indices.size()/3);areas.reserve(indices.size()/3);
        std::vector<Vec3> centres;
        for(std::size_t triangle=0;triangle+2<indices.size();triangle+=3) {
            const auto a=positions[indices[triangle]],b=positions[indices[triangle+1]],c=positions[indices[triangle+2]];
            const auto normal=cross(sub(b,a),sub(c,a));
            const double doubleArea=length(normal);
            normals.push_back(doubleArea>kEpsilon?mul(normal,1.0/doubleArea):Vec3{});
            areas.push_back(doubleArea*0.5);
            centres.push_back(mul(add(add(a,b),c),1.0/3.0));
        }
        std::vector<std::vector<std::size_t>> adjacency(normals.size());
        for(std::size_t triangle=0;triangle+2<indices.size();triangle+=3)
            for(int corner=0;corner<3;++corner) {
                const auto a=welded.remap[indices[triangle+std::size_t(corner)]];
                const auto b=welded.remap[indices[triangle+std::size_t((corner+1)%3)]];
                if(a==b)continue;
                const auto low=std::min(a,b),high=std::max(a,b);
                const auto key=(std::uint64_t(high)<<32)|low;
                static const auto unused=0;(void)unused;
                const auto found=edgeTriangles.find(key);
                if(found==edgeTriangles.end())edgeTriangles.emplace(key,std::vector<std::size_t>{triangle/3});
                else found->second.push_back(triangle/3);
            }
        for(const auto& [key,list]:edgeTriangles)
            if(list.size()==2) {
                adjacency[list[0]].push_back(list[1]);
                adjacency[list[1]].push_back(list[0]);
            }
        const double planeCosine=std::cos(0.09); // ~5 degrees
        const double planeOffset=0.005*diagonal;
        std::vector<std::size_t> regionOf(normals.size(),SIZE_MAX);
        std::vector<Region> regions;
        std::vector<std::size_t> order(normals.size());
        for(std::size_t index=0;index<order.size();++index)order[index]=index;
        std::sort(order.begin(),order.end(),[&](auto x,auto y){return areas[x]>areas[y];});
        std::vector<std::size_t> stack;
        for(const auto seed:order) {
            if(regionOf[seed]!=SIZE_MAX)continue;
            const auto slot=regions.size();
            Region region;region.normal=normals[seed];region.offset=dot(normals[seed],centres[seed]);
            regions.push_back(region);
            Vec3 normalSum=normals[seed];double weight=1;
            regionOf[seed]=slot;regions[slot].triangles.push_back(seed);regions[slot].area+=areas[seed];
            stack.assign(1,seed);
            while(!stack.empty()) {
                const auto current=stack.back();stack.pop_back();
                for(const auto next:adjacency[current]) {
                    if(regionOf[next]!=SIZE_MAX)continue;
                    if(dot(normals[next],regions[slot].normal)<planeCosine)continue;
                    if(std::abs(dot(regions[slot].normal,centres[next])-regions[slot].offset)>planeOffset)continue;
                    regionOf[next]=slot;regions[slot].triangles.push_back(next);
                    regions[slot].area+=areas[next];
                    normalSum=add(normalSum,normals[next]);weight+=1;
                }
            }
            regions[slot].normal=normalized(normalSum);
        }
        report.planarRegions=regions.size();
        std::vector<std::size_t> ranked(regions.size());
        for(std::size_t index=0;index<ranked.size();++index)ranked[index]=index;
        std::sort(ranked.begin(),ranked.end(),[&](auto x,auto y){return regions[x].area>regions[y].area;});
        if(ranked.size()>16)ranked.resize(16);
        for(const auto index:ranked) {
            const auto& region=regions[index];
            if(region.triangles.size()<16)continue;
            // Border vertices of the region.
            std::vector<Vec3> border;
            for(const auto triangle:region.triangles)
                for(int corner=0;corner<3;++corner) {
                    const auto a=welded.remap[indices[triangle*3+std::size_t(corner)]];
                    const auto b=welded.remap[indices[triangle*3+std::size_t((corner+1)%3)]];
                    if(a==b)continue;
                    const auto low=std::min(a,b),high=std::max(a,b);
                    const auto found=edgeTriangles.find((std::uint64_t(high)<<32)|low);
                    if(found==edgeTriangles.end())continue;
                    bool internal=false;
                    for(const auto other:found->second)if(other!=triangle&&regionOf[other]==index){internal=true;break;}
                    if(internal)continue;
                    border.push_back(welded.points[a]);
                }
            if(border.size()<16)continue;
            Vec3 centre{};
            for(const auto& point:border)centre=add(centre,point);
            centre=mul(centre,1.0/double(border.size()));
            const Vec3 helper=std::abs(region.normal.y)<0.9?Vec3{0,1,0}:Vec3{1,0,0};
            const Vec3 u=normalized(cross(helper,region.normal));
            const Vec3 v=cross(region.normal,u);
            double sxx=0,sxy=0,syy=0,sxz=0,syz=0,radialMax=0;
            for(const auto& point:border) {
                const auto delta=sub(point,centre);
                const double x=dot(delta,u),y=dot(delta,v);
                sxx+=x*x;sxy+=x*y;syy+=y*y;sxz+=x*x*x+x*y*y;syz+=y*y*y+x*x*y;
            }
            const double determinant=sxx*syy-sxy*sxy;
            if(std::abs(determinant)<1e-18)continue;
            const double cx=(sxz*syy-syz*sxy)/determinant*0.5;
            const double cy=(syz*sxx-sxz*sxy)/determinant*0.5;
            double radialSum=0;
            for(const auto& point:border) {
                const auto delta=sub(point,centre);
                const double x=dot(delta,u),y=dot(delta,v);
                const double radial=std::hypot(x-cx,y-cy);
                radialSum+=radial;radialMax=std::max(radialMax,radial);
            }
            const double radius=radialSum/double(border.size());
            if(radius<=kEpsilon)continue;
            // A disk, not a rim: the filled area must match the circle it circumscribes.
            const double circleArea=3.14159265358979323846*radius*radius;
            const double fill=region.area/circleArea;
            const double radialRelative=(radialMax-radius)/(radius*2);
            report.bestFill=std::max(report.bestFill,fill>1.30?1.30/fill:fill);
            report.bestRadial=report.bestRadial==0?radialRelative:std::min(report.bestRadial,radialRelative);
            if(fill<0.70||fill>1.30)continue;
            if(radialRelative>settings.maximumRadialErrorRelative)continue;
            SocketCandidate socket;
            socket.position=add(centre,add(mul(u,cx),mul(v,cy)));
            socket.forward=region.normal;
            if(dot(socket.forward,sub(socket.position,centroid))<0)socket.forward=mul(socket.forward,-1.0);
            socket.diameter=radius*2;
            socket.radialErrorMeters=radialMax-radius;
            socket.planeErrorMeters=0;
            socket.loopVertices=border.size();
            socket.closedLoop=true;
            report.sockets.push_back(socket);
        }
    }


    // Stage 3: planar clusters keyed by normal and offset. This needs no adjacency at all,
    // which matters because these scanned GLBs are not reliably vertex-shared. A disk of
    // area A has radius sqrt(A/pi); the cluster's own vertices must reach that radius.
    {
        struct Cluster {Vec3 normal{};double offset{};double area{};Vec3 centroidSum{};double weight{};};
        std::unordered_map<std::uint64_t,std::size_t> index;
        std::vector<Cluster> clusters;
        auto keyOf=[](Vec3 normal,double offset) {
            const auto quantize=[&](double value){return std::uint64_t(std::uint32_t(std::int32_t(std::lround(value*50.0))));};
            const auto offsetKey=std::uint64_t(std::uint32_t(std::int32_t(std::lround(offset/0.005))));
            std::uint64_t key=quantize(normal.x);
            key=key*1000003ull^quantize(normal.y);
            key=key*1000003ull^quantize(normal.z);
            return key*1000003ull^offsetKey;
        };
        for(std::size_t triangle=0;triangle+2<indices.size();triangle+=3) {
            const auto a=positions[indices[triangle]],b=positions[indices[triangle+1]],c=positions[indices[triangle+2]];
            const auto normalRaw=cross(sub(b,a),sub(c,a));
            const double doubleArea=length(normalRaw);
            if(doubleArea<=kEpsilon)continue;
            const Vec3 normal=mul(normalRaw,1.0/doubleArea);
            const double area=doubleArea*0.5;
            const Vec3 centre=mul(add(add(a,b),c),1.0/3.0);
            const auto key=keyOf(normal,dot(normal,centre));
            auto found=index.find(key);
            if(found==index.end()) {
                Cluster cluster;cluster.normal=normal;cluster.offset=dot(normal,centre);
                clusters.push_back(cluster);
                found=index.emplace(key,clusters.size()-1).first;
            }
            auto& cluster=clusters[found->second];
            cluster.area+=area;cluster.weight+=area;
            cluster.centroidSum=add(cluster.centroidSum,mul(centre,area));
            cluster.normal=normalized(add(mul(cluster.normal,1.0),mul(normal,0.01)));
        }
        for(const auto& cluster:clusters) {
            if(cluster.area<0.005||cluster.weight<=0)continue;
            const Vec3 centroidCluster=mul(cluster.centroidSum,1.0/cluster.weight);
            const double radius=std::sqrt(cluster.area/3.14159265358979323846);
            // How far the cluster's triangles reach from its own centroid.
            const Vec3 helper=std::abs(cluster.normal.y)<0.9?Vec3{0,1,0}:Vec3{1,0,0};
            const Vec3 u=normalized(cross(helper,cluster.normal));
            const Vec3 v=cross(cluster.normal,u);
            double maximum=0,sum=0;std::size_t counted=0;
            for(std::size_t triangle=0;triangle+2<indices.size();triangle+=3) {
                const auto a=positions[indices[triangle]],b=positions[indices[triangle+1]],c=positions[indices[triangle+2]];
                const auto normalRaw=cross(sub(b,a),sub(c,a));
                const double doubleArea=length(normalRaw);
                if(doubleArea<=kEpsilon)continue;
                const Vec3 normal=mul(normalRaw,1.0/doubleArea);
                const Vec3 centre=mul(add(add(a,b),c),1.0/3.0);
                if(keyOf(normal,dot(normal,centre))!=keyOf(cluster.normal,cluster.offset))continue;
                const auto delta=sub(centre,centroidCluster);
                const double radial=std::hypot(dot(delta,u),dot(delta,v));
                maximum=std::max(maximum,radial);sum+=radial;++counted;
            }
            if(counted<16)continue;
            const double average=sum/double(counted);
            // A disk fills its circle: its triangles reach the radius and average near 2/3 R.
            if(maximum<radius*0.85||maximum>radius*1.15)continue;
            if(average<radius*0.45||average>radius*0.85)continue;
            SocketCandidate socket;
            socket.position=centroidCluster;
            socket.forward=cluster.normal;
            if(dot(socket.forward,sub(socket.position,centroid))<0)socket.forward=mul(socket.forward,-1.0);
            socket.diameter=radius*2;
            socket.radialErrorMeters=std::abs(maximum-radius);
            socket.loopVertices=counted;
            socket.closedLoop=true;
            report.sockets.push_back(socket);
            ++report.planarRegions;
        }
    }

    std::sort(report.sockets.begin(),report.sockets.end(),[](const auto& x,const auto& y){return x.diameter>y.diameter;});
    return report;
}
}
