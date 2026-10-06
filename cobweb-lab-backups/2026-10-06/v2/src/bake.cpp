#include "cobweb.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_map>
namespace web  {
    static double damage(V2 uv,double amount,uint64_t seed,int patch) {
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
            double d=std::sqrt(std::pow((uv.x-c.x)/rx,2)+std::pow((uv.y-c.y)/ry,2));
            mask=std::min(mask,std::clamp((d-.82)/.18,0.,1.));
        }
        return mask;
    }
    Atlas bake(const Lab&l) {
        Atlas out;
        out.size=l.recipe.atlas;
        size_t count=size_t(out.size)*out.size;
        out.coverage.assign(count,0);
        out.momentX.assign(count,0);
        out.momentY.assign(count,0);
        std::vector<float> weights(count,0);
        int grid=tileGrid(l);
        double tile=double(out.size)/grid,usable=tile-6;
        struct Pixel { float c=0, x=0, y=0; };
        std::vector<std::vector<const Fiber*>> strands(l.fibers.size());
        for(const auto& f:l.fibers) strands[f.strand<0?f.id:f.strand].push_back(&f);
        for(const auto& strand:strands) {
            std::unordered_map<size_t,Pixel> footprint;
            for(const auto* fiber:strand) {
            const auto& f=*fiber;
            const Patch&p=l.patches[f.patch];
            for(size_t k=1;
            k<f.uv.size();
            ++k) {
                V2 ua=f.uv[k-1],ub=f.uv[k],a=atlasUV(l,f.patch,ua)*out.size,b=atlasUV(l,f.patch,ub)*out.size,d=b-a;
                double dsq=d.x*d.x+d.y*d.y;
                if(dsq<1e-12)continue;
                V2 mid=(ua+ub)*.5;
                V3 wp=p.sample(mid);
                double e=1e-4;
                V3 du=(p.sample( {
                    mid.x+e,mid.y
                }
                )-p.sample( {
                    mid.x-e,mid.y
                }
                ))*(.5/e),dv=(p.sample( {
                    mid.x,mid.y+e
                }
                )-p.sample( {
                    mid.x,mid.y-e
                }
                ))*(.5/e);
                V2 delta=ub-ua;
                V3 tangent=normalized(du*delta.x+dv*delta.y),normal=normalized(cross(du,dv)),perp=normalized(cross(normal,tangent));
                // World-to-UV inverse metric gives a physical width normal to the centerline.
                double g00=du.x*du.x+du.y*du.y+du.z*du.z,g01=du.x*dv.x+du.y*dv.y+du.z*dv.z,g11=dv.x*dv.x+dv.y*dv.y+dv.z*dv.z,det=g00*g11-g01*g01;
                double pu=perp.x*du.x+perp.y*du.y+perp.z*du.z,pv=perp.x*dv.x+perp.y*dv.y+perp.z*dv.z;
                V2 inv {
                    (g11*pu-g01*pv)/det,(g00*pv-g01*pu)/det
                }
                ;
                double radius=.5*l.recipe.width*f.width*std::abs(delta.x*inv.y-delta.y*inv.x)/length(delta)*usable;
                double support=radius+.7;
                // Clip footprints to their chart tile, with an empty one-pixel border.
                // Large artistic widths must never contaminate adjacent islands.
                const int tileX=int(std::ceil((f.patch%grid)*tile));
                const int tileY=int(std::ceil((f.patch/grid)*tile));
                const int tileEndX=int(std::ceil((f.patch%grid+1)*tile));
                const int tileEndY=int(std::ceil((f.patch/grid+1)*tile));
                int minx=std::max(tileX+1,int(std::floor(std::min(a.x,b.x)-support)));
                int maxx=std::min(tileEndX-2,int(std::ceil(std::max(a.x,b.x)+support)));
                int miny=std::max(tileY+1,int(std::floor(std::min(a.y,b.y)-support)));
                int maxy=std::min(tileEndY-2,int(std::ceil(std::max(a.y,b.y)+support)));
                double angle=std::atan2(delta.y,delta.x),cx=std::cos(2*angle),cy=std::sin(2*angle);
                (void)wp;
                for(int y=miny;
                y<=maxy;
                ++y)for(int x=minx;
                x<=maxx;
                ++x) {
                    V2 q {
                        x+.5-a.x,y+.5-a.y
                    }
                    ;
                    double t=std::clamp((q.x*d.x+q.y*d.y)/dsq,0.,1.);
                    double dist=length(q-d*t);
                    double cover=std::clamp(radius+.5-dist,0.,std::min(1.,2*radius));
                    if(cover<=0)continue;
                    size_t idx=size_t(y)*out.size+x;
                    auto& pixel=footprint[idx];
                    if(cover>pixel.c) pixel={float(cover),float(cx),float(cy)};
                }
            }
            }
            // Segments and graph splits of one strand are a geometric union.
            // Their shared endpoint caps cannot create additional opacity.
            for(const auto& entry:footprint) {
                const size_t idx=entry.first;
                const auto& pixel=entry.second;
                out.coverage[idx]=1-(1-out.coverage[idx])*(1-pixel.c);
                out.momentX[idx]+=pixel.c*pixel.x;
                out.momentY[idx]+=pixel.c*pixel.y;
                weights[idx]+=pixel.c;
            }
        }
        for(int p=0;
        p<int(l.patches.size());
        ++p) {
            int x0=int(p%grid*tile),x1=int((p%grid+1)*tile),y0=int(p/grid*tile),y1=int((p/grid+1)*tile);
            for(int y=y0;
            y<y1;
            ++y)for(int x=x0;
            x<x1;
            ++x) {
                size_t i=size_t(y)*out.size+x;
                V2 uv {
                    (x+.5-(p%grid*tile+3))/usable,(y+.5-(p/grid*tile+3))/usable
                }
                ;
                double m=damage(uv,l.recipe.broken,l.recipe.seed,p);
                out.coverage[i]*=float(m);
                if(weights[i]>0) {
                    out.momentX[i]/=weights[i];
                    out.momentY[i]/=weights[i];
                }
                out.momentX[i]*=out.coverage[i];
                out.momentY[i]*=out.coverage[i];
            }
        }
        return out;
    }
}
