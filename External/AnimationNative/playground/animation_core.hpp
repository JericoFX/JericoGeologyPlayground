#pragma once
// Original, standalone C++20 algorithms. No Cascadeur or engine dependency.
// Right-handed Y-up, metres/seconds/radians. Transforms are rigid, without scale.
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace JE::AnimationTools::Playground {
struct Vec3 { double x{}, y{}, z{}; };
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline Vec3 operator*(Vec3 a, double s) { return {a.x*s,a.y*s,a.z*s}; }
inline double Dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline Vec3 Cross(Vec3 a, Vec3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
inline double Length(Vec3 a) { return std::hypot(a.x,a.y,a.z); }
inline void Finite(double a) { if (!std::isfinite(a)) throw std::invalid_argument("nonfinite scalar"); }
inline void Finite(Vec3 a) { Finite(a.x); Finite(a.y); Finite(a.z); }
inline Vec3 Unit(Vec3 a) {
    Finite(a);
    const auto length=Length(a);
    if (!std::isfinite(length) || length<1e-12) throw std::invalid_argument("zero/overflow vector");
    return a*(1/length);
}
struct Quat { double x{}, y{}, z{}, w{1}; };
inline Quat Unit(Quat q) {
    Finite(q.x); Finite(q.y); Finite(q.z); Finite(q.w);
    const auto n=std::hypot(std::hypot(q.x,q.y),std::hypot(q.z,q.w));
    if (!std::isfinite(n) || n<1e-12) throw std::invalid_argument("invalid quaternion");
    return {q.x/n,q.y/n,q.z/n,q.w/n};
}
inline Quat Multiply(Quat a, Quat b) {
    a=Unit(a); b=Unit(b);
    return Unit(Quat{a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
                    a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
                    a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
                    a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z});
}
inline Vec3 Rotate(Quat q, Vec3 v) {
    q=Unit(q); Finite(v);
    const Vec3 xyz{q.x,q.y,q.z};
    const Vec3 result=v+Cross(xyz,Cross(xyz,v)+v*q.w)*2;
    Finite(result);
    return result;
}
inline Quat AxisAngle(Vec3 axis, double angle) {
    axis=Unit(axis); Finite(angle);
    const auto s=std::sin(angle*.5);
    return {axis.x*s,axis.y*s,axis.z*s,std::cos(angle*.5)};
}
inline Quat Slerp(Quat a, Quat b, double u) {
    a=Unit(a); b=Unit(b); Finite(u);
    if (u<0 || u>1) throw std::invalid_argument("interpolation fraction outside [0,1]");
    auto dot=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;
    if (dot<0) { b={-b.x,-b.y,-b.z,-b.w}; dot=-dot; }
    dot=std::clamp(dot,0.0,1.0);
    double wa=1-u, wb=u;
    if (dot<.9995) {
        const auto theta=std::acos(dot), divisor=std::sin(theta);
        wa=std::sin((1-u)*theta)/divisor; wb=std::sin(u*theta)/divisor;
    }
    return Unit(Quat{wa*a.x+wb*b.x,wa*a.y+wb*b.y,wa*a.z+wb*b.z,wa*a.w+wb*b.w});
}
struct RigidTransform { Vec3 position{}; Quat rotation{}; };
inline RigidTransform Compose(RigidTransform a, RigidTransform b) {
    Finite(a.position); Finite(b.position);
    return {a.position+Rotate(a.rotation,b.position),Multiply(a.rotation,b.rotation)};
}
inline RigidTransform Inverse(RigidTransform a) {
    auto q=Unit(a.rotation);
    const Quat inverse{-q.x,-q.y,-q.z,q.w};
    return {Rotate(inverse,a.position*-1),inverse};
}
struct Bone { std::string id, parent; RigidTransform local{}; };
using WorldPose=std::map<std::string,RigidTransform>;
inline WorldPose ForwardKinematics(const std::vector<Bone>& bones) {
    std::map<std::string,const Bone*> by_id;
    std::map<std::string,int> visit;
    for (const auto& bone:bones) {
        if (bone.id.empty() || !by_id.emplace(bone.id,&bone).second)
            throw std::invalid_argument("empty/duplicate bone ID");
    }
    WorldPose result;
    std::function<void(const std::string&)> resolve=[&](const std::string& id) {
        if (visit[id]==2) return;
        if (visit[id]==1) throw std::invalid_argument("cyclic parent hierarchy");
        const auto found=by_id.find(id);
        if (found==by_id.end()) throw std::invalid_argument("unresolved parent ID");
        visit[id]=1;
        const auto& bone=*found->second;
        Finite(bone.local.position);
        const RigidTransform local{bone.local.position,Unit(bone.local.rotation)};
        if (bone.parent.empty()) result[id]=local;
        else { resolve(bone.parent); result[id]=Compose(result.at(bone.parent),local); }
        Finite(result.at(id).position);
        visit[id]=2;
    };
    for (const auto& [id,bone]:by_id) { (void)bone; resolve(id); }
    return result;
}
struct TwoLinkResult { Vec3 joint, tip; bool reachable{}; double residual{}; };
inline TwoLinkResult SolveTwoLink(Vec3 root, Vec3 goal, Vec3 pole, double l1, double l2) {
    Finite(root); Finite(goal); Finite(pole); Finite(l1); Finite(l2);
    if (l1<=0 || l2<=0) throw std::invalid_argument("IK lengths must be positive");
    const auto offset=goal-root;
    const auto distance=Length(offset);
    if (!std::isfinite(distance) || !std::isfinite(l1+l2)) throw std::invalid_argument("IK distance overflow");
    const auto lower=std::abs(l1-l2), upper=l1+l2;
    const auto reach=std::clamp(distance,lower,upper);
    if (distance<1e-12 && lower<1e-12) {
        const auto joint=root+Unit(pole-root)*l1;
        return {joint,root,true,0};
    }
    Vec3 axis=distance>1e-12 ? offset*(1/distance) : Unit(pole-root);
    Vec3 bend=pole-root-axis*Dot(pole-root,axis);
    if (Length(bend)<1e-12) {
        const Vec3 fallback=std::abs(axis.y)<.9 ? Vec3{0,1,0}:Vec3{0,0,1};
        bend=fallback-axis*Dot(fallback,axis);
    }
    bend=Unit(bend);
    const auto along=(l1*l1-l2*l2+reach*reach)/(2*reach);
    const auto height=std::sqrt(std::max(0.0,l1*l1-along*along));
    const auto joint=root+axis*along+bend*height, tip=root+axis*reach;
    Finite(joint); Finite(tip);
    return {joint,tip,distance>=lower-1e-12 && distance<=upper+1e-12,Length(goal-tip)};
}
inline double Hermite(double a, double b, double da, double db, double duration, double u) {
    Finite(a); Finite(b); Finite(da); Finite(db); Finite(duration); Finite(u);
    if (duration<=0 || u<0 || u>1) throw std::invalid_argument("invalid curve interval");
    const auto u2=u*u, u3=u2*u;
    const auto value=(2*u3-3*u2+1)*a+(u3-2*u2+u)*duration*da+
                     (-2*u3+3*u2)*b+(u3-u2)*duration*db;
    Finite(value);
    return value;
}
struct MassPoint { double mass{}; Vec3 position{}; };
inline Vec3 CenterOfMass(const std::vector<MassPoint>& points) {
    double total=0; Vec3 sum{};
    for (const auto& point:points) {
        Finite(point.mass); Finite(point.position);
        if (point.mass<0) throw std::invalid_argument("negative mass");
        total+=point.mass; sum=sum+point.position*point.mass;
    }
    if (total<=0 || !std::isfinite(total)) throw std::invalid_argument("invalid total mass");
    const auto result=sum*(1/total); Finite(result); return result;
}
inline Vec3 Ballistic(Vec3 p0, Vec3 v0, Vec3 gravity, double time) {
    Finite(p0); Finite(v0); Finite(gravity); Finite(time);
    if (time<0) throw std::invalid_argument("negative ballistic time");
    const auto result=p0+v0*time+gravity*(.5*time*time); Finite(result); return result;
}
struct Influence { std::size_t joint{}; double weight{}; };
inline Vec3 SkinPosition(Vec3 bind_position, const std::vector<Influence>& influences,
                         const std::vector<RigidTransform>& world,
                         const std::vector<RigidTransform>& bind_world) {
    Finite(bind_position);
    if (world.size()!=bind_world.size()) throw std::invalid_argument("pose/bind cardinality mismatch");
    Vec3 result{}; double sum=0;
    for (const auto& influence:influences) {
        Finite(influence.weight);
        if (influence.weight<0 || influence.joint>=world.size()) throw std::invalid_argument("invalid skin influence");
        const auto transform=Compose(world[influence.joint],Inverse(bind_world[influence.joint]));
        result=result+(transform.position+Rotate(transform.rotation,bind_position))*influence.weight;
        sum+=influence.weight;
    }
    if (std::abs(sum-1)>1e-8) throw std::invalid_argument("skin weights must sum to one");
    Finite(result); return result;
}
inline RigidTransform RetargetLocalDelta(RigidTransform source_pose, RigidTransform source_rest,
                                        RigidTransform target_rest, double translation_scale) {
    Finite(translation_scale);
    if (translation_scale<=0) throw std::invalid_argument("retarget scale must be positive");
    auto delta=Compose(Inverse(source_rest),source_pose);
    delta.position=delta.position*translation_scale;
    return Compose(target_rest,delta);
}
}
