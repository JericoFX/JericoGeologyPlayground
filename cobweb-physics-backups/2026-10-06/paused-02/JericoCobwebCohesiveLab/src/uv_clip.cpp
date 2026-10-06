#include "uv_clip.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <tuple>
#include <utility>

namespace web::uvclip {
namespace {
using Real=long double;
// Dimensionless coordinates: source bounding box has longest side 1.
constexpr Real weldTolerance=1e-12L;
constexpr Real minArea=1e-24L;
struct Point { Real x=0,y=0; Point operator+(Point b)const{return{x+b.x,y+b.y};} Point operator-(Point b)const{return{x-b.x,y-b.y};} Point operator*(Real s)const{return{x*s,y*s};} };
using Polygon=std::vector<Point>;
Real cross(Point a,Point b){return a.x*b.y-a.y*b.x;}
Real dot(Point a,Point b){return a.x*b.x+a.y*b.y;}
Real norm2(Point a){return dot(a,a);}
Real side(Point a,Point b,Point p){
    Point d=b-a,q=p-a;
    // Include coordinate-subtraction roundoff, not only multiplication.
    // Otherwise a previously generated on-line point can turn into a tiny
    // spurious outside fragment when another collinear cut edge is visited.
    Real magnitude=std::abs(d.x)*(std::abs(p.y)+std::abs(a.y))
        +std::abs(d.y)*(std::abs(p.x)+std::abs(a.x))
        +std::abs(q.y)*(std::abs(b.x)+std::abs(a.x))
        +std::abs(q.x)*(std::abs(b.y)+std::abs(a.y));
    Real v=cross(d,q),roundoff=32*std::numeric_limits<Real>::epsilon()*magnitude;
    return std::abs(v)<=roundoff?0:v;
}
Real signedArea(const Polygon& p){
    if(p.size()<3)return 0;
    Real a=0;for(std::size_t i=1;i+1<p.size();++i)a+=cross(p[i]-p[0],p[i+1]-p[0]);return a*.5L;
}
bool same(Point a,Point b){return a.x==b.x&&a.y==b.y;}
void clean(Polygon& p){
    Polygon q;for(auto v:p)if(q.empty()||!same(q.back(),v))q.push_back(v);
    if(q.size()>1&&same(q.front(),q.back()))q.pop_back();
    p=std::move(q);
}
struct Bounds {
    Real x0=std::numeric_limits<Real>::infinity(),y0=x0,x1=-x0,y1=-x0;
    void add(Point p){x0=std::min(x0,p.x);y0=std::min(y0,p.y);x1=std::max(x1,p.x);y1=std::max(y1,p.y);}
    bool interiorOverlaps(const Bounds& b)const{return x0<b.x1&&x1>b.x0&&y0<b.y1&&y1>b.y0;}
};
Bounds bounds(const Polygon& p){Bounds b;for(auto v:p)b.add(v);return b;}
// Split once, sharing the very same computed intersections between both sides.
std::pair<Polygon,Polygon> split(const Polygon& p,Point a,Point b){
    Polygon in,out;
    for(std::size_t i=0;i<p.size();++i){
        Point u=p[i],v=p[(i+1)%p.size()];Real du=side(a,b,u),dv=side(a,b,v);
        if(du>=0)in.push_back(u);
        if(du<=0)out.push_back(u);
        if((du>0&&dv<0)||(du<0&&dv>0)){
            Point x=u+(v-u)*(du/(du-dv));in.push_back(x);out.push_back(x);
        }
    }
    clean(in);clean(out);return{std::move(in),std::move(out)};
}
Polygon intersection(Polygon p,const Polygon& clip){
    for(std::size_t k=0;k<clip.size()&&p.size()>=3;++k)p=split(p,clip[k],clip[(k+1)%clip.size()]).first;
    return p;
}
void fail(ErrorCode code,const std::string& text){throw ClipError(code,"UV clip: "+text);}
double areaMetric(Real normalized,Real scale){
    Real area=normalized*scale*scale;
    double value=double(area);
    if(!std::isfinite(value)||(area>0&&value==0))
        fail(ErrorCode::NumericalFailure,"UV area cannot be represented by the result's double metrics");
    return value;
}
void budget(std::size_t count,std::size_t cap,const char* what){
    if(count>cap)fail(ErrorCode::BudgetExceeded,std::string(what)+" budget exceeded ("+std::to_string(count)+" > "+std::to_string(cap)+"); source unchanged");
}
bool finite(V2 v){return std::isfinite(v.x)&&std::isfinite(v.y);}
bool finite(V3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
Real norm3(V3 v){return std::sqrt(Real(v.x)*v.x+Real(v.y)*v.y+Real(v.z)*v.z);}
Real crossNorm(V3 a,V3 b,V3 c){
    Real x=Real(b.x)-a.x,y=Real(b.y)-a.y,z=Real(b.z)-a.z;
    Real u=Real(c.x)-a.x,v=Real(c.y)-a.y,w=Real(c.z)-a.z;
    return std::sqrt((y*w-z*v)*(y*w-z*v)+(z*u-x*w)*(z*u-x*w)+(x*v-y*u)*(x*v-y*u));
}
struct DisjointSet {
    std::vector<std::size_t> p;
    explicit DisjointSet(std::size_t n):p(n){std::iota(p.begin(),p.end(),0);}
    std::size_t root(std::size_t a){while(p[a]!=a){p[a]=p[p[a]];a=p[a];}return a;}
    void join(std::size_t a,std::size_t b){a=root(a);b=root(b);p[std::max(a,b)]=std::min(a,b);}
};
struct Coordinates {
    Real x=0,y=0,scale=1;
    Point convert(V2 p)const{return{(Real(p.x)-x)/scale,(Real(p.y)-y)/scale};}
};
struct SourceFace {Polygon polygon;Bounds box;std::size_t chart=0;int winding=1;};
struct Validated {
    Coordinates coordinates;
    std::vector<SourceFace> faces;
    std::vector<Polygon> cuts;
    std::vector<Bounds> cutBounds;
    Real totalArea=0;
};
Validated validate(const Mesh& source,const std::vector<ConvexCut>& cuts,const Limits& limits){
    if(limits.maxTriangles==0||limits.maxVertices==0||limits.maxWorkingPolygons==0)
        fail(ErrorCode::InvalidInput,"budgets must be positive");
    if(limits.maxVertices>std::size_t(std::numeric_limits<int>::max()))
        fail(ErrorCode::InvalidInput,"vertex budget must fit Mesh's signed int indices");
    if(source.vertices.size()>std::size_t(std::numeric_limits<int>::max()))
        fail(ErrorCode::InvalidInput,"source vertex count exceeds signed int indices");
    Validated v;Bounds box;
    for(const auto& vert:source.vertices){
        if(!finite(vert.p)||!finite(vert.n)||!finite(vert.uv))fail(ErrorCode::InvalidInput,"source contains nonfinite position, normal or UV");
    }
    for(const auto& f:source.faces){
        for(int i:f)if(i<0||std::size_t(i)>=source.vertices.size())fail(ErrorCode::InvalidInput,"source face has out-of-range vertex index");
        for(int i:f)box.add({source.vertices[i].uv.x,source.vertices[i].uv.y});
    }
    if(!source.faces.empty()){
        v.coordinates={box.x0,box.y0,std::max(box.x1-box.x0,box.y1-box.y0)};
        if(!(v.coordinates.scale>0))fail(ErrorCode::InvalidInput,"source UV extent is zero");
    }
    DisjointSet charts(source.faces.size());
    std::map<std::pair<int,int>,std::vector<std::size_t>> edges;
    for(std::size_t i=0;i<source.faces.size();++i){
        const auto& f=source.faces[i];SourceFace face;
        for(int a:f)face.polygon.push_back(v.coordinates.convert(source.vertices[a].uv));
        Real a=signedArea(face.polygon);
        if(std::abs(a)<=minArea)fail(ErrorCode::InvalidInput,"source face "+std::to_string(i)+" has zero or unresolved UV area");
        if(!(crossNorm(source.vertices[f[0]].p,source.vertices[f[1]].p,source.vertices[f[2]].p)>0))
            fail(ErrorCode::InvalidInput,"source face "+std::to_string(i)+" has zero 3D area");
        face.winding=a>0?1:-1;if(a<0)std::swap(face.polygon[1],face.polygon[2]);
        face.box=bounds(face.polygon);v.totalArea+=std::abs(a);v.faces.push_back(std::move(face));
        for(int j=0;j<3;++j){int x=f[j],y=f[(j+1)%3];if(x>y)std::swap(x,y);auto& adjacent=edges[{x,y}];adjacent.push_back(i);
            if(adjacent.size()>2)fail(ErrorCode::InvalidInput,"source has non-manifold indexed edge");
            if(adjacent.size()==2)charts.join(adjacent[0],i);
        }
    }
    for(std::size_t i=0;i<v.faces.size();++i)v.faces[i].chart=charts.root(i);
    // UV embedding must be single-valued: no positive-area source overlap,
    // including between separate charts. Boundary contact is permitted.
    for(std::size_t i=0;i<v.faces.size();++i)for(std::size_t j=0;j<i;++j){
        if(!v.faces[i].box.interiorOverlaps(v.faces[j].box))continue;
        Real overlap=std::abs(signedArea(intersection(v.faces[i].polygon,v.faces[j].polygon)));
        if(overlap>minArea)fail(ErrorCode::InvalidInput,"source UV triangles overlap: faces "+std::to_string(j)+" and "+std::to_string(i));
    }
    for(std::size_t ci=0;ci<cuts.size();++ci){
        Polygon cut;for(auto p:cuts[ci]){if(!finite(p))fail(ErrorCode::InvalidInput,"cut "+std::to_string(ci)+" has nonfinite coordinates");cut.push_back(v.coordinates.convert(p));}
        if(cut.size()<3||signedArea(cut)<=minArea)fail(ErrorCode::InvalidInput,"cut "+std::to_string(ci)+" must have positive area and CCW winding");
        for(std::size_t i=0;i<cut.size();++i)for(std::size_t j=0;j<i;++j)
            if(same(cut[i],cut[j]))fail(ErrorCode::InvalidInput,"cut has duplicate vertices (do not repeat closing vertex)");
        for(std::size_t i=0;i<cut.size();++i){
            Point a=cut[i],b=cut[(i+1)%cut.size()];
            for(auto p:cut)if(side(a,b,p)<0)fail(ErrorCode::InvalidInput,"cut "+std::to_string(ci)+" is not a convex CCW polygon");
            Real width=0;for(auto p:cut)width=std::max(width,side(a,b,p)/std::sqrt(norm2(b-a)));
            if(width<=32*weldTolerance)fail(ErrorCode::NumericalFailure,"cut width is below supported UV resolution; use a wider finite-width polygon");
        }
        v.cutBounds.push_back(bounds(cut));v.cuts.push_back(std::move(cut));
    }
    return v;
}
Barycentric barycentric(const Mesh& source,std::size_t fi,Point q,const Coordinates& c){
    const auto& f=source.faces[fi];Point a=c.convert(source.vertices[f[0]].uv),b=c.convert(source.vertices[f[1]].uv),d=c.convert(source.vertices[f[2]].uv);
    Real denom=cross(b-a,d-a),w1=cross(q-a,d-a)/denom,w2=cross(b-a,q-a)/denom,w0=1-w1-w2;
    return{double(w0),double(w1),double(w2)};
}
Vertex interpolate(const Mesh& source,std::size_t fi,Point q,const Coordinates& c){
    auto w=barycentric(source,fi,q,c);const auto& f=source.faces[fi];Vertex v{};
    // Long-double accumulation also makes rigid-transform checks less sensitive
    // to cancellation. Normal is linear here, deliberately not renormalized.
    // Anchored affine form preserves constants exactly and avoids multiplying
    // DBL_MAX by three independently rounded weights whose sum may exceed one.
    auto combine=[&](double a,double b,double d){return double(Real(a)+(Real(b)-a)*w[1]+(Real(d)-a)*w[2]);};
    const auto& a=source.vertices[f[0]];const auto& b=source.vertices[f[1]];const auto& d=source.vertices[f[2]];
    v.p={combine(a.p.x,b.p.x,d.p.x),combine(a.p.y,b.p.y,d.p.y),combine(a.p.z,b.p.z,d.p.z)};
    v.n={combine(a.n.x,b.n.x,d.n.x),combine(a.n.y,b.n.y,d.n.y),combine(a.n.z,b.n.z,d.n.z)};
    v.uv={combine(a.uv.x,b.uv.x,d.uv.x),combine(a.uv.y,b.uv.y,d.uv.y)};
    if(!finite(v.p)||!finite(v.n)||!finite(v.uv))
        fail(ErrorCode::NumericalFailure,"interpolated attribute is not finite in double storage");
    if(norm2(c.convert(v.uv)-q)>weldTolerance*weldTolerance)
        fail(ErrorCode::NumericalFailure,"generated UV cannot be represented accurately in double storage");
    return v;
}
struct Piece {std::size_t face;Polygon polygon;};
struct WeldedPoint {Point q;Vertex vertex;std::size_t chart;int original=-1;};
class Registry {
public:
    std::vector<WeldedPoint> points;
    int insert(Point q,Vertex vertex,std::size_t chart,int original=-1){
        std::int64_t x=std::int64_t(std::floor(q.x/weldTolerance)),y=std::int64_t(std::floor(q.y/weldTolerance));
        int match=-1;
        for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy){
            auto it=buckets.find({chart,x+dx,y+dy});if(it==buckets.end())continue;
            for(int id:it->second)if(norm2(points[id].q-q)<=weldTolerance*weldTolerance&&(match<0||id<match))match=id;
        }
        if(match>=0){
            if(original>=0&&points[match].original>=0&&points[match].original!=original)
                fail(ErrorCode::InvalidInput,"distinct source vertices coincide within one UV chart; preserve/split seam sectors before clipping");
            const auto& old=points[match].vertex;
            Real scale=1+std::max(norm3(old.p),norm3(vertex.p));
            if(norm3(old.p-vertex.p)>1e-9L*scale||norm3(old.n-vertex.n)>1e-9L*(1+std::max(norm3(old.n),norm3(vertex.n))))
                fail(ErrorCode::NumericalFailure,"coincident UV vertices disagree in position/normal within a source chart");
            return match;
        }
        if(points.size()>=std::size_t(std::numeric_limits<int>::max()))fail(ErrorCode::BudgetExceeded,"intermediate vertex indices overflow");
        int id=int(points.size());points.push_back({q,vertex,chart,original});buckets[{chart,x,y}].push_back(id);return id;
    }
private:
    std::map<std::tuple<std::size_t,std::int64_t,std::int64_t>,std::vector<int>> buckets;
};
struct IndexedPiece {std::size_t face;std::vector<int> vertices;};
Result identity(const Mesh& source,const Validated& v,const Limits& limits){
    budget(source.faces.size(),limits.maxTriangles,"triangle");budget(source.vertices.size(),limits.maxVertices,"vertex");
    Result result;result.mesh=source;result.sourceUVArea=result.retainedUVArea=areaMetric(v.totalArea,v.coordinates.scale);
    for(std::size_t i=0;i<source.faces.size();++i){result.sourceFaces.push_back(i);result.sourceBarycentrics.push_back({Barycentric{1,0,0},Barycentric{0,1,0},Barycentric{0,0,1}});}return result;
}
}
Result subtract(const Mesh& source,const std::vector<ConvexCut>& cuts,const Limits& limits){
    auto v=validate(source,cuts,limits);
    if(cuts.empty()||source.faces.empty())return identity(source,v,limits);
    std::vector<Piece> pieces;std::size_t affected=0;
    for(std::size_t fi=0;fi<v.faces.size();++fi){
        std::vector<Polygon> survivors{v.faces[fi].polygon};bool changed=false;
        for(std::size_t ci=0;ci<v.cuts.size()&&!survivors.empty();++ci){
            if(!v.faces[fi].box.interiorOverlaps(v.cutBounds[ci]))continue;
            std::vector<Polygon> next;
            for(auto& polygon:survivors){
                if(!bounds(polygon).interiorOverlaps(v.cutBounds[ci])||signedArea(intersection(polygon,v.cuts[ci]))<=minArea){next.push_back(std::move(polygon));continue;}
                changed=true;Polygon candidate=std::move(polygon);
                for(std::size_t k=0;k<v.cuts[ci].size()&&candidate.size()>=3;++k){
                    auto halves=split(candidate,v.cuts[ci][k],v.cuts[ci][(k+1)%v.cuts[ci].size()]);
                    if(signedArea(halves.second)>minArea)next.push_back(std::move(halves.second));
                    candidate=std::move(halves.first);
                    budget(pieces.size()+next.size(),limits.maxWorkingPolygons,"working polygon");
                }
                // Remaining candidate lies inside this cut and is discarded.
            }
            survivors=std::move(next);
        }
        if(changed)++affected;
        for(auto& p:survivors){pieces.push_back({fi,std::move(p)});budget(pieces.size(),limits.maxWorkingPolygons,"working polygon");}
    }
    if(affected==0)return identity(source,v,limits);
    Result result;result.affectedSourceFaces=affected;result.sourceUVArea=areaMetric(v.totalArea,v.coordinates.scale);
    Registry registry;
    // Seed original vertices first, preserving their exact p/n/UV bits when
    // clipping reaches them. Do not weld separate indexed-edge source charts.
    std::map<std::size_t,std::set<int>> originals;
    for(std::size_t fi=0;fi<v.faces.size();++fi)for(int index:source.faces[fi])originals[v.faces[fi].chart].insert(index);
    for(const auto& entry:originals)for(int id:entry.second)registry.insert(v.coordinates.convert(source.vertices[id].uv),source.vertices[id],entry.first,id);
    std::vector<IndexedPiece> indexed;
    for(const auto& piece:pieces){IndexedPiece p{piece.face,{}};for(auto q:piece.polygon){int id=registry.insert(q,interpolate(source,piece.face,q,v.coordinates),v.faces[piece.face].chart);if(p.vertices.empty()||p.vertices.back()!=id)p.vertices.push_back(id);}if(p.vertices.size()>1&&p.vertices.front()==p.vertices.back())p.vertices.pop_back();if(p.vertices.size()<3)fail(ErrorCode::NumericalFailure,"positive-area survivor collapsed during UV welding");indexed.push_back(std::move(p));}
    // All original and generated boundary points now exist. Insert points on
    // every collinear piece edge, preventing T-junctions between source faces
    // or between different convex pieces of the same source face.
    std::map<std::size_t,std::vector<int>> chartPoints;
    for(int i=0;i<int(registry.points.size());++i)chartPoints[registry.points[i].chart].push_back(i);
    for(auto& entry:chartPoints)std::sort(entry.second.begin(),entry.second.end(),[&](int a,int b){auto x=registry.points[a].q,y=registry.points[b].q;return std::tie(x.x,x.y,a)<std::tie(y.x,y.y,b);});
    for(auto& piece:indexed){
        std::vector<int> boundary;const auto& candidates=chartPoints.at(v.faces[piece.face].chart);
        for(std::size_t k=0;k<piece.vertices.size();++k){
            int ia=piece.vertices[k],ib=piece.vertices[(k+1)%piece.vertices.size()];Point a=registry.points[ia].q,b=registry.points[ib].q,d=b-a;Real d2=norm2(d);
            if(d2<=weldTolerance*weldTolerance)fail(ErrorCode::NumericalFailure,"survivor edge below welding resolution");
            std::vector<std::pair<Real,int>> onEdge{{0,ia}};
            Real lo=std::min(a.x,b.x)-weldTolerance,hi=std::max(a.x,b.x)+weldTolerance;
            auto start=std::lower_bound(candidates.begin(),candidates.end(),lo,[&](int id,Real x){return registry.points[id].q.x<x;});
            for(auto it=start;it!=candidates.end()&&registry.points[*it].q.x<=hi;++it){int id=*it;if(id==ia||id==ib)continue;Point delta=registry.points[id].q-a;Real t=dot(delta,d)/d2;
                if(t>0&&t<1&&std::abs(cross(d,delta))<=weldTolerance*std::sqrt(d2))onEdge.push_back({t,id});
            }
            std::sort(onEdge.begin(),onEdge.end());for(auto pair:onEdge)if(boundary.empty()||boundary.back()!=pair.second)boundary.push_back(pair.second);
        }
        piece.vertices=std::move(boundary);
    }
    std::vector<int> outputIndex;
    auto outputVertex=[&](int registryId){
        if(outputIndex.size()<registry.points.size())outputIndex.resize(registry.points.size(),-1);
        int& id=outputIndex[registryId];if(id<0){budget(result.mesh.vertices.size()+1,limits.maxVertices,"vertex");id=int(result.mesh.vertices.size());result.mesh.vertices.push_back(registry.points[registryId].vertex);}return id;
    };
    Real emittedArea=0;
    auto triangle=[&](std::size_t fi,int a,int b,int c){
        Point pa=registry.points[a].q,pb=registry.points[b].q,pc=registry.points[c].q;Real twice=cross(pb-pa,pc-pa);
        if(twice<=2*minArea)fail(ErrorCode::NumericalFailure,"nonpositive or unresolved triangle after conforming triangulation");
        Point storedA=v.coordinates.convert(registry.points[a].vertex.uv),storedB=v.coordinates.convert(registry.points[b].vertex.uv),storedC=v.coordinates.convert(registry.points[c].vertex.uv);
        if(cross(storedB-storedA,storedC-storedA)<=2*minArea)
            fail(ErrorCode::NumericalFailure,"generated triangle collapses or reverses in double UV storage");
        if(!(crossNorm(registry.points[a].vertex.p,registry.points[b].vertex.p,registry.points[c].vertex.p)>0))fail(ErrorCode::NumericalFailure,"generated triangle has zero 3D area");
        budget(result.mesh.faces.size()+1,limits.maxTriangles,"triangle");
        if(v.faces[fi].winding<0){std::swap(b,c);std::swap(pb,pc);}
        result.mesh.faces.push_back({outputVertex(a),outputVertex(b),outputVertex(c)});
        result.sourceFaces.push_back(fi);result.sourceBarycentrics.push_back({barycentric(source,fi,pa,v.coordinates),barycentric(source,fi,pb,v.coordinates),barycentric(source,fi,pc,v.coordinates)});emittedArea+=twice*.5L;
    };
    for(const auto& piece:indexed){
        const auto& ids=piece.vertices;
        if(ids.size()==3){triangle(piece.face,ids[0],ids[1],ids[2]);continue;}
        Point center;for(int id:ids)center=center+registry.points[id].q;center=center*(1/Real(ids.size()));
        int c=registry.insert(center,interpolate(source,piece.face,center,v.coordinates),v.faces[piece.face].chart);
        for(std::size_t i=0;i<ids.size();++i)triangle(piece.face,c,ids[i],ids[(i+1)%ids.size()]);
    }
    Real expected=0;for(const auto& p:pieces)expected+=signedArea(p.polygon);
    if(std::abs(expected-emittedArea)>1e-10L*std::max(Real(1),v.totalArea))fail(ErrorCode::NumericalFailure,"area conservation check failed after welding/triangulation");
    result.retainedUVArea=areaMetric(emittedArea,v.coordinates.scale);
    return result;
}
}
