// SPDX-License-Identifier: GPL-3.0-only
#include "GeometryArchetypes.hpp"
#include "GeometryAnalysis.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace JE::GeometryArchetypes {
namespace {
constexpr double kDegenerate=1e-12;
struct Triangle {std::array<Vec3,3> corners;Vec3 normal{},centre{};double area{},offset{};std::uint32_t source{};};
struct UnionFind {
    std::vector<std::size_t> parent;
    explicit UnionFind(std::size_t n):parent(n){for(std::size_t i=0;i<n;++i)parent[i]=i;}
    std::size_t find(std::size_t x){while(parent[x]!=x){parent[x]=parent[parent[x]];x=parent[x];}return x;}
    void join(std::size_t a,std::size_t b){a=find(a);b=find(b);if(a!=b)parent[b]=a;}
};
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
double clamp01(double value){return value<0?0:(value>1?1:value);}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Vec3 normalized(Vec3 a){const double scale=length(a);return scale>kDegenerate?a*(1.0/scale):Vec3{};}

// Symmetric 3x3 Jacobi eigen decomposition; axes ordered by variance.
std::array<Vec3,3> principalAxes(std::span<const Vec3> points,Vec3 centre) {
    double covariance[3][3]{};
    for(const auto p:points) {
        const double d[3]{p.x-centre.x,p.y-centre.y,p.z-centre.z};
        for(int row=0;row<3;++row)for(int column=0;column<3;++column)covariance[row][column]+=d[row]*d[column];
    }
    const double scale=points.empty()?1:1/double(points.size());
    double a[3][3]{};
    for(int row=0;row<3;++row)for(int column=0;column<3;++column)a[row][column]=covariance[row][column]*scale;
    double v[3][3]{{1,0,0},{0,1,0},{0,0,1}};
    for(int sweep=0;sweep<32;++sweep) {
        if(std::abs(a[0][1])+std::abs(a[0][2])+std::abs(a[1][2])<=1e-18)break;
        for(int p=0;p<2;++p)for(int q=p+1;q<3;++q) {
            if(std::abs(a[p][q])<1e-20)continue;
            const double theta=(a[q][q]-a[p][p])/(2*a[p][q]);
            const double t=(theta>=0?1:-1)/(std::abs(theta)+std::sqrt(theta*theta+1));
            const double c=1/std::sqrt(t*t+1),s=t*c;
            for(int k=0;k<3;++k){const double akp=a[k][p],akq=a[k][q];a[k][p]=c*akp-s*akq;a[k][q]=s*akp+c*akq;}
            for(int k=0;k<3;++k){const double apk=a[p][k],aqk=a[q][k];a[p][k]=c*apk-s*aqk;a[q][k]=s*apk+c*aqk;}
            for(int k=0;k<3;++k){const double vkp=v[k][p],vkq=v[k][q];v[k][p]=c*vkp-s*vkq;v[k][q]=s*vkp+c*vkq;}
        }
    }
    std::array<std::pair<double,Vec3>,3> eigen{};
    for(int index=0;index<3;++index)eigen[index]={a[index][index],normalized({v[0][index],v[1][index],v[2][index]})};
    std::sort(eigen.begin(),eigen.end(),[](const auto& x,const auto& y){return x.first>y.first;});
    return {eigen[0].second,eigen[1].second,eigen[2].second};
}

// Mean distance from sampled vertices to their nearest mirror image, normalised by
// the diagonal. Missing neighbours count as the full diagonal, never as zero.
double mirrorError(std::span<const Vec3> points,double cell,double diagonal,int axis,double planeCoordinate) {
    std::unordered_map<Cell,std::vector<std::uint32_t>,CellHash> grid;
    grid.reserve(points.size()*2);
    for(std::uint32_t index=0;index<points.size();++index)grid[cellOf(points[index],cell)].push_back(index);
    const std::size_t stride=std::max<std::size_t>(1,points.size()/20000);
    double sum=0;std::size_t counted=0;
    for(std::size_t index=0;index<points.size();index+=stride) {
        Vec3 mirrored=points[index];
        if(axis==0)mirrored.x=2*planeCoordinate-mirrored.x;else mirrored.z=2*planeCoordinate-mirrored.z;
        const auto base=cellOf(mirrored,cell);double best=diagonal;
        for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)for(int dz=-1;dz<=1;++dz) {
            const auto found=grid.find({base.x+dx,base.y+dy,base.z+dz});
            if(found==grid.end())continue;
            for(const auto candidate:found->second)best=std::min(best,length(mirrored-points[candidate]));
        }
        sum+=best;++counted;
    }
    return counted?sum/double(counted)/diagonal:1.0;
}

