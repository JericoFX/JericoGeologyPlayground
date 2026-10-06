#include "cobweb.hpp"
#include <algorithm>
#include <cmath>
namespace web  {
    static constexpr double pi=3.14159265358979323846;
    static int node(Lab&l,int p,V2 uv) {
        int id=int(l.nodes.size());
        l.nodes.push_back( {
            id,p,uv
        }
        );
        return id;
    }
    static int curve(Lab&l,int p,int a,int b,double width,bool structural,V2 bow= {
    }
    ) {
        Fiber f {
            int(l.fibers.size()),p,a,b,structural,width, {
            }
        }
        ;
        f.strand=f.id;
        V2 x=l.nodes[a].uv,y=l.nodes[b].uv;
        int n=std::max(3,int(length(y-x)*45));
        for(int i=0;
        i<=n;
        ++i) {
            double t=double(i)/n;
            V2 z=x*(1-t)+y*t+bow*(4*t*(1-t));
            f.uv.push_back(z);
        }
        l.fibers.push_back(f);
        return f.id;
    }
    static int attach(Lab& l,int index,double t) {
        Fiber f=l.fibers[index];
        if(t<1e-9)return f.nodeA;
        if(t>1-1e-9)return f.nodeB;
        double q=t*(f.uv.size()-1);
        int k=std::min(int(f.uv.size())-2,int(q));
        double a=q-k;
        V2 uv=f.uv[k]*(1-a)+f.uv[k+1]*a;
        int n=node(l,f.patch,uv);
        std::vector<V2> first(f.uv.begin(),f.uv.begin()+k+1);
        first.push_back(uv);
        std::vector<V2> second {
            uv
        }
        ;
        second.insert(second.end(),f.uv.begin()+k+1,f.uv.end());
        l.fibers[index].uv=first;
        l.fibers[index].nodeB=n;
        f.id=int(l.fibers.size());
        f.nodeA=n;
        f.uv=second;
        l.fibers.push_back(f);
        return n;
    }
    static V2 pointAt(const Fiber&f,double t) {
        double q=t*(f.uv.size()-1);
        int k=std::min(int(f.uv.size())-2,int(q));
        double a=q-k;
        return f.uv[k]*(1-a)+f.uv[k+1]*a;
    }
    static int boundaryNode(Lab&l,int patch,V2 uv) {
        for(const auto&n:l.nodes)if(n.patch==patch&&length(n.uv-uv)<1e-9)return n.id;
        for(int i=0;
        i<int(l.fibers.size());
        ++i) {
            const auto&f=l.fibers[i];
            if(f.patch!=patch||!f.structural)continue;
            V2 a=f.uv.front(),b=f.uv.back(),d=b-a;
            double ds=d.x*d.x+d.y*d.y;
            if(ds<1e-12)continue;
            double t=((uv.x-a.x)*d.x+(uv.y-a.y)*d.y)/ds;
            if(t>=-1e-9&&t<=1+1e-9&&length(a+d*t-uv)<1e-8)return attach(l,i,std::clamp(t,0.,1.));
        }
        return node(l,patch,uv);
    }
    static void radial(Lab&l,const Patch&p,Random&r) {
        std::vector<int> frame;
        for(V2 v: {
            V2 {
                0,0
            }
            ,V2 {
                1,0
            }
            ,V2 {
                1,1
            }
            ,V2 {
                0,1
            }
        }
        )frame.push_back(node(l,p.id,v));
        for(int i=0;
        i<4;
        ++i)curve(l,p.id,frame[i],frame[(i+1)%4],2.1,true);
        V2 hub {
            .47+(r.unit()-.5)*.065,.48+(r.unit()-.5)*.065
        }
        ;
        int hubNode=node(l,p.id,hub);
        int rays=30,turns=12+int(22*l.recipe.density);
        std::vector<std::vector<int>> nodes(rays);
        std::vector<V2> edges;
        for(int i=0;
        i<rays;
        ++i) {
            double theta=2*pi*(i+.17*(r.unit()-.5))/rays;
            V2 d {
                std::cos(theta),std::sin(theta)
            }
            ;
            double t=1e9;
            if(d.x>1e-10)t=std::min(t,(1-hub.x)/d.x);
            if(d.x< -1e-10)t=std::min(t,-hub.x/d.x);
            if(d.y>1e-10)t=std::min(t,(1-hub.y)/d.y);
            if(d.y< -1e-10)t=std::min(t,-hub.y/d.y);
            edges.push_back(hub+d*t);
            int prev=hubNode;
            for(int k=0;
            k<turns;
            ++k) {
                double q=.07+.88*(k+double(i)/rays)/turns;
                V2 uv=hub+(edges.back()-hub)*q;
                int n=node(l,p.id,uv);
                nodes[i].push_back(n);
                curve(l,p.id,prev,n,1.2,true);
                prev=n;
            }
            int n=boundaryNode(l,p.id,edges.back());
            curve(l,p.id,prev,n,1.5,true);
        }
        // A single open spiral: last ray on turn k joins ray zero on turn k+1.
        for(int k=0;
        k<turns;
        ++k)for(int i=0;
        i<rays;
        ++i) {
            int nextI=(i+1)%rays,nextK=k+(i==rays-1);
            if(nextK>=turns)continue;
            int a=nodes[i][k],b=nodes[nextI][nextK];
            V2 mid=(l.nodes[a].uv+l.nodes[b].uv)*.5;
            V2 bow=(hub-mid)*(.007+.016*r.unit());
            curve(l,p.id,a,b,.58+.28*r.unit(),false,bow);
        }
    }
    static void irregular(Lab&l,const Patch&p,Random&r) {
        int start=int(l.fibers.size());
        std::vector<int> boundary;
        for(V2 v: {
            V2 {
                0,0
            }
            ,V2 {
                1,0
            }
            ,V2 {
                p.triangular?0.:1.,1.
            }
        }
        )boundary.push_back(node(l,p.id,v));
        if(!p.triangular)boundary.push_back(node(l,p.id, {
            0,1
        }
        ));
        for(int i=0;
        i<int(boundary.size());
        ++i)curve(l,p.id,boundary[i],boundary[(i+1)%boundary.size()],2.6,true);
        // Correlated U-shaped scaffolding and ribs hold the later local branches.
        const int ribs=18;
        for(int i=1;
        i<ribs;
        ++i) {
            double t=(i+.28*(r.unit()-.5))/ribs;
            V2 a,b,bow;
            if(p.triangular) {
                a= {
                    t,0
                }
                ;
                b= {
                    0,t
                }
                ;
                bow= {
                    -.025*t,-.025*t
                }
                ;
            }
            else {
                a= {
                    0,t
                }
                ;
                b= {
                    1,std::clamp(t+.12*std::sin(t*5),.01,.99)
                }
                ;
                bow= {
                    0,-.055*std::sin(pi*t)
                }
                ;
            }
            int na=boundaryNode(l,p.id,a),nb=boundaryNode(l,p.id,b);
            curve(l,p.id,na,nb,1.05+.7*r.unit(),true,bow);
        }
        // Cross ribs reuse exact graph junctions where they meet the frame.
        for(int i=1;
        i<=7;
        ++i) {
            double t=double(i)/8;
            V2 a,b;
            if(p.triangular) {
                a= {
                    t,0
                }
                ;
                b= {
                    1-t,t
                }
                ;
            }
            else {
                a= {
                    t,0
                }
                ;
                b= {
                    std::clamp(t+.08*std::sin(t*9),.01,.99),1
                }
                ;
            }
            int na=boundaryNode(l,p.id,a),nb=boundaryNode(l,p.id,b);
            curve(l,p.id,na,nb,1.5,true);
        }
        int steps=100+int(l.recipe.density*1000);
        if(p.layer>0)steps=int(steps*.52);
        for(int step=0;
        step<steps;
        ++step) {
            int ia=start+r.pick(int(l.fibers.size())-start);
            double ta=.12+.76*r.unit();
            V2 a=pointAt(l.fibers[ia],ta);
            int ib=-1;
            double tb=0;
            V2 b;
            // Bounded candidate search. Local attachment avoids all-to-all hair spaghetti.
            for(int retry=0;
            retry<24;
            ++retry) {
                int candidate=start+r.pick(int(l.fibers.size())-start);
                if(candidate==ia)continue;
                double t=.12+.76*r.unit();
                V2 v=pointAt(l.fibers[candidate],t);
                double d=length(v-a);
                if(d>.025&&d<.28) {
                    ib=candidate;
                    tb=t;
                    b=v;
                    break;
                }
            }
            if(ib<0)continue;
            double stripe=.5+.5*std::sin(11*a.x+4*a.y);
            double clump=std::exp(-((a.x-.2)*(a.x-.2)+(a.y-.25)*(a.y-.25))/.09);
            if(r.unit()>.42+.35*stripe+.23*clump)continue;
            int na=attach(l,ia,ta),nb=attach(l,ib,tb);
            V2 d=b-a;
            double len=length(d);
            V2 bow= {
                -d.y,d.x
            }
            ;
            bow=bow*((r.unit()-.5)*.09);
            double safety=std::min( {
                a.x,a.y,b.x,b.y,1-a.x,1-a.y,1-b.x,1-b.y
            }
            );
            if(p.triangular)safety=std::min( {
                safety,1-a.x-a.y,1-b.x-b.y
            }
            );
            if(length(bow)>safety*.35&&length(bow)>0)bow=bow*(std::max(0.,safety*.35)/length(bow));
            curve(l,p.id,na,nb,.42+1.2*r.unit()+.8*clump,false,bow);
            (void)len;
        }
    }
    void addFibers(Lab&l) {
        for(const auto&p:l.patches) {
            Random r(l.recipe.seed^((uint64_t(p.id)+1)*0xd6e8feb86659fd93ULL));
            if(l.recipe.preset=="radial")radial(l,p,r);
            else irregular(l,p,r);
        }
    }
}
