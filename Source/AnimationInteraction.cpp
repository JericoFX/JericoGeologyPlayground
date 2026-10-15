#include "AnimationInteraction.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace JE::AnimationTools {
namespace {
namespace M = Math;
using Matrix = std::vector<std::vector<double>>;
M::Quat conjugate(M::Quat q) {return Examples::RiggingDetail::Conjugate(q);}
M::Vec3 rotationLog(M::Quat q) {
    q=M::Unit(q);if(q.w<0)q={-q.x,-q.y,-q.z,-q.w};
    const double n=std::hypot(q.x,q.y,q.z);
    if(n<1e-12)return {2*q.x,2*q.y,2*q.z};
    return M::Vec3{q.x,q.y,q.z}*(2*std::atan2(n,q.w)/n);
}
M::Quat rotationExp(M::Vec3 v) {const double n=M::Length(v);return n<1e-12?M::Unit(M::Quat{v.x*.5,v.y*.5,v.z*.5,1}):M::AxisAngle(v,n);}
double angularError(M::Quat a,M::Quat b) {return M::Length(rotationLog(M::Multiply(a,conjugate(b))));}
M::RigidTransform normalized(M::RigidTransform t) {M::Finite(t.position);t.rotation=M::Unit(t.rotation);return t;}
double fraction(double time,double start,double duration) {return duration<=0?1:std::clamp((time-start)/duration,0.0,1.0);}
double smooth(double u) {return u*u*(3-2*u);}
M::RigidTransform interpolate(M::RigidTransform a,M::RigidTransform b,double u) {return {a.position*(1-u)+b.position*u,M::Slerp(a.rotation,b.rotation,u)};}
void timing(double time,double duration) {M::Finite(time);M::Finite(duration);if(time<0||duration<0)throw std::invalid_argument("Invalid interaction time or duration");}
double blendAt(const LimbControl& limb,double time) {return limb.weightFrom+(limb.weight-limb.weightFrom)*smooth(fraction(time,limb.weightStart,limb.weightDuration));}
M::RigidTransform effector(const M::WorldPose& world,const LimbControl& limb) {auto t=world.at(limb.chain.bones.back());t.position=t.position+M::Rotate(t.rotation,limb.chain.tipOffset);return t;}
M::RigidTransform desiredTarget(const InteractionState& state,const LimbControl& limb,double time) {
    const auto target=limb.grabPointId.empty()?limb.target:M::Compose(grabPointWorld(state,limb.grabPointId),limb.grabOffset);
    return interpolate(limb.targetFrom,target,smooth(fraction(time,limb.targetStart,limb.targetDuration)));
}
struct Layout {
    std::map<std::string,std::size_t> index;
    std::map<std::string,std::string> parent;
    explicit Layout(const Pose& bones) {for(std::size_t i=0;i<bones.size();++i){index.emplace(bones[i].id,i);parent.emplace(bones[i].id,bones[i].parent);}}
    bool ancestor(const std::string& a,const std::string& b) const {auto id=b;while(!id.empty()){if(id==a)return true;id=parent.at(id);}return false;}
};
std::set<std::string> unavailable(const Layout& layout,const std::set<std::string>& locks) {
    std::set<std::string> result;
    for(const auto& lock:locks){if(!layout.index.contains(lock))throw std::invalid_argument("Unknown world lock ID");auto id=lock;while(!id.empty()){result.insert(id);id=layout.parent.at(id);}}
    return result;
}
std::vector<std::string> controlledIds(const LimbControl& limb) {
    std::set<std::string> ids(limb.chain.bones.begin(),limb.chain.bones.end());ids.insert(limb.sharedJoints.begin(),limb.sharedJoints.end());return {ids.begin(),ids.end()};
}
struct Angles {M::Quat swing{};double swingAngle{},twistAngle{};};
Angles limitAngles(M::Quat local,M::Quat rest,const JointLimit& limit) {
    auto delta=M::Multiply(conjugate(rest),local);if(delta.w<0)delta={-delta.x,-delta.y,-delta.z,-delta.w};
    const auto axis=M::Unit(limit.twistAxis);const double projection=M::Dot({delta.x,delta.y,delta.z},axis);
    M::Quat twist{axis.x*projection,axis.y*projection,axis.z*projection,delta.w};
    if(std::hypot(projection,delta.w)<1e-12)twist={};else twist=M::Unit(twist);
    auto swing=M::Multiply(delta,conjugate(twist));if(swing.w<0)swing={-swing.x,-swing.y,-swing.z,-swing.w};
    return {swing,M::Length(rotationLog(swing)),2*std::atan2(M::Dot({twist.x,twist.y,twist.z},axis),twist.w)};
}
double violation(M::Quat local,M::Quat rest,const JointLimit& limit) {
    const auto a=limitAngles(local,rest,limit);return std::max({0.0,a.swingAngle-limit.swingRadians,limit.twistMin-a.twistAngle,a.twistAngle-limit.twistMax});
}
bool fixed(const JointLimit& limit) {return limit.swingRadians==0&&limit.twistMin==limit.twistMax;}
void projectLimits(Pose& pose,const Document& base,const InteractionState& state,const Layout& layout,const std::set<std::string>& allowed) {
    for(const auto& [id,limit]:state.jointLimits) {
        if(!allowed.contains(id))continue;
        auto& rotation=pose[layout.index.at(id)].local.rotation;const auto rest=base.bones[layout.index.at(id)].local.rotation;
        const auto angles=limitAngles(rotation,rest,limit);auto swing=angles.swing;
        if(angles.swingAngle>limit.swingRadians&&angles.swingAngle>1e-12)swing=M::Slerp({},swing,limit.swingRadians/angles.swingAngle);
        const auto twist=M::AxisAngle(limit.twistAxis,std::clamp(angles.twistAngle,limit.twistMin,limit.twistMax));
        rotation=M::Multiply(rest,M::Multiply(swing,twist));
    }
}
Pose proceduralBase(const Document& base,const InteractionState& state,double clipTime,double time) {
    auto pose=Examples::EvaluateLocals(base,clipTime);const Layout layout(pose);
    for(const auto& [id,o]:state.overrides) {
        const double amount=o.fadeToBase?1-smooth(fraction(time,o.startTime,o.fadeDuration)):1;
        auto& local=pose[layout.index.at(id)].local;local=M::Compose(local,interpolate({},o.delta,amount));
    }return pose;
}
struct Goal {std::string id;const LimbControl* limb{};M::RigidTransform target{};double weight{};};
M::Vec3 defaultPole(const M::WorldPose& world,const LimbControl& limb) {
    if(limb.pole)return *limb.pole;
    const auto root=world.at(limb.chain.bones.front()).position,middle=world.at(limb.chain.bones[1]).position;
    const auto axis=effector(world,limb).position-root;
    if(M::Length(axis)>1e-12){const auto direction=M::Unit(axis);const auto bend=middle-root-direction*M::Dot(middle-root,direction);if(M::Length(bend)>1e-8)return root+M::Unit(bend);return root+(std::abs(direction.y)<.9?M::Vec3{0,1,0}:M::Vec3{0,0,1});}
    return root+M::Vec3{0,1,0};
}
std::optional<M::Vec3> poleGoal(const M::WorldPose& world,const Goal& goal) {
    const auto& limb=*goal.limb;if(limb.chain.bones.size()!=3||M::Length(limb.chain.tipOffset)>1e-12)return std::nullopt;
    const auto root=world.at(limb.chain.bones[0]).position,middle=world.at(limb.chain.bones[1]).position,end=world.at(limb.chain.bones[2]).position;
    const auto first=M::Length(middle-root),second=M::Length(end-middle);if(first<1e-12||second<1e-12)return std::nullopt;
    return M::SolveTwoLink(root,goal.target.position,defaultPole(world,limb),first,second).joint;
}
std::vector<double> errors(const M::WorldPose& world,const std::vector<Goal>& goals,const InteractionSettings& settings) {
    std::vector<double> result;result.reserve(goals.size()*9);
    for(const auto& goal:goals) {
        const auto actual=effector(world,*goal.limb);const auto p=goal.target.position-actual.position;
        const auto r=rotationLog(M::Multiply(goal.target.rotation,conjugate(actual.rotation)))*settings.angularLengthScale;
        result.insert(result.end(),{p.x,p.y,p.z,r.x,r.y,r.z});
        if(goal.limb->pole&&settings.poleWeight>0)if(const auto desired=poleGoal(world,goal)){const auto e=(*desired-world.at(goal.limb->chain.bones[1]).position)*settings.poleWeight;result.insert(result.end(),{e.x,e.y,e.z});}
    }return result;
}
double squared(const std::vector<double>& values) {double result{};for(const auto v:values){M::Finite(v);result+=v*v;}M::Finite(result);return result;}
bool reached(const M::WorldPose& world,const std::vector<Goal>& goals,const InteractionSettings& settings) {
    for(const auto& goal:goals){const auto actual=effector(world,*goal.limb);if(M::Length(goal.target.position-actual.position)>settings.positionTolerance||angularError(goal.target.rotation,actual.rotation)>settings.angularTolerance)return false;}return true;
}
void seedTwoLinks(Pose& pose,const Document& base,const InteractionState& state,const Layout& layout,const std::vector<Goal>& goals,const std::set<std::string>& allowed) {
    for(const auto& goal:goals) {
        const auto& limb=*goal.limb;if(limb.chain.bones.size()!=3||M::Length(limb.chain.tipOffset)>1e-12)continue;
        if(std::any_of(limb.chain.bones.begin(),limb.chain.bones.end(),[&](const auto& id){return !allowed.contains(id);}))continue;
        auto world=M::ForwardKinematics(pose);const auto actual=effector(world,limb);
        if(M::Length(actual.position-goal.target.position)<=state.settings.positionTolerance&&angularError(actual.rotation,goal.target.rotation)<=state.settings.angularTolerance)continue;
        const auto root=world.at(limb.chain.bones[0]).position,middle=world.at(limb.chain.bones[1]).position,end=world.at(limb.chain.bones[2]).position;
        const auto first=M::Length(middle-root),second=M::Length(end-middle);if(first<1e-12||second<1e-12)continue;
        const auto solution=M::SolveTwoLink(root,goal.target.position,defaultPole(world,limb),first,second);
        auto turn=[&](const std::string& id,M::Vec3 from,M::Vec3 to) {
            if(M::Length(from)<1e-12||M::Length(to)<1e-12)return;
            auto& bone=pose[layout.index.at(id)];const auto parent=bone.parent.empty()?M::Quat{}:world.at(bone.parent).rotation;
            const auto delta=Examples::RiggingDetail::RotationBetween(from,to);
            bone.local.rotation=M::Multiply(M::Multiply(M::Multiply(conjugate(parent),delta),parent),bone.local.rotation);world=M::ForwardKinematics(pose);
        };
        turn(limb.chain.bones[0],middle-root,solution.joint-root);
        turn(limb.chain.bones[1],world.at(limb.chain.bones[2]).position-world.at(limb.chain.bones[1]).position,solution.tip-world.at(limb.chain.bones[1]).position);
        auto& wrist=pose[layout.index.at(limb.chain.bones[2])];wrist.local.rotation=M::Multiply(conjugate(world.at(wrist.parent).rotation),goal.target.rotation);
        projectLimits(pose,base,state,layout,allowed);
    }
}
// Generalizes the original portable Cholesky to unique rotational DOFs.
// A is assembled symmetrically; no inverse, private layout or native ABI.
bool cholesky(const Matrix& a,const std::vector<double>& right,std::vector<double>& solution) {
    const auto n=a.size();Matrix lower(n,std::vector<double>(n));
    for(std::size_t i=0;i<n;++i)for(std::size_t j=0;j<=i;++j){double value=a[i][j];for(std::size_t k=0;k<j;++k)value-=lower[i][k]*lower[j][k];if(i==j){if(!(value>0)||!std::isfinite(value))return false;lower[i][j]=std::sqrt(value);}else{lower[i][j]=value/lower[j][j];if(!std::isfinite(lower[i][j]))return false;}}
    std::vector<double> intermediate(n);solution.assign(n,0);
    for(std::size_t i=0;i<n;++i){double value=right[i];for(std::size_t j=0;j<i;++j)value-=lower[i][j]*intermediate[j];intermediate[i]=value/lower[i][i];}
    for(std::size_t remaining=n;remaining>0;--remaining){const auto i=remaining-1;double value=intermediate[i];for(std::size_t j=i+1;j<n;++j)value-=lower[j][i]*solution[j];solution[i]=value/lower[i][i];if(!std::isfinite(solution[i]))return false;}return true;
}
M::Vec3 angularColumn(M::Vec3 error,M::Vec3 axis) {
    const double theta=M::Length(error);const double factor=theta<1e-5?1.0/12:1/(theta*theta)-1/(2*theta*std::tan(theta*.5));
    return axis+M::Cross(error,axis)*.5+M::Cross(error,M::Cross(error,axis))*factor;
}
Matrix jacobian(const M::WorldPose& world,const std::vector<Goal>& goals,const std::vector<std::string>& joints,const Layout& layout,const InteractionSettings& settings) {
    const auto residual=errors(world,goals,settings);Matrix j(residual.size(),std::vector<double>(joints.size()*3));std::size_t row{};
    constexpr std::array<M::Vec3,3> axes{{{1,0,0},{0,1,0},{0,0,1}}};
    for(const auto& goal:goals) {
        const auto actual=effector(world,*goal.limb);const auto angle=rotationLog(M::Multiply(goal.target.rotation,conjugate(actual.rotation)));
        const bool poleRows=goal.limb->pole&&settings.poleWeight>0&&poleGoal(world,goal).has_value();
        for(std::size_t joint=0;joint<joints.size();++joint) {
            const auto& id=joints[joint];if(!layout.ancestor(id,goal.limb->chain.bones.back()))continue;const auto& t=world.at(id);
            for(std::size_t axisIndex=0;axisIndex<3;++axisIndex) {
                const auto axis=M::Rotate(t.rotation,axes[axisIndex]);const auto p=M::Cross(axis,actual.position-t.position),r=angularColumn(angle,axis)*settings.angularLengthScale;const auto column=joint*3+axisIndex;
                j[row][column]=p.x;j[row+1][column]=p.y;j[row+2][column]=p.z;j[row+3][column]=r.x;j[row+4][column]=r.y;j[row+5][column]=r.z;
                if(poleRows&&layout.ancestor(id,goal.limb->chain.bones[1])){const auto v=M::Cross(axis,world.at(goal.limb->chain.bones[1]).position-t.position)*settings.poleWeight;j[row+6][column]=v.x;j[row+7][column]=v.y;j[row+8][column]=v.z;}
            }
        }row+=poleRows?9:6;
    }return j;
}
void applyStep(Pose& pose,const std::vector<std::string>& joints,const Layout& layout,const std::vector<double>& delta,double amount,double maxStep) {
    for(std::size_t joint=0;joint<joints.size();++joint){M::Vec3 step{delta[joint*3],delta[joint*3+1],delta[joint*3+2]};const double n=M::Length(step);if(n>maxStep)step=step*(maxStep/n);step=step*amount;auto& local=pose[layout.index.at(joints[joint])].local;local.rotation=M::Multiply(local.rotation,rotationExp(step));}
}
bool collinear(const M::WorldPose& world,const LimbControl& limb) {
    if(limb.chain.bones.size()<3)return true;const auto a=world.at(limb.chain.bones[0]).position,b=world.at(limb.chain.bones[1]).position,c=world.at(limb.chain.bones[2]).position;
    return M::Length(M::Cross(b-a,c-b))<1e-8;
}
void captureOverrides(const Document& base,InteractionState& state,const RuntimePose& current,const LimbControl& limb,double clipTime,double time,bool fade,double duration) {
    const auto original=Examples::EvaluateLocals(base,clipTime);const Layout layout(original);
    for(const auto& id:controlledIds(limb)){const auto i=layout.index.at(id);state.overrides[id]={M::Compose(M::Inverse(original[i].local),current.locals[i].local),time,duration,fade};}
}
M::RigidTransform limitedOffset(M::RigidTransform offset,const GrabMotionLimits& limits) {
    offset.position={std::clamp(offset.position.x,limits.translationMin.x,limits.translationMax.x),std::clamp(offset.position.y,limits.translationMin.y,limits.translationMax.y),std::clamp(offset.position.z,limits.translationMin.z,limits.translationMax.z)};
    auto r=rotationLog(offset.rotation);r={std::clamp(r.x,limits.rotationMin.x,limits.rotationMax.x),std::clamp(r.y,limits.rotationMin.y,limits.rotationMax.y),std::clamp(r.z,limits.rotationMin.z,limits.rotationMax.z)};offset.rotation=rotationExp(r);return offset;
}
void box(SkinMesh& mesh,M::Vec3 a,M::Vec3 b,double radius,std::size_t joint) {
    const auto axis=M::Unit(b-a);const auto side=M::Unit(M::Cross(axis,std::abs(axis.y)<.9?M::Vec3{0,1,0}:M::Vec3{0,0,1}))*radius,up=M::Cross(axis,side);const auto start=static_cast<std::uint32_t>(mesh.positions.size());
    for(const auto p:{a,b})for(const auto signs:std::array<std::array<int,2>,4>{{{{-1,-1}},{{1,-1}},{{1,1}},{{-1,1}}}}){mesh.positions.push_back(p+side*signs[0]+up*signs[1]);mesh.influences.push_back({{joint,1}});}
    constexpr std::array<std::array<std::uint32_t,3>,12> faces{{{{0,2,1}},{{0,3,2}},{{4,5,6}},{{4,6,7}},{{0,1,5}},{{0,5,4}},{{1,2,6}},{{1,6,5}},{{2,3,7}},{{2,7,6}},{{3,0,4}},{{3,4,7}}}};
    for(auto f:faces){for(auto& v:f)v+=start;mesh.triangles.push_back(f);}
}
}

