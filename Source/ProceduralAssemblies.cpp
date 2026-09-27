#include "ProceduralAssemblies.hpp"
#include "SolidGeometry.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace Playground::Assembly
{
namespace
{
Vec3d add(Vec3d a,Vec3d b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3d sub(Vec3d a,Vec3d b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3d mul(Vec3d a,double value) {return {a.x*value,a.y*value,a.z*value};}
double dot(Vec3d a,Vec3d b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3d cross(Vec3d a,Vec3d b)
{return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double length(Vec3d value) {return std::sqrt(dot(value,value));}
Vec3d unit(Vec3d value)
{
    const auto magnitude=length(value);
    if (!std::isfinite(magnitude) || magnitude<=1e-12)
        throw std::runtime_error("Assembly has an undefined direction");
    return mul(value,1/magnitude);
}
std::uint64_t stableHash(std::string_view value,std::uint32_t seed=0)
{
    std::uint64_t hash=14695981039346656037ull;
    for (unsigned shift=0;shift<32;shift+=8)
    {hash^=(seed>>shift)&255u;hash*=1099511628211ull;}
    for (unsigned char byte:value)
    {hash^=byte;hash*=1099511628211ull;}
    return hash;
}
double hash01(std::string_view value,std::uint32_t seed)
{return double(stableHash(value,seed)>>11)*(1.0/9007199254740992.0);}
double segmentDistanceSquared(Vec3d p,Vec3d q,Vec3d r,Vec3d s)
{
    const auto u=sub(q,p),v=sub(s,r),w=sub(p,r);
    const double a=dot(u,u),b=dot(u,v),c=dot(v,v),d=dot(u,w),e=dot(v,w);
    const double determinant=a*c-b*b;
    double first=determinant>1e-18 ? std::clamp((b*e-c*d)/determinant,0.0,1.0) : 0;
    double second=std::clamp((b*first+e)/c,0.0,1.0);
    first=std::clamp((b*second-d)/a,0.0,1.0);
    const auto delta=sub(add(w,mul(u,first)),mul(v,second));
    return dot(delta,delta);
}
void validateProbability(double value,const char *label)
{
    if (!std::isfinite(value) || value<0 || value>1)
        throw std::runtime_error(std::string("Invalid damage probability: ")+label);
}
}

const char *name(Kind kind)
{
    constexpr const char *names[]={"PipeCouplings","Railway","BrokenRailway","Railing",
        "BrokenRailing","WoodenPier","DamagedPier","Catwalk","BeamBridge","TrussWarren",
        "TrussPratt","TrussHowe","SuspensionBridge","RopePlankBridge"};
    return names[std::size_t(kind)];
}
const char *name(Role role)
{
    constexpr const char *names[]={"PipeBody","Coupling","Clamp","Flange","Support","Rail",
        "Sleeper","RailJoint","Post","TopRail","MiddleRail","LowerRail","Baluster",
        "DeckPlank","Beam","CrossBeam","Pile","Brace","Fascia","Girder","Tower",
        "Cable","Hanger","HandRope","Anchor"};
    return names[std::size_t(role)];
}
const char *name(State state)
{
    constexpr const char *names[]={"Intact","Missing","Broken","Bent","Loose",
        "Detached","Decayed","Collapsed"};
    return names[std::size_t(state)];
}
bool stateAllowed(Role role,State state)
{
    if (state==State::Intact || state==State::Missing) return true;
    if (role==Role::PipeBody || role==Role::Cable || role==Role::HandRope)
        return state==State::Broken || state==State::Bent || state==State::Detached;
    if (role==Role::Coupling || role==Role::Clamp || role==Role::Flange ||
        role==Role::Support || role==Role::RailJoint || role==Role::CrossBeam)
        return false;
    if (role==Role::DeckPlank || role==Role::Sleeper)
        return state==State::Broken || state==State::Loose || state==State::Bent ||
               state==State::Detached || state==State::Decayed || state==State::Collapsed;
    return state!=State::Decayed;
}
std::string stablePartId(Kind kind,Role role,std::uint32_t index,int side)
{
    std::ostringstream stream;
    stream<<name(kind)<<'/'<<name(role)<<'_'<<std::setfill('0')<<std::setw(3)<<index;
    if (side<0) stream<<"_Left";
    else if (side>0) stream<<"_Right";
    return stream.str();
}
Jerico::Guid stablePartGuid(const std::string &id)
{return Solid::region(2202,stableHash(id));}
State resolveDamage(const DamageSettings &settings,const Part &part)
{
    const auto manual=settings.manual.find(part.id);
    if (manual!=settings.manual.end())
    {
        if (!stateAllowed(part.role,manual->second))
            throw std::runtime_error("Manual state is invalid for selected part role");
        return manual->second;
    }
    if (settings.protectedIds.contains(part.id)) return State::Intact;
    for (const auto &rule:settings.rules)
    {
        if (rule.role!=part.role) continue;
        const bool matches=rule.mode==SelectorMode::IndexRange ?
            part.index>=rule.firstIndex && part.index<=rule.lastIndex :
            part.stationMeters>=rule.startMeters && part.stationMeters<=rule.endMeters;
        if (matches && stateAllowed(part.role,rule.state)) return rule.state;
    }
    if (!settings.proceduralEnabled) return State::Intact;
    validateProbability(settings.missingProbability,"missing");
    validateProbability(settings.brokenProbability,"broken");
    validateProbability(settings.looseProbability,"loose");
    validateProbability(settings.bentProbability,"bent");
    validateProbability(settings.clusterStrength,"cluster");
    if (settings.missingProbability+settings.brokenProbability+
        settings.looseProbability+settings.bentProbability>1+1e-12)
        throw std::runtime_error("Damage probabilities exceed one");
    if (settings.clusterStrength>0)
    {
        if (!std::isfinite(settings.clusterCenterMeters) ||
            !std::isfinite(settings.clusterRadiusMeters) || settings.clusterRadiusMeters<=0)
            throw std::runtime_error("Invalid damage cluster center or radius");
        const auto relative=(part.stationMeters-settings.clusterCenterMeters)/settings.clusterRadiusMeters;
        const auto probability=settings.clusterStrength*std::exp(-.5*relative*relative);
        if (hash01(part.id+"/cluster",settings.seed)<probability &&
            stateAllowed(part.role,State::Broken)) return State::Broken;
    }
    const auto random=hash01(part.id,settings.seed);
    if (random<settings.missingProbability) return State::Missing;
    if (random<settings.missingProbability+settings.brokenProbability &&
        stateAllowed(part.role,State::Broken)) return State::Broken;
    if (random<settings.missingProbability+settings.brokenProbability+
        settings.looseProbability && stateAllowed(part.role,State::Loose)) return State::Loose;
    if (random<settings.missingProbability+settings.brokenProbability+
        settings.looseProbability+settings.bentProbability &&
        stateAllowed(part.role,State::Bent)) return State::Bent;
    return State::Intact;
}
FitResult fitCountSpacing(double lengthMeters,double preferredWidthMeters,double gapMeters)
{
    if (!std::isfinite(lengthMeters) || !std::isfinite(preferredWidthMeters) ||
        !std::isfinite(gapMeters) || lengthMeters<=0 || preferredWidthMeters<=0 || gapMeters<0 ||
        preferredWidthMeters>lengthMeters || lengthMeters>10000)
        throw std::runtime_error("Invalid board fit length, width or gap");
    const auto count=std::max(std::size_t(1),std::size_t(std::floor((lengthMeters+gapMeters)/
        (preferredWidthMeters+gapMeters))));
    if (count>10000) throw std::runtime_error("Board fit exceeds part budget");
    const auto actual=(lengthMeters-double(count-1)*gapMeters)/double(count);
    if (!(actual>0)) throw std::runtime_error("Board fit collapsed below zero width");
    return {count,actual,gapMeters};
}
Construction::PathResult offsetPath(const Construction::PathResult &source,
                                    double normalOffset,double binormalOffset)
{
    if (source.frames.size()<2 || !std::isfinite(normalOffset) ||
        !std::isfinite(binormalOffset) || std::abs(normalOffset)>1000 ||
        std::abs(binormalOffset)>1000)
        throw std::runtime_error("Invalid offset path or offset magnitude");
    if (source.frames.size()>4096)
        throw std::runtime_error("Offset path exceeds bounded intersection validation budget");
    Construction::PathResult result=source;
    result.lengthMeters=0;
    for (std::size_t i=0;i<result.frames.size();++i)
    {
        const auto &original=source.frames[i];
        auto &frame=result.frames[i];
        frame.position=add(original.position,
            add(mul(original.normal,normalOffset),mul(original.binormal,binormalOffset)));
        if (i)
        {
            const auto step=length(sub(frame.position,result.frames[i-1].position));
            if (!std::isfinite(step) || step<1e-7)
                throw std::runtime_error("Offset path folded or collapsed at a tight curve");
            result.lengthMeters+=step;
        }
        frame.distanceMeters=result.lengthMeters;
    }
    const auto closed=length(sub(result.frames.front().position,result.frames.back().position))<1e-7;
    for (std::size_t i=0;i<result.frames.size();++i)
    {
        const auto before=i?i-1:0;
        const auto after=i+1<result.frames.size()?i+1:i;
        auto &frame=result.frames[i];
        frame.tangent=unit(sub(result.frames[after].position,result.frames[before].position));
        if (dot(frame.tangent,source.frames[i].tangent)<0.05)
            throw std::runtime_error("Offset path reverses direction at a tight curve");
        frame.normal=unit(sub(source.frames[i].normal,
            mul(frame.tangent,dot(source.frames[i].normal,frame.tangent))));
        frame.binormal=unit(cross(frame.tangent,frame.normal));
    }
    for (std::size_t i=0;i+1<result.frames.size();++i)
        for (std::size_t j=i+2;j+1<result.frames.size();++j)
        {
            if (closed && i==0 && j+2==result.frames.size()) continue;
            if (segmentDistanceSquared(result.frames[i].position,result.frames[i+1].position,
                result.frames[j].position,result.frames[j+1].position)<1e-10)
                throw std::runtime_error("Offset path self-intersects or nearly touches");
        }
    return result;
}
double parabolicSag(double x,double span,double sag)
{
    if (!std::isfinite(x) || !std::isfinite(span) || !std::isfinite(sag) ||
        span<=0 || span>10000 || sag<=0 || sag>span || x<0 || x>span)
        throw std::runtime_error("Invalid suspension span, station or sag");
    return 4*sag*(x/span)*(1-x/span);
}
double solveCatenaryA(double span,double sag)
{
    if (!std::isfinite(span) || !std::isfinite(sag) || span<=0 ||
        span>10000 || sag<1e-6 || sag>span)
        throw std::runtime_error("Invalid catenary span or sag");
    auto height=[&](double a)
    {
        const auto half=std::sinh(span/(4*a));
        return 2*a*half*half;
    };
    double low=span/100,high=span*span/sag;
    if (!(height(low)>sag && height(high)<sag))
        throw std::runtime_error("Catenary root could not be bracketed");
    for (unsigned iteration=0;iteration<96;++iteration)
    {
        const double middle=std::midpoint(low,high);
        const double candidate=height(middle);
        if (!std::isfinite(candidate))
            throw std::runtime_error("Catenary root became non-finite");
        if (candidate>sag) low=middle;
        else high=middle;
        if (std::abs(candidate-sag)<=std::max(1e-9,sag*1e-10))
            return middle;
    }
    const auto a=std::midpoint(low,high);
    if (std::abs(height(a)-sag)>std::max(1e-7,sag*1e-8))
        throw std::runtime_error("Catenary root did not converge within 96 iterations");
    return a;
}
double catenaryHeight(double x,double span,double sag,double a)
{
    if (!std::isfinite(x) || !std::isfinite(span) || !std::isfinite(sag) ||
        !std::isfinite(a) || span<=0 || sag<=0 || a<=0 || x<0 || x>span)
        throw std::runtime_error("Invalid catenary station or parameter");
    return 2*a*std::sinh(x/(2*a))*std::sinh((span-x)/(2*a));
}
}
