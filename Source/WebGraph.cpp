#include "WebLod.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace Playground::Web
{
namespace
{
Vec3d add(Vec3d a,Vec3d b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3d sub(Vec3d a,Vec3d b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3d mul(Vec3d a,double scale) {return {a.x*scale,a.y*scale,a.z*scale};}
double dot(Vec3d a,Vec3d b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3d cross(Vec3d a,Vec3d b)
{return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double length(Vec3d a) {return std::sqrt(dot(a,a));}
Vec3d unit(Vec3d a)
{
    const auto size=length(a);
    if (!std::isfinite(size) || size<1e-12)
        throw std::runtime_error("Web has an undefined plane axis");
    return mul(a,1/size);
}
bool finite(Vec3d p)
{return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
std::uint32_t mix(std::uint32_t value)
{value^=value>>16;value*=0x7feb352du;value^=value>>15;
 value*=0x846ca68bu;return value^(value>>16);}
double noise(std::uint32_t seed,unsigned a,unsigned b,unsigned tag)
{return double(mix(seed^mix(a+0x9e3779b9u)^mix(b+0x85ebca6bu)^tag))/4294967295.0;}
struct Point2 {double x{},y{};};
Point2 local(Vec3d p,Vec3d center,Vec3d u,Vec3d v)
{const auto d=sub(p,center);return {dot(d,u),dot(d,v)};}
double rayBoundary(Point2 hub,Point2 direction,const std::vector<Point2> &polygon)
{
    double closest=std::numeric_limits<double>::infinity();
    for (std::size_t i=0;i<polygon.size();++i)
    {
        const auto a=polygon[i],b=polygon[(i+1)%polygon.size()];
        const Point2 e{b.x-a.x,b.y-a.y},relative{a.x-hub.x,a.y-hub.y};
        const auto determinant=direction.x*e.y-direction.y*e.x;
        if (std::abs(determinant)<1e-12) continue;
        const auto t=(relative.x*e.y-relative.y*e.x)/determinant;
        const auto edgeT=(relative.x*direction.y-relative.y*direction.x)/determinant;
        if (t>1e-5 && edgeT>=-1e-8 && edgeT<=1+1e-8)
            closest=std::min(closest,t);
    }
    if (!std::isfinite(closest)) throw std::runtime_error("Web hub must lie inside the authored area");
    return closest;
}
}
std::uint32_t edgeToken(const std::string &id)
{
    std::uint32_t hash=2166136261u;
    for (unsigned char value:id)
    {hash^=value;hash*=16777619u;}
    return hash?hash:1u;
}
Graph makeGraph(const Settings &settings)
{
    if (settings.spokes<4 || settings.spokes>64 || settings.rings<1 ||
        settings.rings>12 || settings.radiusMeters<=0 ||
        !std::isfinite(settings.radiusMeters) || settings.radiusMeters>.2 ||
        !std::isfinite(settings.warpMeters) || std::abs(settings.warpMeters)>1 ||
        !std::isfinite(settings.widthMeters) || !std::isfinite(settings.heightMeters) ||
        settings.widthMeters<=0 || settings.heightMeters<=0 ||
        settings.widthMeters>100 || settings.heightMeters>100 ||
        !finite(settings.center) || !finite(settings.axisU) || !finite(settings.axisV) ||
        !std::isfinite(settings.density) || settings.density<0 || settings.density>1 ||
        !std::isfinite(settings.secondaryStrands) || settings.secondaryStrands<0 ||
        settings.secondaryStrands>1 || !std::isfinite(settings.irregularity) ||
        settings.irregularity<0 || settings.irregularity>.4 ||
        !std::isfinite(settings.microStrandDensity) ||
        settings.microStrandDensity<0 || settings.microStrandDensity>1 ||
        settings.structuralStrandCount>64 ||
        !std::isfinite(settings.gravityInfluence) ||
        settings.gravityInfluence<0 || settings.gravityInfluence>4 ||
        !std::isfinite(settings.missingFraction) || settings.missingFraction<0 ||
        settings.missingFraction>1 || !std::isfinite(settings.sagMeters) ||
        settings.sagMeters<0 || settings.sagMeters>2 ||
        !std::isfinite(settings.hubOffsetU) || !std::isfinite(settings.hubOffsetV) ||
        std::abs(settings.hubOffsetU)>.25 || std::abs(settings.hubOffsetV)>.25)
        throw std::runtime_error("Web recipe has invalid dimensions, controls or budgets");
    const auto axisU=unit(settings.axisU);
    const auto axisV=unit(sub(settings.axisV,mul(axisU,dot(settings.axisV,axisU))));
    const auto normal=unit(cross(axisU,axisV));
    Vec3d origin=settings.center;
    std::vector<Point2> polygon;
    if (settings.authoring==Authoring::Plane)
        polygon={{-settings.widthMeters*.5,-settings.heightMeters*.5},
                 {settings.widthMeters*.5,-settings.heightMeters*.5},
                 {settings.widthMeters*.5,settings.heightMeters*.5},
                 {-settings.widthMeters*.5,settings.heightMeters*.5}};
    else
    {
        const auto &points=settings.authoring==Authoring::Anchors?settings.anchors:settings.area;
        if (points.size()<3 || points.size()>32)
            throw std::runtime_error("Anchor/area web requires 3 to 32 points");
        Graph boundary;
        boundary.nodes=points;
        for (unsigned i=0;i<points.size();++i)
        {
            if (!finite(points[i])) throw std::runtime_error("Web anchor is non-finite");
            boundary.edges.push_back({"Boundary_"+std::to_string(i),i,
                unsigned((i+1)%points.size()),0,settings.radiusMeters});
        }
        const auto fit=fitPlane(boundary,0,0);
        if (fit.maximumPlaneErrorMeters>settings.maximumPlaneErrorMeters)
            throw std::runtime_error("Authored points exceed the local plane tolerance");
        origin=fit.center;
        polygon.reserve(points.size());
        for (auto point:points) polygon.push_back(local(point,origin,fit.axisU,fit.axisV));
    }
    Vec3d u=axisU,v=axisV,n=normal;
    if (settings.authoring!=Authoring::Plane)
    {
        Graph boundary;
        const auto &points=settings.authoring==Authoring::Anchors?settings.anchors:settings.area;
        boundary.nodes=points;
        for (unsigned i=0;i<points.size();++i)
            boundary.edges.push_back({"B",i,unsigned((i+1)%points.size()),0,settings.radiusMeters});
        const auto fit=fitPlane(boundary,0,0);
        u=fit.axisU;v=fit.axisV;n=fit.normal;
    }
    const Point2 hub{settings.hubOffsetU*settings.widthMeters,
                     settings.hubOffsetV*settings.heightMeters};
    if (settings.authoring!=Authoring::Plane &&
        (std::abs(settings.hubOffsetU)>1e-9 || std::abs(settings.hubOffsetV)>1e-9))
        rayBoundary(hub,{1,0},polygon);
    Graph graph;
    const unsigned patches=settings.preset==Preset::Corner?2:1;
    for (unsigned patch=0;patch<patches;++patch)
    {
        const auto patchU=patch?n:u;
        const auto patchV=v;
        const auto patchNormal=patch?mul(u,-1):n;
        const std::string prefix=patch?"Web/CornerB/":"Web/CornerA/";
        if (settings.authoring==Authoring::Area)
        {
            graph.areaMasks.emplace_back();
            for (auto point:polygon)
                graph.areaMasks.back().push_back(add(origin,
                    add(mul(patchU,point.x),mul(patchV,point.y))));
        }
        Point2 localHub=hub;
        if (settings.preset==Preset::Corner)
            localHub.x+=(patch?.18:-.18)*settings.widthMeters;
        const auto hubNode=std::uint32_t(graph.nodes.size());
        graph.nodes.push_back(add(origin,add(mul(patchU,localHub.x),
                                             mul(patchV,localHub.y))));
        graph.nodeIds.push_back(prefix+"Hub");
        std::vector<std::vector<std::uint32_t>> rings(settings.rings);
        for (unsigned ring=0;ring<settings.rings;++ring)
        {
            for (unsigned spoke=0;spoke<settings.spokes;++spoke)
            {
                const double angle=2*std::numbers::pi*spoke/settings.spokes;
                const Point2 ray{std::cos(angle),std::sin(angle)};
                const auto boundary=rayBoundary(localHub,ray,polygon);
                const double presetWarp=settings.preset==Preset::Messy?.27:
                    settings.preset==Preset::Old?.18:settings.irregularity;
                const auto jitter=(noise(settings.seed,spoke,ring,23+patch)-.5)*
                    2*std::max(settings.irregularity,presetWarp);
                const double radial=std::clamp(double(ring+1)/(settings.rings+1)+jitter,
                    .02,.98)*boundary;
                const double x=localHub.x+radial*ray.x,y=localHub.y+radial*ray.y;
                const double warp=settings.warpMeters*
                    std::sin((spoke+1)*1.732+(ring+1)*2.618);
                Vec3d point=add(origin,add(add(mul(patchU,x),mul(patchV,y)),
                    mul(patchNormal,warp)));
                rings[ring].push_back(std::uint32_t(graph.nodes.size()));
                graph.nodes.push_back(point);
                graph.nodeIds.push_back(prefix+"Node_"+std::to_string(ring)+"_"+
                    std::to_string(spoke));
            }
        }
        for (unsigned ring=0;ring<settings.rings;++ring)
            for (unsigned spoke=0;spoke<settings.spokes;++spoke)
            {
                const auto node=rings[ring][spoke];
                const auto prior=ring?rings[ring-1][spoke]:hubNode;
                const auto next=rings[ring][(spoke+1)%settings.spokes];
                auto append=[&](const char *role,unsigned index,std::uint32_t a,
                                std::uint32_t b,double thickness,double chance)
                {
                    Edge edge{prefix+role+std::to_string(ring)+"_"+
                        std::to_string(index),a,b,patch,thickness};
                    const auto random=noise(settings.seed,patch*1000+ring,index,
                        role[0]=='R'?47:71);
                    const auto missing=std::max(settings.missingFraction,
                        settings.preset==Preset::Old?.22:0.0);
                    if (random<missing) edge.state=EdgeState::Missing;
                    else if (random>chance) edge.state=EdgeState::Disabled;
                    if (settings.sagMeters>0)
                    {
                        edge.sagged=true;
                        edge.sagPoint=sub(mul(add(graph.nodes[a],graph.nodes[b]),.5),
                            mul(patchV,settings.sagMeters));
                    }
                    graph.edges.push_back(std::move(edge));
                };
                const auto variation=.7+.6*noise(settings.seed,spoke,ring,97+patch);
                append("Spoke_",spoke,prior,node,settings.radiusMeters*variation,1);
                append("Ring_",spoke,node,next,settings.radiusMeters*.75*variation,
                    settings.preset==Preset::Trap?.15:settings.density);
                const auto secondary=std::max(settings.secondaryStrands,
                    settings.preset==Preset::Dense?.5:settings.preset==Preset::Messy?.35:0.0);
                if (noise(settings.seed,ring,spoke,131+patch)<secondary)
                    append("Secondary_",spoke,node,
                        rings[ring][(spoke+2)%settings.spokes],
                        settings.radiusMeters*.55*variation,1);
                if (noise(settings.seed,ring,spoke,173+patch)<settings.microStrandDensity)
                    append("Micro_",spoke,node,
                        rings[ring][(spoke+3)%settings.spokes],
                        settings.radiusMeters*.18*variation,1);
            }
    }
    if (graph.edges.size()>4096)
    {
        const auto optional=[](const Edge &edge)
        {return edge.id.find("/Secondary_")!=std::string::npos ||
                edge.id.find("/Micro_")!=std::string::npos;};
        const auto primary=std::count_if(graph.edges.begin(),graph.edges.end(),
            [&](const Edge &edge){return !optional(edge);});
        if (primary>4096) throw std::runtime_error("Web primary edge budget exceeded");
        const auto extras=graph.edges.size()-primary;
        const auto capacity=4096-primary;
        std::vector<Edge> selected;
        selected.reserve(4096);
        std::size_t seen{};
        for (auto &edge:graph.edges)
        {
            if (!optional(edge))
            {selected.push_back(std::move(edge));continue;}
            const auto keep=((seen+1)*capacity)/extras>(seen*capacity)/extras;
            ++seen;
            if (keep) selected.push_back(std::move(edge));
        }
        graph.edges=std::move(selected);
    }
    return graph;
}
CardPatch fitPlane(const Graph &graph,unsigned patch,double padding)
{
    if (graph.nodes.empty() || graph.edges.empty() || !std::isfinite(padding) ||
        padding<0 || padding>10)
        throw std::runtime_error("Invalid web graph or card padding");
    std::vector<bool> selected(graph.nodes.size());
    for (const auto &edge:graph.edges)
    {
        if (edge.start>=graph.nodes.size() || edge.end>=graph.nodes.size() ||
            edge.start==edge.end)
            throw std::runtime_error("Web edge references invalid nodes");
        if (patch==std::numeric_limits<unsigned>::max() || edge.patch==patch)
        {selected[edge.start]=true;selected[edge.end]=true;}
    }
    Vec3d center{};
    std::size_t count{};
    for (std::size_t i=0;i<selected.size();++i)
        if (selected[i]) {center=add(center,graph.nodes[i]);++count;}
    if (count<3) throw std::runtime_error("Web card patch needs three points");
    center=mul(center,1/double(count));
    double matrix[3][3]{};
    for (std::size_t i=0;i<selected.size();++i)
    {
        if (!selected[i]) continue;
        const auto delta=sub(graph.nodes[i],center);
        const double values[]={delta.x,delta.y,delta.z};
        for (unsigned row=0;row<3;++row)
            for (unsigned column=0;column<3;++column)
                matrix[row][column]+=values[row]*values[column];
    }
    double eigenvectors[3][3]={{1,0,0},{0,1,0},{0,0,1}};
    for (unsigned iteration=0;iteration<48;++iteration)
    {
        unsigned first=0,second=1;
        for (unsigned row=0;row<3;++row)
            for (unsigned column=row+1;column<3;++column)
                if (std::abs(matrix[row][column])>
                    std::abs(matrix[first][second]))
                {first=row;second=column;}
        if (std::abs(matrix[first][second])<1e-12) break;
        const double angle=.5*std::atan2(2*matrix[first][second],
            matrix[second][second]-matrix[first][first]);
        const double c=std::cos(angle),s=std::sin(angle);
        const double a=matrix[first][first],b=matrix[first][second],
                     d=matrix[second][second];
        matrix[first][first]=c*c*a-2*s*c*b+s*s*d;
        matrix[second][second]=s*s*a+2*s*c*b+c*c*d;
        matrix[first][second]=matrix[second][first]=0;
        for (unsigned index=0;index<3;++index)
        {
            if (index!=first && index!=second)
            {
                const auto x=matrix[index][first],y=matrix[index][second];
                matrix[index][first]=matrix[first][index]=c*x-s*y;
                matrix[index][second]=matrix[second][index]=s*x+c*y;
            }
            const auto x=eigenvectors[index][first],y=eigenvectors[index][second];
            eigenvectors[index][first]=c*x-s*y;
            eigenvectors[index][second]=s*x+c*y;
        }
    }
    unsigned smallest=0;
    for (unsigned i=1;i<3;++i)
        if (matrix[i][i]<matrix[smallest][smallest]) smallest=i;
    auto normal=unit({eigenvectors[0][smallest],eigenvectors[1][smallest],
                      eigenvectors[2][smallest]});
    const auto components=std::array<double,3>{normal.x,normal.y,normal.z};
    unsigned dominant=0;
    for (unsigned i=1;i<3;++i)
        if (std::abs(components[i])>std::abs(components[dominant])) dominant=i;
    if (components[dominant]<0) normal=mul(normal,-1);
    Vec3d axisU{};
    for (const auto &edge:graph.edges)
        if (patch==std::numeric_limits<unsigned>::max() || edge.patch==patch)
        {
            auto direction=sub(graph.nodes[edge.end],graph.nodes[edge.start]);
            direction=sub(direction,mul(normal,dot(direction,normal)));
            if (length(direction)>1e-8) {axisU=unit(direction);break;}
        }
    if (length(axisU)<1e-8)
        throw std::runtime_error("Web card has no in-plane axis");
    const auto axisV=unit(cross(normal,axisU));
    CardPatch result;
    result.center=center;result.axisU=axisU;result.axisV=axisV;result.normal=normal;
    result.index=patch;
    result.minU=result.minV=std::numeric_limits<double>::infinity();
    result.maxU=result.maxV=-std::numeric_limits<double>::infinity();
    double squared{};
    for (std::size_t i=0;i<selected.size();++i)
    {
        if (!selected[i]) continue;
        const auto delta=sub(graph.nodes[i],center);
        const auto distance=dot(delta,normal);
        result.maximumPlaneErrorMeters=
            std::max(result.maximumPlaneErrorMeters,std::abs(distance));
        squared+=distance*distance;
        const auto u=dot(delta,axisU),v=dot(delta,axisV);
        result.minU=std::min(result.minU,u);
        result.maxU=std::max(result.maxU,u);
        result.minV=std::min(result.minV,v);
        result.maxV=std::max(result.maxV,v);
    }
    result.rmsPlaneErrorMeters=std::sqrt(squared/count);
    result.minU-=padding;result.maxU+=padding;
    result.minV-=padding;result.maxV+=padding;
    if (!(result.maxU-result.minU>1e-5 && result.maxV-result.minV>1e-5))
        throw std::runtime_error("Web card projection collapsed");
    return result;
}
}