void validateInteraction(const Document& base,const InteractionState& state) {
    Examples::Validate(base);if(base.bones.empty())throw std::invalid_argument("Interaction needs a skeleton");const Layout layout(base.bones);const auto& s=state.settings;
    for(const auto& track:base.tracks)for(const auto& key:track.keys)if(key.time<0)throw std::invalid_argument("Negative interaction key time");
    for(const auto value:{s.positionTolerance,s.angularTolerance,s.angularLengthScale,s.damping,s.maxJointStep,s.poleWeight})M::Finite(value);
    if(s.maxIterations==0||s.maxIterations>10000||s.positionTolerance<=0||s.angularTolerance<=0||s.angularLengthScale<=0||s.damping<=0||s.maxJointStep<=0||s.poleWeight<0)throw std::invalid_argument("Invalid interaction solver settings");
    for(const auto& [id,o]:state.objects){if(id.empty()||o.id!=id)throw std::invalid_argument("Object ID mismatch");(void)normalized(o.world);if(o.authority!=TransformAuthority::KinematicScene&&o.authority!=TransformAuthority::PhysicsBody)throw std::invalid_argument("Invalid object authority");}
    for(const auto& [id,p]:state.grabPoints) {
        if(id.empty()||p.id!=id||!state.objects.contains(p.objectId))throw std::invalid_argument("GrabPoint object/ID mismatch");(void)normalized(p.local);
        if(p.permittedHands!=Hand::Left&&p.permittedHands!=Hand::Right&&p.permittedHands!=Hand::Either)throw std::invalid_argument("Invalid permitted hands");
        if(p.type!=GrabType::Fixed&&p.type!=GrabType::Handle&&p.type!=GrabType::Wheel)throw std::invalid_argument("Invalid grab type");
        for(const auto v:{p.limits.translationMin,p.limits.translationMax,p.limits.rotationMin,p.limits.rotationMax})M::Finite(v);
        if(p.limits.translationMin.x>p.limits.translationMax.x||p.limits.translationMin.y>p.limits.translationMax.y||p.limits.translationMin.z>p.limits.translationMax.z||p.limits.rotationMin.x>p.limits.rotationMax.x||p.limits.rotationMin.y>p.limits.rotationMax.y||p.limits.rotationMin.z>p.limits.rotationMax.z)throw std::invalid_argument("Inverted grab limits");
        if(p.status==GrabStatus::Occupied){if(!state.limbs.contains(p.occupant)||state.limbs.at(p.occupant).grabPointId!=id)throw std::invalid_argument("Grab occupancy mismatch");}
        else if((p.status!=GrabStatus::Free&&p.status!=GrabStatus::Released)||!p.occupant.empty())throw std::invalid_argument("Invalid free/released grab state");
    }
    for(const auto& [id,l]:state.limbs) {
        if(id.empty()||l.chain.id!=id||l.chain.bones.empty())throw std::invalid_argument("Limb ID/chain mismatch");std::set<std::string> unique;std::string previous;
        for(const auto& bone:l.chain.bones){if(!layout.index.contains(bone)||!unique.insert(bone).second||(!previous.empty()&&layout.parent.at(bone)!=previous))throw std::invalid_argument("Invalid limb chain");previous=bone;}
        for(const auto& joint:l.sharedJoints)if(!layout.index.contains(joint)||!layout.ancestor(joint,l.chain.bones.front()))throw std::invalid_argument("Shared joint must be a chain ancestor");
        M::Finite(l.chain.tipOffset);(void)normalized(l.target);(void)normalized(l.targetFrom);(void)normalized(l.grabOffset);if(l.pole)M::Finite(*l.pole);timing(l.targetStart,l.targetDuration);timing(l.weightStart,l.weightDuration);M::Finite(l.weight);M::Finite(l.weightFrom);
        if(l.weight<0||l.weight>1||l.weightFrom<0||l.weightFrom>1||(l.mode!=LimbMode::Fk&&l.mode!=LimbMode::Ik))throw std::invalid_argument("Invalid IK/FK blend or mode");
        if(l.hand!=Hand::None&&l.hand!=Hand::Left&&l.hand!=Hand::Right)throw std::invalid_argument("Invalid effector hand identity");
        if(!l.grabPointId.empty()){const auto& p=state.grabPoints.at(l.grabPointId);if(p.status!=GrabStatus::Occupied||p.occupant!=id)throw std::invalid_argument("Limb grab relationship mismatch");}
    }
    for(const auto& [id,l]:state.jointLimits){if(id!=l.boneId||!layout.index.contains(id))throw std::invalid_argument("Unknown joint limit ID");(void)M::Unit(l.twistAxis);M::Finite(l.swingRadians);M::Finite(l.twistMin);M::Finite(l.twistMax);if(l.swingRadians<0||l.swingRadians>std::numbers::pi||l.twistMin>l.twistMax||l.twistMin<-std::numbers::pi||l.twistMax>std::numbers::pi)throw std::invalid_argument("Invalid joint limit angles");}
    for(const auto& [id,o]:state.overrides){if(!layout.index.contains(id))throw std::invalid_argument("Unknown override ID");(void)normalized(o.delta);timing(o.startTime,o.fadeDuration);}
    for(const auto& [id,take]:state.takes){if(id.empty()||id!=take.id)throw std::invalid_argument("Invalid take ID");Examples::Validate(take.document);if(take.document.bones.size()!=base.bones.size())throw std::invalid_argument("Take skeleton mismatch");for(const auto& b:take.document.bones)if(!layout.index.contains(b.id)||layout.parent.at(b.id)!=b.parent)throw std::invalid_argument("Take topology mismatch");timing(take.start,0);timing(take.end,0);if(take.end<take.start)throw std::invalid_argument("Invalid take range");}
}
M::RigidTransform grabPointWorld(const InteractionState& state,const std::string& id){const auto& p=state.grabPoints.at(id);return M::Compose(state.objects.at(p.objectId).world,p.local);}

