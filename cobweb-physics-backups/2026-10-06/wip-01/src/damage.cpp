#include "damage.hpp"
#include <algorithm>
#include <cmath>
namespace web {
    double damage(V2 uv,double amount,uint64_t seed,int patch,bool irregular,int layer) {
        if(amount<=0)return 1;
        Random r(seed^(uint64_t(patch+13)*0x9e3779b97f4a7c15ULL));
        double mask=1;
        for(int i=0;
        i<4;
        ++i) {
            V2 c {
                .18+.65*r.unit(),.15+.68*r.unit()
            }
            ;
            double rx=(.06+.14*r.unit())*std::sqrt(amount),ry=(.07+.16*r.unit())*std::sqrt(amount);
            double qx=(uv.x-c.x)/rx,qy=(uv.y-c.y)/ry;
            double d=std::hypot(qx,qy);
            if(irregular) {
                // Shape randomness is local to one hole, not a global noise mask.
                // This separate stream preserves the original hole centers.
                Random shape(seed^(uint64_t(patch+1)*0xbf58476d1ce4e5b9ULL)^(uint64_t(i+1)*0x94d049bb133111ebULL));
                const double pi=3.14159265358979323846;
                double phase=shape.unit()*2*pi+.43*layer;
                double angle=std::atan2(qy,qx);
                double profile=1+.22*std::sin(3*angle+phase)+.11*std::sin(7*angle-.7*phase)+.06*std::sin(13*angle+1.3*phase);
                d*=profile;
                double direction=(patch%3==2?.08:(patch%3==0?.65:-.65))+.32*(shape.unit()-.5)+.12*layer;
                V2 axis{std::cos(direction),std::sin(direction)};
                double baseRadius=std::sqrt(rx*ry/amount);
                // Two separated short tears follow the local patch orientation.
                // Fixed centers + expanding capsule radii keep damage monotonic.
                for(int tear=0;tear<2;++tear) {
                    double sign=tear==0?-1.:1.;
                    double offset=baseRadius*(.75+.45*shape.unit());
                    V2 origin=c+axis*(sign*offset),delta=uv-origin;
                    double along=delta.x*axis.x+delta.y*axis.y;
                    double across=-delta.x*axis.y+delta.y*axis.x;
                    double halfLength=(.025+.045*shape.unit())*std::sqrt(amount);
                    double radius=(.004+.006*shape.unit())*std::sqrt(amount);
                    double end=std::max(0.,std::abs(along)-halfLength);
                    double distance=std::hypot(end,across)/radius;
                    mask=std::min(mask,std::clamp((distance-.75)/.25,0.,1.));
                }
            }
            mask=std::min(mask,std::clamp((d-.82)/.18,0.,1.));
        }
        return mask;
    }
}
namespace web {
// Conservative outer bounds of the exact R3 damage shapes. False guarantees no
// mask intersection; true may remove a little extra silk near an irregular edge.
bool damageMayIntersect(V2 a,V2 b,double amount,uint64_t seed,int patch,bool irregular,int layer){
 if(amount<=0)return false;
 auto pointSegment=[](V2 p,V2 x,V2 y){V2 d=y-x;double q=d.x*d.x+d.y*d.y;double t=q>1e-20?std::clamp(((p.x-x.x)*d.x+(p.y-x.y)*d.y)/q,0.,1.):0.;return length(p-(x+d*t));};
 Random r(seed^(uint64_t(patch+13)*0x9e3779b97f4a7c15ULL));
 for(int i=0;i<4;++i){V2 c{.18+.65*r.unit(),.15+.68*r.unit()};double rx=(.06+.14*r.unit())*std::sqrt(amount),ry=(.07+.16*r.unit())*std::sqrt(amount);
  V2 x{(a.x-c.x)/rx,(a.y-c.y)/ry},y{(b.x-c.x)/rx,(b.y-c.y)/ry};double bound=irregular?1/.61:1.;if(pointSegment({},x,y)<=bound)return true;
  if(irregular){Random shape(seed^(uint64_t(patch+1)*0xbf58476d1ce4e5b9ULL)^(uint64_t(i+1)*0x94d049bb133111ebULL));shape.unit();double direction=(patch%3==2?.08:(patch%3==0?.65:-.65))+.32*(shape.unit()-.5)+.12*layer;V2 axis{std::cos(direction),std::sin(direction)};double baseRadius=std::sqrt(rx*ry/amount);
   for(int tear=0;tear<2;++tear){double sign=tear==0?-1.:1.;double offset=baseRadius*(.75+.45*shape.unit());V2 origin=c+axis*(sign*offset);double halfLength=(.025+.045*shape.unit())*std::sqrt(amount),radius=(.004+.006*shape.unit())*std::sqrt(amount);
    // Bounding circle fully encloses this outer capsule, including all feathering.
    if(pointSegment(origin,a,b)<=halfLength+radius)return true;
   }
  }
 }
 return false;
}
}
