#include "cobweb.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace web  {
    double length(V2 v) {
        return std::hypot(v.x,v.y);
    }
    double length(V3 v) {
        return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);
    }
    V3 cross(V3 a,V3 b) {
        return {
            a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x
        }
        ;
    }
    V3 normalized(V3 v) {
        double n=length(v);
        return n>1e-15?v*(1/n):V3 {
            0,1,0
        }
        ;
    }
    uint64_t Random::next() {
        uint64_t z=(state+=0x9e3779b97f4a7c15ULL);
        z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;
        z=(z^(z>>27))*0x94d049bb133111ebULL;
        return z^(z>>31);
    }
    double Random::unit() {
        return double(next()>>11)*0x1.0p-53;
    }
    int Random::pick(int n) {
        return int(next()%uint64_t(n));
    }
    void validate(const Recipe& r) {
        if(r.preset!="radial"&&r.preset!="corner"&&r.preset!="corridor")throw std::invalid_argument("unknown preset");
        for(double v: {
            r.density,r.sag,r.tension,r.broken
        }
        )if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("density, sag, tension and broken must be finite in [0,1]");
        if(!std::isfinite(r.width)||r.width<.0001||r.width>.03)throw std::invalid_argument("width must be finite in [.0001,.03] metres");
        if(r.layers<1||r.layers>3||r.atlas<64||r.atlas>8192||r.quality<4||r.quality>48)throw std::invalid_argument("layers 1..3, atlas 64..8192, quality 4..48 required");
    }
    V3 Patch::sample(V2 t)const {
        double u=t.x,v=t.y;
        if(triangular) {
            double w=1-u-v;
            return corners[0]*w+corners[1]*u+corners[2]*v+bows[0]*(4*w*u)+bows[1]*(4*u*v)+bows[2]*(4*v*w)+bulge*(27*u*v*w);
        }
        V3 bil=corners[0]*((1-u)*(1-v))+corners[1]*(u*(1-v))+corners[2]*(u*v)+corners[3]*((1-u)*v);
        return bil+bows[0]*(4*u*(1-u)*(1-v))+bows[1]*(4*v*(1-v)*u)+bows[2]*(4*u*(1-u)*v)+bows[3]*(4*v*(1-v)*(1-u))+bulge*(16*u*(1-u)*v*(1-v));
    }
    static void insertPatch(Lab& l,Patch p) {
        p.id=int(l.patches.size());
        l.patches.push_back(p);
        int n=p.triangular?3:4;
        for(int i=0;
        i<n;
        ++i) {
            bool exists=false;
            for(const auto&a:l.anchors)if(length(a.position-p.corners[i])<1e-10)exists=true;
            if(!exists)l.anchors.push_back( {
                int(l.anchors.size()),p.corners[i]
            }
            );
        }
    }
    Lab generate(const Recipe&r) {
        validate(r);
        Lab l;
        l.recipe=r;
        double slack=.2+.8*std::pow(1-r.tension,2);
        double sag=r.sag*slack;
        if(r.preset=="radial") {
            Patch p;
            p.corners= {
                V3 {
                    -1.6,0,.25
                }
                ,V3 {
                    1.6,.04,.35
                }
                ,V3 {
                    1.45,.02,3.1
                }
                ,V3 {
                    -1.4,-.08,3.2
                }
            }
            ;
            p.bows= {
                V3 {
                    0,.03,.19
                }
                ,V3 {
                    -.18,0,0
                }
                ,V3 {
                    0,.03,-.2
                }
                ,V3 {
                    .15,0,0
                }
            }
            ;
            p.bulge= {
                0,.3*sag,-.10*sag
            }
            ;
            insertPatch(l,p);
            l.mainPatches=1;
        }
        else if(r.preset=="corner") {
            Patch p;
            p.triangular=true;
            p.corners= {
                V3 {
                    -1.6,0,3.15
                }
                ,V3 {
                    1.5,.28,2.85
                }
                ,V3 {
                    -1.55,.35,.15
                }
                ,V3 {
                }
            }
            ;
            p.bows= {
                V3 {
                    0,.12,-.20-.25*sag
                }
                ,V3 {
                    -.22,0,.20
                }
                ,V3 {
                    .22,.08,0
                }
            }
            ;
            p.bulge= {
                .10,.5*sag,-.18*sag
            }
            ;
            insertPatch(l,p);
            l.mainPatches=1;
        }
        else {
            for(int bay=0;
            bay<3;
            ++bay) {
                double a=bay*3.+.12,b=(bay+1)*3.-.12;
                for(int side: {
                    -1,1
                }
                ) {
                    double s=double(side);
                    Patch p;
                    double low=side<0?.32:.65;
                    p.corners= {
                        V3 {
                            s*2.08,a,low
                        }
                        ,V3 {
                            s*2.08,b,low+.22
                        }
                        ,V3 {
                            s*1.00,b,3.65
                        }
                        ,V3 {
                            s*1.13,a,3.72
                        }
                    }
                    ;
                    p.bows= {
                        V3 {
                            s*-.06,0,.18
                        }
                        ,V3 {
                            s*.12,0,0
                        }
                        ,V3 {
                            s*.08,0,-.1
                        }
                        ,V3 {
                            s*.12,0,0
                        }
                    }
                    ;
                    p.bulge= {
                        s*-.16*sag,.12*sag,-.22*sag
                    }
                    ;
                    insertPatch(l,p);
                }
                Patch roof;
                roof.corners= {
                    V3 {
                        -1.13,a,3.72
                    }
                    ,V3 {
                        1.13,a,3.72
                    }
                    ,V3 {
                        1,b,3.65
                    }
                    ,V3 {
                        -1,b,3.65
                    }
                }
                ;
                roof.bows= {
                    V3 {
                        0,.1,-.17
                    }
                    ,V3 {
                        -.08,0,-.1
                    }
                    ,V3 {
                        0,-.1,-.23
                    }
                    ,V3 {
                        .08,0,-.1
                    }
                }
                ;
                roof.bulge= {
                    .12,0,-.4*sag
                }
                ;
                insertPatch(l,roof);
            }
            l.mainPatches=9;
            for(int layer=1;
            layer<r.layers;
            ++layer)for(int bay=0;
            bay<3;
            ++bay) {
                int side=(bay%2==0?-1:1);
                double s=double(side),a=bay*3.+.12,b=(bay+1)*3.-.12;
                Patch p;
                p.layer=layer;
                p.triangular=true;
                p.corners= {
                    V3 {
                        s*2.08,a,1.05+.25*layer
                    }
                    ,V3 {
                        s*2.08,b,2.65+.15*layer
                    }
                    ,V3 {
                        s*1.15,b,3.6
                    }
                    ,V3 {
                    }
                }
                ;
                p.bows= {
                    V3 {
                        s*-.14,.06,-.20
                    }
                    ,V3 {
                        s*.04,0,-.08
                    }
                    ,V3 {
                        s*.05,0,-.12
                    }
                }
                ;
                p.bulge= {
                    s*-.13*sag,-.13*layer,-.13*sag
                }
                ;
                insertPatch(l,p);
            }
        }
        addFibers(l);
        triangulate(l);
        return l;
    }
    int tileGrid(const Lab&l) {
        return int(std::ceil(std::sqrt(double(l.patches.size()))));
    }
    V2 atlasUV(const Lab&l,int p,V2 uv) {
        int grid=tileGrid(l);
        double tile=double(l.recipe.atlas)/grid;
        double pad=3.;
        double usable=tile-2*pad;
        return {
            (p%grid*tile+pad+uv.x*usable)/l.recipe.atlas,(p/grid*tile+pad+uv.y*usable)/l.recipe.atlas
        }
        ;
    }
    bool triangleInvadesVoid(V3 a,V3 b,V3 c) {
        return !(std::max( {
            a.x,b.x,c.x
        }
        )<=-.72||std::min( {
            a.x,b.x,c.x
        }
        )>=.72||std::min( {
            a.z,b.z,c.z
        }
        )>=2.4);
    }
    void triangulate(Lab& l) {
        for(const auto&p:l.patches) {
            int n=l.recipe.quality;
            int start=int(l.mesh.vertices.size());
            std::vector<std::vector<int>> ids(n+1);
            for(int j=0;
            j<=n;
            ++j) {
                int count=p.triangular?n-j:n;
                for(int i=0;
                i<=count;
                ++i) {
                    V2 uv {
                        double(i)/n,double(j)/n
                    }
                    ;
                    V3 pos=p.sample(uv);
                    double e=1e-5;
                    V3 du=(p.sample( {
                        uv.x+e,uv.y
                    }
                    )-p.sample( {
                        uv.x-e,uv.y
                    }
                    ))*(.5/e),dv=(p.sample( {
                        uv.x,uv.y+e
                    }
                    )-p.sample( {
                        uv.x,uv.y-e
                    }
                    ))*(.5/e);
                    if(length(cross(du,dv))<1e-8)throw std::runtime_error("degenerate patch derivative");
                    ids[j].push_back(int(l.mesh.vertices.size()));
                    l.mesh.vertices.push_back( {
                        pos,normalized(cross(du,dv)),atlasUV(l,p.id,uv)
                    }
                    );
                }
            }
            for(int j=0;
            j<n;
            ++j) {
                int count=p.triangular?n-j:n;
                for(int i=0;
                i<count;
                ++i) {
                    l.mesh.faces.push_back( {
                        ids[j][i],ids[j][i+1],ids[j+1][i]
                    }
                    );
                    if(!p.triangular||i<count-1)l.mesh.faces.push_back( {
                        ids[j][i+1],ids[j+1][i+1],ids[j+1][i]
                    }
                    );
                }
            }
            if(int(l.mesh.vertices.size())==start)throw std::runtime_error("empty carrier patch");
        }
        for(auto f:l.mesh.faces) {
            V3 a=l.mesh.vertices[f[0]].p,b=l.mesh.vertices[f[1]].p,c=l.mesh.vertices[f[2]].p;
            if(length(cross(b-a,c-a))<1e-10)throw std::runtime_error("zero-area face");
            if(l.recipe.preset=="corridor"&&triangleInvadesVoid(a,b,c))throw std::runtime_error("carrier face intersects central clearance");
        }
    }
}