RuntimePose sampleInteraction(const Document& base,const InteractionState& state,double clipTime,double time,const std::set<std::string>& locks) {
    timing(clipTime,0);timing(time,0);validateInteraction(base,state);RuntimePose result;result.locals=proceduralBase(base,state,clipTime,time);const auto fk=result.locals;const Layout layout(fk);const auto forbidden=unavailable(layout,locks);
    std::vector<Goal> goals;std::map<std::string,double> jointWeight;
    for(const auto& [id,l]:state.limbs){const auto weight=l.enabled&&l.mode==LimbMode::Ik?blendAt(l,time):0;if(weight<=0)continue;goals.push_back({id,&l,desiredTarget(state,l,time),weight});for(const auto& joint:controlledIds(l))if(!forbidden.contains(joint))jointWeight[joint]=std::max(jointWeight[joint],weight);}
    std::set<std::string> allowed;std::vector<std::string> joints;
    for(const auto& [id,_]:jointWeight){allowed.insert(id);if(state.jointLimits.contains(id)&&fixed(state.jointLimits.at(id)))continue;joints.push_back(id);}
    result.uniqueSolverJoints=joints.size();auto world=M::ForwardKinematics(result.locals);result.objectiveBefore=squared(errors(world,goals,state.settings));bool solverFailure{};
    projectLimits(result.locals,base,state,layout,allowed);world=M::ForwardKinematics(result.locals);
    if(!joints.empty()&&!reached(world,goals,state.settings)) {
        seedTwoLinks(result.locals,base,state,layout,goals,allowed);world=M::ForwardKinematics(result.locals);double objective=squared(errors(world,goals,state.settings));
        for(std::size_t iteration=0;iteration<state.settings.maxIterations&&!reached(world,goals,state.settings);++iteration) {
            const auto error=errors(world,goals,state.settings);const auto j=jacobian(world,goals,joints,layout,state.settings);const auto columns=joints.size()*3;
            Matrix normal(columns,std::vector<double>(columns));std::vector<double> right(columns);
            for(std::size_t a=0;a<columns;++a){for(std::size_t row=0;row<j.size();++row)right[a]+=j[row][a]*error[row];for(std::size_t b=0;b<=a;++b){double value{};for(std::size_t row=0;row<j.size();++row)value+=j[row][a]*j[row][b];normal[a][b]=normal[b][a]=value;}normal[a][a]+=state.settings.damping*state.settings.damping;}
            std::vector<double> delta;result.iterations=iteration+1;if(!cholesky(normal,right,delta)){solverFailure=true;break;}bool accepted{};
            for(double step=1;step>=1.0/128;step*=.5){auto candidate=result.locals;applyStep(candidate,joints,layout,delta,step,state.settings.maxJointStep);projectLimits(candidate,base,state,layout,allowed);auto candidateWorld=M::ForwardKinematics(candidate);const double next=squared(errors(candidateWorld,goals,state.settings));if(next<objective){result.locals.swap(candidate);world.swap(candidateWorld);objective=next;accepted=true;break;}}
            if(!accepted)break;
        }
    }
    projectLimits(result.locals,base,state,layout,allowed);const auto fullySolvedWorld=M::ForwardKinematics(result.locals);
    const auto fullySolvedLocals=result.locals;
    for(const auto& [id,weight]:jointWeight){const auto i=layout.index.at(id);result.locals[i].local=interpolate(fk[i].local,result.locals[i].local,weight);}
    projectLimits(result.locals,base,state,layout,allowed);result.world=M::ForwardKinematics(result.locals);result.objectiveAfter=squared(errors(result.world,goals,state.settings));
    for(const auto& [id,l]:state.limbs) {
        LimbDiagnostics d;d.actual=effector(result.world,l);d.weight=l.enabled&&l.mode==LimbMode::Ik?blendAt(l,time):0;d.target=d.weight>0?desiredTarget(state,l,time):d.actual;d.positionError=M::Length(d.target.position-d.actual.position);d.angularError=angularError(d.target.rotation,d.actual.rotation);
        if(!l.grabPointId.empty())d.contactError=M::Length(M::Compose(grabPointWorld(state,l.grabPointId),l.grabOffset).position-d.actual.position);
        for(const auto& joint:controlledIds(l))if(state.jointLimits.contains(joint)){const auto i=layout.index.at(joint);d.limitViolation=std::max(d.limitViolation,violation(result.locals[i].local.rotation,base.bones[i].local.rotation,state.jointLimits.at(joint)));}
        const auto solved=effector(fullySolvedWorld,l);d.solverPositionError=M::Length(d.target.position-solved.position);d.solverAngularError=angularError(d.target.rotation,solved.rotation);
        double solvedLimitViolation{};
        for(const auto& joint:controlledIds(l))if(state.jointLimits.contains(joint)){const auto i=layout.index.at(joint);solvedLimitViolation=std::max(solvedLimitViolation,violation(fullySolvedLocals[i].local.rotation,base.bones[i].local.rotation,state.jointLimits.at(joint)));}
        d.partialBlend=d.weight>0&&d.weight<1;d.converged=d.weight<=0||(d.solverPositionError<=state.settings.positionTolerance&&d.solverAngularError<=state.settings.angularTolerance&&solvedLimitViolation<=1e-8);
        d.unreachable=d.weight>0&&!d.converged;d.singular=d.unreachable&&(solverFailure||collinear(fullySolvedWorld,l));if(d.unreachable)for(const auto& joint:controlledIds(l))d.hardLockConflict=d.hardLockConflict||forbidden.contains(joint);result.limbs.emplace(id,d);
    }return result;
}
InteractionState withObjectTransform(const InteractionState& state,const std::string& id,M::RigidTransform world) {
    world=normalized(world);if(state.objects.at(id).authority!=TransformAuthority::KinematicScene)throw std::invalid_argument("Object transform is owned by physics");auto next=state;next.objects.at(id).world=world;return next;
}
InteractionState withPhysicsSnapshot(const InteractionState& state,const std::string& id,M::RigidTransform world) {
    world=normalized(world);if(state.objects.at(id).authority!=TransformAuthority::PhysicsBody)throw std::invalid_argument("Physics snapshot requires physics-owned object");auto next=state;next.objects.at(id).world=world;return next;
}
InteractionState withLimbTarget(const InteractionState& state,const std::string& id,M::RigidTransform target,double time,double duration) {
    timing(time,duration);target=normalized(target);const auto& limb=state.limbs.at(id);const auto previous=desiredTarget(state,limb,time);auto next=state;auto& c=next.limbs.at(id);c.targetFrom=previous;c.targetStart=time;c.targetDuration=duration;
    if(c.grabPointId.empty())c.target=target;
    else if(state.objects.at(state.grabPoints.at(c.grabPointId).objectId).authority==TransformAuthority::PhysicsBody){c.target=target;c.motorTargetEnabled=true;}
    else c.grabOffset=limitedOffset(M::Compose(M::Inverse(grabPointWorld(state,c.grabPointId)),target),state.grabPoints.at(c.grabPointId).limits);return next;
}
InteractionState withLimbBlend(const InteractionState& state,const std::string& id,double weight,double time,double duration) {
    timing(time,duration);M::Finite(weight);if(weight<0||weight>1)throw std::invalid_argument("Blend outside [0,1]");auto next=state;auto& c=next.limbs.at(id);c.weightFrom=blendAt(state.limbs.at(id),time);c.weight=weight;c.weightStart=time;c.weightDuration=duration;return next;
}
InteractionState withLimbMode(const Document& base,const InteractionState& state,const std::string& id,LimbMode mode,double clipTime,double time,const std::set<std::string>& locks) {
    timing(time,0);if(mode!=LimbMode::Fk&&mode!=LimbMode::Ik)throw std::invalid_argument("Invalid limb mode");const auto& limb=state.limbs.at(id);if(limb.mode==mode)return state;const auto current=sampleInteraction(base,state,clipTime,time,locks);auto next=state;auto& c=next.limbs.at(id);
    if(mode==LimbMode::Fk){captureOverrides(base,next,current,limb,clipTime,time,false,0);c.motorTargetEnabled=false;}
    else{c.target=current.limbs.at(id).actual;c.targetFrom=c.target;c.targetStart=time;c.targetDuration=c.grabPointId.empty()?0:.15;}c.mode=mode;c.enabled=true;validateInteraction(base,next);return next;
}
InteractionState acquireGrab(const Document& base,const InteractionState& state,const std::string& limbId,const std::string& grabId,double clipTime,double time,double duration,const std::set<std::string>& locks) {
    timing(time,duration);if(duration<=0)throw std::invalid_argument("Acquire requires a positive approach duration");const auto& limb=state.limbs.at(limbId);const auto& p=state.grabPoints.at(grabId);
    if(p.status==GrabStatus::Occupied||!limb.grabPointId.empty())throw std::invalid_argument("GrabPoint or limb already occupied");if(limb.hand!=Hand::Left&&limb.hand!=Hand::Right)throw std::invalid_argument("Grab requires an explicitly identified hand");if((static_cast<int>(limb.hand)&static_cast<int>(p.permittedHands))==0)throw std::invalid_argument("Hand not permitted by GrabPoint");
    const auto current=sampleInteraction(base,state,clipTime,time,locks);auto next=state;auto& c=next.limbs.at(limbId);c.targetFrom=current.limbs.at(limbId).actual;c.target=grabPointWorld(state,grabId);c.targetStart=time;c.targetDuration=duration;c.grabPointId=grabId;c.grabOffset={};c.motorTargetEnabled=false;c.mode=LimbMode::Ik;c.enabled=true;c.weight=c.weightFrom=1;c.weightStart=time;c.weightDuration=0;auto& grab=next.grabPoints.at(grabId);grab.status=GrabStatus::Occupied;grab.occupant=limbId;validateInteraction(base,next);return next;
}
InteractionState releaseGrab(const Document& base,const InteractionState& state,const std::string& limbId,double clipTime,double time,double duration,const std::set<std::string>& locks) {
    timing(time,duration);if(duration<=0)throw std::invalid_argument("Release requires a positive recovery duration");const auto& limb=state.limbs.at(limbId);if(limb.grabPointId.empty())throw std::invalid_argument("Limb has no grip to release");const auto current=sampleInteraction(base,state,clipTime,time,locks);auto next=state;captureOverrides(base,next,current,limb,clipTime,time,true,duration);
    auto& p=next.grabPoints.at(limb.grabPointId);p.status=GrabStatus::Released;p.occupant.clear();auto& c=next.limbs.at(limbId);c.grabPointId.clear();c.grabOffset={};c.motorTargetEnabled=false;c.mode=LimbMode::Fk;c.target=current.limbs.at(limbId).actual;c.targetFrom=c.target;c.targetDuration=0;validateInteraction(base,next);return next;
}
InteractionState withPoseOverride(const Document& base,const InteractionState& state,const std::string& boneId,M::RigidTransform local,double clipTime,double time) {
    timing(time,0);local=normalized(local);validateInteraction(base,state);const auto pose=Examples::EvaluateLocals(base,clipTime);const Layout layout(pose);auto next=state;next.overrides[boneId]={M::Compose(M::Inverse(pose[layout.index.at(boneId)].local),local),time,0,false};validateInteraction(base,next);return next;
}
InteractionState withBakedTake(const InteractionState& state,const Document& base,const std::string& id,std::span<const RecordedPose> frames) {
    validateInteraction(base,state);if(id.empty()||state.takes.contains(id)||frames.empty())throw std::invalid_argument("Bake needs a new take ID and samples");BakedTake take;take.id=id;take.document.bones=base.bones;take.start=frames.front().time;take.end=frames.back().time;const Layout layout(base.bones);std::map<std::string,std::vector<Keyframe>> keys;double previous=-1;
    for(const auto& frame:frames){timing(frame.time,0);if(frame.time<=previous)throw std::invalid_argument("Baked frame times must increase");previous=frame.time;(void)M::ForwardKinematics(frame.locals);if(frame.locals.size()!=base.bones.size())throw std::invalid_argument("Baked pose cardinality mismatch");for(const auto& bone:frame.locals){if(!layout.index.contains(bone.id)||layout.parent.at(bone.id)!=bone.parent)throw std::invalid_argument("Baked pose topology mismatch");keys[bone.id].push_back({frame.time,normalized(bone.local),{}});}}
    for(auto& [bone,boneKeys]:keys)take.document.tracks.push_back({bone,std::move(boneKeys)});Examples::Validate(take.document);auto next=state;next.takes.emplace(id,std::move(take));validateInteraction(base,next);return next;
}
InteractionState interactionForAsset(const AnimationAsset& asset) {
    validateAsset(asset);InteractionState state;const auto world=Examples::EvaluateWorld(asset.document,0);for(const auto& chain:asset.chains){LimbControl limb;limb.chain=chain;limb.target=effector(world,limb);limb.targetFrom=limb.target;state.limbs.emplace(chain.id,limb);}validateInteraction(asset.document,state);return state;
}
InteractionFixture humanoidInteractionFixture() {
    InteractionFixture f;auto& a=f.actor;a.document.bones={{"pelvis","",{{0,1.08,0},{}}},{"torso","pelvis",{{0,.45,0},{}}}};
    for(const auto& side:std::array<std::string,2>{"left","right"}){const double sign=side=="left"?-1:1;a.document.bones.push_back({side+"/shoulder","torso",{{sign*.25,.3,0},{}}});a.document.bones.push_back({side+"/elbow",side+"/shoulder",{{sign*.45,0,0},{}}});a.document.bones.push_back({side+"/wrist",side+"/elbow",{{sign*.4,0,0},{}}});a.chains.push_back({side+"-arm",{side+"/shoulder",side+"/elbow",side+"/wrist"},{}});}
    for(const auto& side:std::array<std::string,2>{"left","right"}){const double sign=side=="left"?-1:1;a.document.bones.push_back({side+"/hip","pelvis",{{sign*.16,-.08,0},{}}});a.document.bones.push_back({side+"/knee",side+"/hip",{{0,-.5,0},{}}});a.document.bones.push_back({side+"/foot",side+"/knee",{{0,-.5,.08},{}}});a.chains.push_back({side+"-leg",{side+"/hip",side+"/knee",side+"/foot"},{}});}
    const Layout layout(a.document.bones);
    for(const auto& side:std::array<std::string,2>{"left","right"}) {
        const double sign=side=="left"?-1:1;
        for(const auto& suffix:{"/shoulder","/hip","/knee"}){const auto id=side+suffix;Examples::BoneTrack track{id,{}};for(int frame=0;frame<=8;++frame){const double time=frame/4.;const double phase=std::sin(time*2*std::numbers::pi);auto local=a.document.bones[layout.index.at(id)].local;if(std::string(suffix)=="/shoulder")local.rotation=M::AxisAngle({0,1,0},sign*.2*phase);else if(std::string(suffix)=="/hip")local.rotation=M::AxisAngle({1,0,0},sign*.28*phase);else local.rotation=M::AxisAngle({1,0,0},.12*(1-sign*phase));track.keys.push_back({time,local,{}});}a.document.tracks.push_back(std::move(track));}
    }
    std::vector<std::string> ids;for(const auto& b:a.document.bones)ids.push_back(b.id);a.mesh.binding=Examples::BuildSkinBinding(a.document.bones,ids);const auto world=M::ForwardKinematics(a.document.bones);box(a.mesh,{0,1.1,0},{0,1.53,0},.2,0);box(a.mesh,{0,1.53,0},{0,1.88,0},.23,1);
    for(const auto& chain:a.chains){for(std::size_t i=0;i+1<chain.bones.size();++i)box(a.mesh,world.at(chain.bones[i]).position,world.at(chain.bones[i+1]).position,chain.id.ends_with("arm")?.07:.085,layout.index.at(chain.bones[i]));const auto end=world.at(chain.bones.back()).position;box(a.mesh,end-M::Vec3{0,0,.08},end+M::Vec3{0,0,.12},.075,layout.index.at(chain.bones.back()));}
    validateAsset(a);f.interaction=interactionForAsset(a);auto& state=f.interaction;state.sceneId="cube";state.objects.emplace("cube",SceneObject{"cube",{{0,1.55,.55},{}},TransformAuthority::KinematicScene});
    for(const auto& side:std::array<std::string,2>{"left","right"}){const auto hand=side=="left"?Hand::Left:Hand::Right;const double sign=side=="left"?-1:1;auto& arm=state.limbs.at(side+"-arm");arm.hand=hand;arm.sharedJoints={"torso"};arm.pole=M::Vec3{sign*.65,1.3,.3};state.limbs.at(side+"-leg").pole=M::Vec3{sign*.16,.5,.6};state.grabPoints.emplace("cube/"+side,GrabPoint{"cube/"+side,"cube",{{sign*.25,0,0},{}},hand,GrabType::Fixed,{},GrabStatus::Free,{}});}
    validateInteraction(a.document,state);return f;
}
} // namespace JE::AnimationTools
