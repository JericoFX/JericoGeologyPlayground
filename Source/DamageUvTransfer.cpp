#include "DamageUvTransfer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace Playground
{
namespace
{
Vec3d sub(Vec3d a,Vec3d b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3d add(Vec3d a,Vec3d b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3d mul(Vec3d a,double n) {return {a.x*n,a.y*n,a.z*n};}
double dot(Vec3d a,Vec3d b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
struct SurfacePoint
{
    std::array<double,3> weights{};
    double squaredDistance{std::numeric_limits<double>::infinity()};
};
SurfacePoint nearestOnTriangle(Vec3d point,Vec3d a,Vec3d b,Vec3d c)
{
    const auto ab=sub(b,a),ac=sub(c,a),ap=sub(point,a);
    const auto aa=dot(ab,ab),abac=dot(ab,ac),cc=dot(ac,ac);
    const auto denominator=aa*cc-abac*abac;
    if (!(denominator>1e-18))
        throw std::runtime_error("Damage UV source triangle collapsed");
    const auto u=(dot(ap,ab)*cc-dot(ap,ac)*abac)/denominator;
    const auto v=(dot(ap,ac)*aa-dot(ap,ab)*abac)/denominator;
    auto candidate=[&](std::array<double,3> weights)
    {
        const auto nearest=add(mul(a,weights[0]),
            add(mul(b,weights[1]),mul(c,weights[2])));
        const auto delta=sub(point,nearest);
        return SurfacePoint{weights,dot(delta,delta)};
    };
    if (u>=0 && v>=0 && u+v<=1) return candidate({1-u-v,u,v});
    SurfacePoint best;
    const Vec3d vertices[]={a,b,c};
    for (int edge=0;edge<3;++edge)
    {
        const auto next=(edge+1)%3;
        const auto direction=sub(vertices[next],vertices[edge]);
        const auto t=std::clamp(dot(sub(point,vertices[edge]),direction)/
                                dot(direction,direction),0.0,1.0);
        std::array<double,3> weights{};
        weights[edge]=1-t;weights[next]=t;
        auto found=candidate(weights);
        if (found.squaredDistance<best.squaredDistance) best=found;
    }
    return best;
}
class UvSourceIndex
{
    struct Node
    {
        Vec3d lower{},upper{};
        std::size_t begin{},end{},left{},right{};
    };
    const TriangleMesh &mesh;
    std::vector<std::size_t> faces;
    std::vector<Node> nodes;
    std::size_t evaluations{};
    static double coordinate(Vec3d point,int axis)
    {return axis==0?point.x:(axis==1?point.y:point.z);}
    static double boxDistance(Vec3d point,const Node &node)
    {
        double distance=0;
        for (int axis=0;axis<3;++axis)
        {
            const auto value=coordinate(point,axis);
            const auto delta=std::max({coordinate(node.lower,axis)-value,
                                       value-coordinate(node.upper,axis),0.0});
            distance+=delta*delta;
        }
        return distance;
    }
    std::size_t build(std::size_t begin,std::size_t end)
    {
        Node node;
        node.begin=begin;node.end=end;
        const auto infinity=std::numeric_limits<double>::infinity();
        node.lower={infinity,infinity,infinity};
        node.upper={-infinity,-infinity,-infinity};
        for (auto entry=begin;entry<end;++entry)
            for (int corner=0;corner<3;++corner)
            {
                const auto point=mesh.positions[mesh.indices[faces[entry]*3+corner]];
                node.lower={std::min(node.lower.x,point.x),std::min(node.lower.y,point.y),
                            std::min(node.lower.z,point.z)};
                node.upper={std::max(node.upper.x,point.x),std::max(node.upper.y,point.y),
                            std::max(node.upper.z,point.z)};
            }
        const auto index=nodes.size();
        nodes.push_back(node);
        if (end-begin<=8) return index;
        const auto span=sub(node.upper,node.lower);
        const int axis=span.x>=span.y && span.x>=span.z?0:(span.y>=span.z?1:2);
        const auto middle=begin+(end-begin)/2;
        std::nth_element(faces.begin()+std::ptrdiff_t(begin),
                         faces.begin()+std::ptrdiff_t(middle),
                         faces.begin()+std::ptrdiff_t(end),
            [&](std::size_t a,std::size_t b)
            {
                double centerA=0,centerB=0;
                for (int corner=0;corner<3;++corner)
                {
                    centerA+=coordinate(mesh.positions[mesh.indices[a*3+corner]],axis);
                    centerB+=coordinate(mesh.positions[mesh.indices[b*3+corner]],axis);
                }
                return centerA<centerB || (centerA==centerB && a<b);
            });
        const auto left=build(begin,middle);
        const auto right=build(middle,end);
        nodes[index].left=left;nodes[index].right=right;
        return index;
    }
  public:
    explicit UvSourceIndex(const TriangleMesh &source):mesh(source)
    {
        faces.resize(mesh.indices.size()/3);
        std::iota(faces.begin(),faces.end(),0);
        if (!faces.empty()) build(0,faces.size());
    }
    std::size_t nearestFace(Vec3d point)
    {
        SurfacePoint best;
        std::size_t closest=0;
        auto visit=[&](auto &&self,std::size_t index)->void
        {
            const auto &node=nodes[index];
            if (boxDistance(point,node)>best.squaredDistance) return;
            if (node.end-node.begin<=8)
            {
                for (auto entry=node.begin;entry<node.end;++entry)
                {
                    if (++evaluations>16'000'000)
                        throw std::runtime_error("Damage UV transfer budget exceeded");
                    const auto face=faces[entry],offset=face*3;
                    const auto found=nearestOnTriangle(point,
                        mesh.positions[mesh.indices[offset]],
                        mesh.positions[mesh.indices[offset+1]],
                        mesh.positions[mesh.indices[offset+2]]);
                    if (found.squaredDistance<best.squaredDistance ||
                        (found.squaredDistance==best.squaredDistance && face<closest))
                    {best=found;closest=face;}
                }
                return;
            }
            const auto left=node.left,right=node.right;
            if (boxDistance(point,nodes[left])<=boxDistance(point,nodes[right]))
            {self(self,left);self(self,right);}
            else
            {self(self,right);self(self,left);}
        };
        if (!nodes.empty()) visit(visit,0);
        return closest;
    }
};
}
std::vector<std::array<float,2>> transferDamageCornerUv(
    const TriangleMesh &source,
    const std::vector<std::array<float,2>> &sourceCornerUv,
    const TriangleMesh &cut,Jerico::Guid freshInterior,
    const DamageProjector &projector)
{
    if (source.indices.empty() || source.indices.size()!=sourceCornerUv.size() ||
        cut.indices.size()/3!=cut.triangleRegions.size())
        throw std::runtime_error("Damage UV source or result is incomplete");
    UvSourceIndex index(source);
    std::vector<std::array<float,2>> result;
    result.reserve(cut.indices.size());
    for (std::size_t face=0;face<cut.indices.size()/3;++face)
    {
        const auto a=cut.indices[3*face],b=cut.indices[3*face+1],
                   c=cut.indices[3*face+2];
        if (a>=cut.positions.size() || b>=cut.positions.size() ||
            c>=cut.positions.size())
            throw std::runtime_error("Damage UV result index is invalid");
        const auto p=cut.positions[a],q=cut.positions[b],r=cut.positions[c];
        if (cut.triangleRegions[face]==freshInterior)
        {
            for (const auto point:{p,q,r})
            {
                const auto local=projectorCoordinates(projector,point);
                result.push_back({float(local.x),float(local.y)});
            }
            continue;
        }
        const auto midpoint=mul(add(p,add(q,r)),1.0/3);
        const auto closest=index.nearestFace(midpoint);
        const auto offset=closest*3;
        for (const auto point:{p,q,r})
        {
            const auto weights=nearestOnTriangle(point,
                source.positions[source.indices[offset]],
                source.positions[source.indices[offset+1]],
                source.positions[source.indices[offset+2]]).weights;
            std::array<float,2> uv{};
            for (int corner=0;corner<3;++corner)
                for (int axis=0;axis<2;++axis)
                    uv[axis]+=float(weights[corner]*sourceCornerUv[offset+corner][axis]);
            result.push_back(uv);
        }
    }
    return result;
}
}
