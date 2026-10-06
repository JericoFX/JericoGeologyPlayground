#include "cobweb.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>
namespace web  {
    static std::ofstream file(const std::filesystem::path&p,bool binary=false) {
        std::ofstream f(p,binary?std::ios::binary:std::ios::out);
        if(!f)throw std::runtime_error("cannot write "+p.string());
        f<<std::setprecision(10);
        return f;
    }
    static void v3(std::ostream&f,V3 p) {
        f<<'['<<p.x<<','<<p.y<<','<<p.z<<']';
    }
    static void tga(const Atlas&a,const std::filesystem::path&p,bool orient) {
        auto f=file(p,true);
        unsigned char header[18]= {
        }
        ;
        header[2]=2;
        header[12]=static_cast<unsigned char>(a.size&255);
        header[13]=static_cast<unsigned char>(a.size>>8);
        header[14]=header[12];
        header[15]=header[13];
        header[16]=32;
        header[17]=8;
        f.write(reinterpret_cast<char*>(header),18);
        for(size_t i=0;
        i<a.coverage.size();
        ++i) {
            auto b=[](double x) {
                return static_cast<unsigned char>(std::round(std::clamp(x,0.,1.)*255));
            }
            ;
            unsigned char r=b(a.coverage[i]),g=r,blue=r;
            if(orient) {
                r=b(.5+.5*a.momentX[i]);
                g=b(.5+.5*a.momentY[i]);
                blue=b(a.coverage[i]);
            }
            unsigned char pixel[4]= {
                blue,g,r,255
            }
            ;
            f.write(reinterpret_cast<char*>(pixel),4);
        }
        if(!f)throw std::runtime_error("TGA write failed");
    }
    void write(const Lab&l,const Atlas&a,const std::string&out,bool overwrite) {
        std::filesystem::path dir(out);
        for(const char* name:{"mesh.obj","coverage.tga","orientation.tga","curves.json","metadata.json","recipe.json"}) {
            if(!overwrite && std::filesystem::exists(dir/name))
                throw std::runtime_error("output already exists: "+(dir/name).string()+"; choose a new directory or explicitly pass --overwrite");
        }
        std::filesystem::create_directories(dir);
        auto mesh=file(dir/"mesh.obj");
        mesh<<"# Jerico Cobweb Lab: one carrier object, no tubes per fiber\no CobwebCarrier\n";
        V3 lo {
            1e9,1e9,1e9
        }
        ,hi {
            -1e9,-1e9,-1e9
        }
        ;
        for(auto v:l.mesh.vertices) {
            mesh<<"v "<<v.p.x<<' '<<v.p.y<<' '<<v.p.z<<'\n';
            lo= {
                std::min(lo.x,v.p.x),std::min(lo.y,v.p.y),std::min(lo.z,v.p.z)
            }
            ;
            hi= {
                std::max(hi.x,v.p.x),std::max(hi.y,v.p.y),std::max(hi.z,v.p.z)
            }
            ;
        }
        for(auto v:l.mesh.vertices)mesh<<"vt "<<v.uv.x<<' '<<v.uv.y<<'\n';
        for(auto v:l.mesh.vertices)mesh<<"vn "<<v.n.x<<' '<<v.n.y<<' '<<v.n.z<<'\n';
        for(auto face:l.mesh.faces) {
            mesh<<'f';
            for(auto i:face)mesh<<' '<<i+1<<'/'<<i+1<<'/'<<i+1;
            mesh<<'\n';
        }
        tga(a,dir/"coverage.tga",false);
        tga(a,dir/"orientation.tga",true);
        auto recipe=file(dir/"recipe.json");
        auto&r=l.recipe;
        recipe<<"{\"generator_version\":3,\"units\":\"meters\",\"preset\":\""<<r.preset<<"\",\"seed\":"<<r.seed<<",\"density\":"<<r.density<<",\"sag\":"<<r.sag<<",\"tension\":"<<r.tension<<",\"broken\":"<<r.broken<<",\"width\":"<<r.width<<",\"layers\":"<<r.layers<<",\"atlas\":"<<r.atlas<<",\"quality\":"<<r.quality<<"}\n";
        auto curves=file(dir/"curves.json");
        curves<<"{\"nodes\":[";
        bool first=true;
        for(auto n:l.nodes) {
            if(!first)curves<<',';
            first=false;
            curves<<"{\"id\":"<<n.id<<",\"patch\":"<<n.patch<<",\"uv\":["<<n.uv.x<<','<<n.uv.y<<"]}";
        }
        curves<<"],\"curves\":[";
        first=true;
        for(auto f:l.fibers) {
            if(!first)curves<<',';
            first=false;
            curves<<"{\"id\":"<<f.id<<",\"strand\":"<<f.strand<<",\"patch\":"<<f.patch<<",\"node_a\":"<<f.nodeA<<",\"node_b\":"<<f.nodeB<<",\"structural\":"<<(f.structural?"true":"false")<<",\"width_m\":"<<r.width*f.width<<",\"uv\":[";
            for(size_t i=0;
            i<f.uv.size();
            ++i) {
                if(i)curves<<',';
                curves<<'['<<f.uv[i].x<<','<<f.uv[i].y<<']';
            }
            curves<<"]}";
        }
        curves<<"]}\n";
        double coverage=0;
        for(float c:a.coverage)coverage+=c;
        coverage/=double(a.coverage.size());
        auto m=file(dir/"metadata.json");
        m<<"{\"density_foci\":[";
        if(l.recipe.preset=="corridor") {
            bool firstFocus=true;
            for(auto f:corridorFoci()) {
                if(!firstFocus)m<<',';
                firstFocus=false;
                m<<"{\"patch\":"<<f.patch<<",\"uv\":["<<f.uv.x<<','<<f.uv.y<<"],\"radius\":["<<f.radius.x<<','<<f.radius.y<<"]}";
            }
        }
        m<<"],";

        m<<"\"generator_version\":3,\"preset\":\""<<r.preset<<"\",\"seed\":"<<r.seed<<",\"units\":\"meters\",\"anchors\":[";
        first=true;
        for(auto anchor:l.anchors) {
            if(!first)m<<',';
            first=false;
            m<<"{\"id\":"<<anchor.id<<",\"position\":";
            v3(m,anchor.position);
            m<<'}';
        }
        m<<"],\"patches\":[";
        first=true;
        for(auto p:l.patches) {
            if(!first)m<<',';
            first=false;
            m<<"{\"id\":"<<p.id<<",\"layer\":"<<p.layer<<",\"triangular\":"<<(p.triangular?"true":"false")<<",\"corners\":[";
            for(int i=0;
            i<(p.triangular?3:4);
            ++i) {
                if(i)m<<',';
                v3(m,p.corners[i]);
            }
            m<<"]}";
        }
        m<<"],\"aabb\":{\"min\":";
        v3(m,lo);
        m<<",\"max\":";
        v3(m,hi);
        m<<"},\"clearance\":{\"half_width\":0.72,\"height\":2.4,\"y_min\":0,\"y_max\":9},\"stats\":{\"vertices\":"<<l.mesh.vertices.size()<<",\"triangles\":"<<l.mesh.faces.size()<<",\"main_patches\":"<<l.mainPatches<<",\"patches\":"<<l.patches.size()<<",\"fiber_curves\":"<<l.fibers.size()<<",\"authoring_nodes\":"<<l.nodes.size()<<",\"runtime_web_objects\":1,\"fiber_tubes\":0,\"clearance_intersections\":0,\"mean_atlas_coverage\":"<<coverage<<",\"atlas_bytes\":"<<a.coverage.size()*8<<"},\"limitations\":[\"Artistic width and sag, no silk physics\",\"Offline renderer, no real-time performance measurement\",\"Single-sheet silhouettes disappear near tangent views\",\"Damage is baked coverage; undamaged authoring graph retained\"]}\n";
    }
}
