#pragma once

#include "Renderer.hpp"

#include <cmath>
#include <optional>

namespace Playground
{
struct ViewportPoint {double x{},y{},depth{};};
namespace ViewportDetail
{
inline Vec3d add(Vec3d a,Vec3d b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3d sub(Vec3d a,Vec3d b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3d mul(Vec3d a,double s) {return {a.x*s,a.y*s,a.z*s};}
inline double dot(Vec3d a,Vec3d b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
struct Basis {Vec3d right,up,forward,eye,target;};
inline Basis basis(const Camera &camera)
{
    const double cy=std::cos(camera.yaw),sy=std::sin(camera.yaw);
    const double cp=std::cos(camera.pitch),sp=std::sin(camera.pitch);
    const Vec3d right{cy,0,-sy},forward{sy*cp,sp,cy*cp};
    const Vec3d up{-sy*sp,cp,-cy*sp};
    const Vec3d target{camera.target[0],camera.target[1],camera.target[2]};
    return {right,up,forward,sub(target,mul(forward,camera.distance)),target};
}
}
inline std::optional<ViewportPoint> projectToViewport(Vec3d world,
    const Camera &camera,unsigned width,unsigned height)
{
    if (!width || !height || !(camera.distance>0)) return std::nullopt;
    const auto basis=ViewportDetail::basis(camera);
    const auto relative=ViewportDetail::sub(world,basis.target);
    const auto depth=ViewportDetail::dot(relative,basis.forward)+camera.distance;
    if (!(depth>.05)) return std::nullopt;
    const auto x=ViewportDetail::dot(relative,basis.right)*2.4/
        (double(width)/height)/depth;
    const auto y=ViewportDetail::dot(relative,basis.up)*2.4/depth;
    return ViewportPoint{(x+1)*width*.5,(1-y)*height*.5,depth};
}
inline std::optional<Vec3d> pointOnPlaneAtPixel(const Camera &camera,
    unsigned width,unsigned height,double pixelX,double pixelY,
    Vec3d planePoint,Vec3d planeNormal)
{
    if (!width || !height || !(camera.distance>0)) return std::nullopt;
    const auto basis=ViewportDetail::basis(camera);
    const auto ndcX=2*pixelX/width-1,ndcY=1-2*pixelY/height;
    const auto direction=ViewportDetail::add(basis.forward,
        ViewportDetail::add(ViewportDetail::mul(basis.right,
            ndcX*(double(width)/height)/2.4),
            ViewportDetail::mul(basis.up,ndcY/2.4)));
    const auto denominator=ViewportDetail::dot(direction,planeNormal);
    if (!std::isfinite(denominator) || std::abs(denominator)<1e-12)
        return std::nullopt;
    const auto distance=ViewportDetail::dot(
        ViewportDetail::sub(planePoint,basis.eye),planeNormal)/denominator;
    if (!std::isfinite(distance) || distance<0) return std::nullopt;
    return ViewportDetail::add(basis.eye,ViewportDetail::mul(direction,distance));
}
} // namespace Playground