struct Weld {
    std::vector<std::uint32_t> remap; // Original vertex -> welded vertex.
    std::vector<Vec3> points;
};

// Scanned meshes split vertices along UV seams; adjacency must not depend on that.
Weld weld(std::span<const Vec3> positions,double diagonal) {
    const double cell=std::max(1e-9,1e-5*diagonal);
    std::unordered_map<Cell,std::vector<std::uint32_t>,CellHash> grid;
    Weld result;result.remap.resize(positions.size());
    for(std::uint32_t index=0;index<positions.size();++index) {
        const auto base=cellOf(positions[index],cell);
        std::uint32_t found=UINT32_MAX;
        for(int dx=-1;dx<=1&&found==UINT32_MAX;++dx)for(int dy=-1;dy<=1&&found==UINT32_MAX;++dy)for(int dz=-1;dz<=1&&found==UINT32_MAX;++dz) {
            const auto cellIt=grid.find({base.x+dx,base.y+dy,base.z+dz});
            if(cellIt==grid.end())continue;
            for(const auto candidate:cellIt->second)if(length(positions[candidate]-positions[index])<=cell) {found=candidate;break;}
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

const char* name(Class type) {
    switch(type) {
        case Class::Biped:return "Biped";
        case Class::Quadruped:return "Quadruped";
        case Class::Arthropod:return "Arthropod";
        case Class::Axial:return "Axial";
        case Class::HingedPanel:return "HingedPanel";
        case Class::Drawer:return "Drawer";
        case Class::RigidFrame:return "RigidFrame";
        default:return "Unknown";
    }
}

Report classify(std::span<const Vec3> positions,std::span<const std::uint32_t> indices,const Settings& settings) {
    if(indices.empty()||indices.size()%3!=0)throw std::invalid_argument("Archetype classification needs a triangle list");
    if(positions.empty())throw std::invalid_argument("Archetype classification needs vertices");
    if(positions.size()>settings.maximumVertices||indices.size()/3>settings.maximumTriangles)
        throw std::invalid_argument("Archetype classification input exceeds its stated budget");
    for(const auto index:indices)if(index>=positions.size())throw std::invalid_argument("Archetype classification index is out of range");

    Report report;auto& evidence=report.evidence;
    evidence.vertexCount=positions.size();
    Vec3 minimum{positions[0]},maximum{positions[0]},sum{};
    for(const auto p:positions) {
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))throw std::invalid_argument("Archetype classification rejects non-finite vertices");
        minimum={std::min(minimum.x,p.x),std::min(minimum.y,p.y),std::min(minimum.z,p.z)};
        maximum={std::max(maximum.x,p.x),std::max(maximum.y,p.y),std::max(maximum.z,p.z)};
        sum=sum+p;
    }
    const Vec3 centroid=sum*(1.0/double(positions.size()));
    const Vec3 extent=maximum-minimum;
    const double diagonal=length(extent);
    if(!(diagonal>0))throw std::invalid_argument("Archetype classification needs a non-degenerate bounding box");
    evidence.minimum=minimum;evidence.maximum=maximum;evidence.extent=extent;evidence.centroid=centroid;evidence.diagonal=diagonal;

    const auto welded=weld(positions,diagonal);

    std::vector<Triangle> triangles;triangles.reserve(indices.size()/3);
    double totalArea=0;
    for(std::size_t corner=0;corner<indices.size();corner+=3) {
        const auto ia=indices[corner],ib=indices[corner+1],ic=indices[corner+2];
        const auto a=positions[ia],b=positions[ib],c=positions[ic];
        const Vec3 normal=cross(b-a,c-a);const double doubleArea=length(normal);
        if(!(doubleArea>0)||a.x==b.x&&a.y==b.y&&a.z==b.z) {++evidence.degenerateTriangles;continue;}
        Triangle triangle;triangle.corners={a,b,c};triangle.normal=normal*(1.0/doubleArea);triangle.area=doubleArea*0.5;
        triangle.centre=(a+b+c)*(1.0/3.0);triangle.offset=dot(triangle.normal,triangle.centre);triangle.source=std::uint32_t(corner/3);
        totalArea+=triangle.area;triangles.push_back(triangle);
    }
    if(triangles.empty()||!(totalArea>0))throw std::invalid_argument("Archetype classification found no usable triangles");
    evidence.triangleCount=triangles.size();evidence.area=totalArea;

    constexpr double axisCosine=0.966; // ~15 degrees from a world axis.
    double boxArea=0;
    for(const auto& triangle:triangles) {
        const auto n=triangle.normal;
        if(std::abs(n.x)>axisCosine||std::abs(n.y)>axisCosine||std::abs(n.z)>axisCosine)boxArea+=triangle.area;
    }
    evidence.boxiness=boxArea/totalArea;

    const auto axes=principalAxes(positions,centroid);
    std::array<double,3> principalExtent{0,0,0};
    for(const auto p:positions) {
        const double d[3]{p.x-centroid.x,p.y-centroid.y,p.z-centroid.z};
        for(int axis=0;axis<3;++axis) {
            const double projection=std::abs(d[0]*axes[axis].x+d[1]*axes[axis].y+d[2]*axes[axis].z);
            principalExtent[axis]=std::max(principalExtent[axis],projection*2);
        }
    }
    std::sort(principalExtent.begin(),principalExtent.end(),std::greater<double>());
    evidence.thinness=principalExtent[2]/diagonal;
    evidence.elongation=principalExtent[1]>kDegenerate?principalExtent[0]/principalExtent[1]:1;

    // Planar regions over welded adjacency. Tolerances are scan-tolerant on purpose:
    // photogrammetry never produces an exact plane.
    std::vector<std::pair<std::uint64_t,std::size_t>> edges;edges.reserve(triangles.size()*3);
    auto edgeKey=[](std::uint32_t a,std::uint32_t b){const auto low=std::min(a,b),high=std::max(a,b);return (std::uint64_t(high)<<32)|low;};
    for(std::size_t index=0;index<triangles.size();++index) {
        const auto base=triangles[index].source*3;
        for(int corner=0;corner<3;++corner) {
            const auto a=welded.remap[indices[base+std::size_t(corner)]];
            const auto b=welded.remap[indices[base+std::size_t((corner+1)%3)]];
            edges.emplace_back(edgeKey(a,b),index);
        }
    }
    std::sort(edges.begin(),edges.end());
    UnionFind regions(triangles.size());
    const double planarCosine=std::cos(settings.planarAngleRadians);
    const double planarOffset=settings.planarOffsetRelative*diagonal;
    for(std::size_t edge=1;edge<edges.size();++edge) {
        if(edges[edge].first!=edges[edge-1].first)continue;
        const auto first=edges[edge-1].second,second=edges[edge].second;
        if(first==second)continue;
        const auto& x=triangles[first];const auto& y=triangles[second];
        if(std::abs(dot(x.normal,y.normal))<planarCosine)continue;
        if(std::abs(x.offset-y.offset)>planarOffset)continue;
        regions.join(first,second);
    }
    std::unordered_map<std::size_t,std::size_t> regionIndex;std::vector<std::size_t> regionOf(triangles.size());
    std::vector<double> regionArea;std::vector<Vec3> regionNormal;std::vector<std::size_t> regionTriangles;
    for(std::size_t index=0;index<triangles.size();++index) {
        const auto root=regions.find(index);
        auto found=regionIndex.find(root);
        if(found==regionIndex.end()) {
            found=regionIndex.emplace(root,regionArea.size()).first;
            regionArea.push_back(0);regionNormal.push_back({});regionTriangles.push_back(0);
        }
        const auto slot=found->second;regionOf[index]=slot;
        regionArea[slot]+=triangles[index].area;regionTriangles[slot]+=1;
        regionNormal[slot]=regionNormal[slot]+triangles[index].normal*triangles[index].area;
    }
    const double panelFloor=settings.panelMinimumAreaRelative*totalArea;
    std::vector<std::size_t> rankedRegions;double planarArea=0;
    for(std::size_t region=0;region<regionArea.size();++region) {
        if(regionArea[region]<panelFloor)continue;
        rankedRegions.push_back(region);planarArea+=regionArea[region];
    }
    std::sort(rankedRegions.begin(),rankedRegions.end(),[&](auto x,auto y){return regionArea[x]>regionArea[y];});
    if(rankedRegions.size()>8)rankedRegions.resize(8);
    evidence.planarAreaFraction=clamp01(planarArea/totalArea);

    for(const auto region:rankedRegions) {
        Panel panel;panel.area=regionArea[region];panel.triangles=regionTriangles[region];
        panel.normal=normalized(regionNormal[region]);
        Vec3 panelSum{};std::size_t panelCount=0;Vec3 minimumPoint{},maximumPoint{};bool first=true;
        for(std::size_t index=0;index<triangles.size();++index)if(regionOf[index]==region) {
            for(const auto p:triangles[index].corners) {
                if(first){minimumPoint=maximumPoint=p;first=false;}
                else {
                    minimumPoint={std::min(minimumPoint.x,p.x),std::min(minimumPoint.y,p.y),std::min(minimumPoint.z,p.z)};
                    maximumPoint={std::max(maximumPoint.x,p.x),std::max(maximumPoint.y,p.y),std::max(maximumPoint.z,p.z)};
                }
                panelSum=panelSum+p;++panelCount;
            }
        }
        panel.centre=panelCount?panelSum*(1.0/double(panelCount)):Vec3{};
        panel.offset=dot(panel.normal,panel.centre);
        panel.minimum=minimumPoint;panel.maximum=maximumPoint;
        for(std::size_t edge=0;edge<edges.size();++edge) {
            if(regionOf[edges[edge].second]!=region)continue;
            const auto sameKey=[&](std::size_t other){return other<edges.size()&&edges[other].first==edges[edge].first;};
            bool boundary=true;
            if(sameKey(edge+1)&&regionOf[edges[edge+1].second]==region)boundary=false;
            if(edge>0&&sameKey(edge-1)&&regionOf[edges[edge-1].second]==region)boundary=false;
            if(!boundary)continue;
            const auto a=positions[std::uint32_t(edges[edge].first&0xffffffffu)],b=positions[std::uint32_t(edges[edge].first>>32)];
            const double candidate=length(b-a);
            if(candidate>panel.longestEdgeMeters){panel.longestEdgeMeters=candidate;panel.longestEdgeDirection=normalized(b-a);}
        }
        evidence.panels.push_back(panel);
    }

    {
        std::vector<JE::GeometryAnalysis::Vec3> points(positions.size());
        for(std::size_t index=0;index<positions.size();++index)points[index]={positions[index].x,positions[index].y,positions[index].z};
        JE::GeometryAnalysis::Settings analysis;analysis.maximumVertices=settings.maximumVertices;analysis.maximumTriangles=settings.maximumTriangles;
        analysis.weldRelative=1e-5;
        const auto connectivity=JE::GeometryAnalysis::analyze({points,indices},analysis);
        evidence.componentCount=connectivity.components.size();
    }

    // Supports: local minima of Y, clustered. A band alone merges a low body with
    // its feet, which is why the earlier support-only inference saw 2 or 4 legs.
    {
        const double reach=std::max(1e-9,settings.supportClusterRelative*diagonal);
        const double drop=std::max(1e-9,0.004*extent.y);
        std::unordered_map<Cell,std::vector<std::uint32_t>,CellHash> grid;
        for(std::uint32_t index=0;index<positions.size();++index)grid[cellOf(positions[index],reach)].push_back(index);
        std::vector<std::uint32_t> minima;
        for(std::uint32_t index=0;index<positions.size();++index) {
            const auto base=cellOf(positions[index],reach);bool candidate=true;
            for(int dx=-1;dx<=1&&candidate;++dx)for(int dy=-1;dy<=1&&candidate;++dy)for(int dz=-1;dz<=1&&candidate;++dz) {
                const auto found=grid.find({base.x+dx,base.y+dy,base.z+dz});
                if(found==grid.end())continue;
                for(const auto other:found->second)if(positions[other].y<positions[index].y-drop) {candidate=false;break;}
            }
            if(candidate)minima.push_back(index);
        }
        // Greedy separation: one support per spatially distinct minimum. Union-find on
        // a tolerance grid over-split noisy scans (16 supports on an eight-leg spider).
        const double separation=3*reach;
        std::vector<Vec3> supports;
        std::sort(minima.begin(),minima.end(),[&](auto x,auto y){return positions[x].y<positions[y].y;});
        for(const auto index:minima) {
            bool merged=false;
            for(const auto& support:supports)if(length(positions[index]-support)<=separation){merged=true;break;}
            if(!merged)supports.push_back(positions[index]);
        }
        evidence.supportCount=supports.size();
        // Spread of the ground contacts around their own best line: limbs surround a
        // body, while an axial body keeps all of its low points on one corridor.
        if(supports.size()>=2) {
            Vec3 centre{};for(const auto& support:supports)centre=centre+support;
            centre=centre*(1.0/double(supports.size()));
            double covarianceXZ[2][2]{};
            for(const auto& support:supports) {
                const double dx=support.x-centre.x,dz=support.z-centre.z;
                covarianceXZ[0][0]+=dx*dx;covarianceXZ[0][1]+=dx*dz;covarianceXZ[1][1]+=dz*dz;
            }
            const double direction=0.5*std::atan2(2*covarianceXZ[0][1],covarianceXZ[0][0]-covarianceXZ[1][1]);
            const double cx=std::cos(direction),cz=std::sin(direction);
            double spread=0,along=0;
            for(const auto& support:supports) {
                const double dx=support.x-centre.x,dz=support.z-centre.z;
                spread=std::max(spread,std::abs(-dx*cz+dz*cx));
                along=std::max(along,std::abs(dx*cx+dz*cz));
            }
            evidence.supportSpread=along>1e-9?spread/along:0;
        }
        double groundedArea=0;
        const double band=minimum.y+settings.groundBandRelative*extent.y;
        for(const auto& triangle:triangles)if(triangle.centre.y<=band)groundedArea+=triangle.area;
        evidence.groundedAreaFraction=clamp01(groundedArea/totalArea);
    }

    {
        const double cell=std::max(1e-9,settings.symmetryCellRelative*diagonal);
        evidence.symmetryError=std::min(mirrorError(positions,cell,diagonal,0,centroid.x),mirrorError(positions,cell,diagonal,2,centroid.z));
    }

    // Vertical/horizontal area split, measured on triangles rather than merged
    // regions: scanned doors never satisfy an exact-plane merge.
    double verticalArea=0,horizontalArea=0;
    for(const auto& triangle:triangles) {
        if(std::abs(triangle.normal.y)<0.30)verticalArea+=triangle.area;
        if(std::abs(triangle.normal.y)>0.95)horizontalArea+=triangle.area;
    }
    evidence.verticalPanelFraction=verticalArea/totalArea;
    evidence.horizontalPanelFraction=horizontalArea/totalArea;

    // Vertical coverage: how much of the face's own bounding rectangle the panels
    // fill. A leaf fills its rectangle; an empty frame only fills its border.
    {
        const bool xThin=extent.x<=extent.z; // Thin horizontal axis = panel normal.
        const double spanU=xThin?extent.z:extent.x,spanV=extent.y;
        constexpr int resolution=40;
        std::vector<unsigned char> occupied(resolution*resolution,0);
        const auto uOf=[&](Vec3 p){return xThin?p.z:p.x;};
        const auto vOf=[&](Vec3 p){return p.y;};
        const double uMin=xThin?minimum.z:minimum.x;
        // Rasterise the projected triangle instead of sampling its corners: a coarse
        // box would otherwise fill five cells and a dense leaf would fill the grid.
        const auto columnOf=[&](Vec3 p){return (uOf(p)-uMin)/std::max(1e-12,spanU)*resolution;};
        const auto rowOf=[&](Vec3 p){return (vOf(p)-minimum.y)/std::max(1e-12,spanV)*resolution;};
        for(const auto& triangle:triangles) {
            if(std::abs(triangle.normal.y)>=0.30)continue;
            const double u[3]{columnOf(triangle.corners[0]),columnOf(triangle.corners[1]),columnOf(triangle.corners[2])};
            const double v[3]{rowOf(triangle.corners[0]),rowOf(triangle.corners[1]),rowOf(triangle.corners[2])};
            const double area=(u[1]-u[0])*(v[2]-v[0])-(u[2]-u[0])*(v[1]-v[0]);
            if(std::abs(area)<1e-12)continue;
            const int lowColumn=std::clamp(int(std::floor(std::min({u[0],u[1],u[2]}))),0,resolution-1);
            const int highColumn=std::clamp(int(std::ceil(std::max({u[0],u[1],u[2]}))),0,resolution-1);
            const int lowRow=std::clamp(int(std::floor(std::min({v[0],v[1],v[2]}))),0,resolution-1);
            const int highRow=std::clamp(int(std::ceil(std::max({v[0],v[1],v[2]}))),0,resolution-1);
            for(int row=lowRow;row<=highRow;++row)for(int column=lowColumn;column<=highColumn;++column) {
                const double x=column+0.5,y=row+0.5;
                const double w0=((u[1]-x)*(v[2]-y)-(u[2]-x)*(v[1]-y))/area;
                const double w1=((u[2]-x)*(v[0]-y)-(u[0]-x)*(v[2]-y))/area;
                const double w2=1-w0-w1;
                if(w0>=-1e-9&&w1>=-1e-9&&w2>=-1e-9)occupied[std::size_t(row)*resolution+std::size_t(column)]=1;
            }
        }
        std::size_t filled=0;for(const auto cell:occupied)filled+=cell;
        evidence.verticalCoverage=double(filled)/double(resolution*resolution);
    }

    const auto format=[](double value){char text[64];std::snprintf(text,sizeof text,"%.3f",value);return std::string(text);};
    auto add=[&](Class type,double score,std::string reason){report.ranked.push_back({type,clamp01(score),std::move(reason)});};

    const bool furnitureLike=evidence.boxiness>0.45;
    const bool organic=evidence.boxiness<0.28;
    const double axialConfidence=clamp01((1.5-evidence.supportSpread)/0.5)*clamp01((evidence.elongation-1.4)/0.6);

    if(furnitureLike) {
        if(evidence.thinness<0.40&&evidence.verticalPanelFraction>0.50&&evidence.verticalCoverage>0.45)
            add(Class::HingedPanel,0.50+0.25*evidence.verticalPanelFraction+0.15*evidence.verticalCoverage+0.10*(1-evidence.thinness),
                "upright rigid leaf: vertical area "+format(evidence.verticalPanelFraction)+", face coverage "+format(evidence.verticalCoverage)+", thinness "+format(evidence.thinness));
        add(Class::RigidFrame,0.44+0.30*evidence.boxiness+0.15*evidence.verticalCoverage,
            "axis-aligned shell: boxiness "+format(evidence.boxiness)+", coverage "+format(evidence.verticalCoverage)+", supports "+std::to_string(evidence.supportCount));
        if(evidence.thinness<0.15&&evidence.verticalPanelFraction>0.35)
            add(Class::Drawer,0.42+0.25*evidence.verticalPanelFraction,"thin face in a rigid shell: coverage "+format(evidence.verticalCoverage));
    }
    if(organic) {
        const double supports=double(evidence.supportCount);
        // Ground contacts are counted, not guessed: an eight-leg scan splits a foot
        // into several minima, so each family accepts a band and states it.
        const auto creature=[&](Class type,double low,double high,double ideal,double base) {
            const double match=supports<low||supports>high?0.25:clamp01(1.0-std::abs(supports-ideal)/std::max(1.0,ideal));
            add(type,base*match,"supports "+std::to_string(evidence.supportCount)+" inside ["+format(low)+","+format(high)+"] targeting "+format(ideal));
        };
        const double limbFactor=1.0-0.60*axialConfidence;
        creature(Class::Arthropod,6,20,8,(0.60+0.40*organic)*limbFactor);
        creature(Class::Quadruped,3,5,4,(0.55+0.40*organic)*limbFactor);
        creature(Class::Biped,1,3,2,(0.52+0.40*organic)*limbFactor);
        if(evidence.elongation>1.4&&evidence.supportCount>=2)
            add(Class::Axial,0.55+0.35*axialConfidence,
                "elongation "+format(evidence.elongation)+", support spread "+format(evidence.supportSpread)+" with "+std::to_string(evidence.supportCount)+" low points");
    }
    add(Class::Unknown,0.05,"no admissible archetype reached its evidence floor");

    std::stable_sort(report.ranked.begin(),report.ranked.end(),[](const auto& x,const auto& y){return x.score>y.score;});
    if(report.ranked.size()>1)report.ambiguous=(report.ranked[0].score-report.ranked[1].score)<0.05;
    return report;
}
}
