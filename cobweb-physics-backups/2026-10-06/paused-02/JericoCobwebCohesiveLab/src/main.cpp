#include "cobweb.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
static double number(const std::string&s) {
    size_t n=0;
    double v=std::stod(s,&n);
    if(n!=s.size())throw std::invalid_argument("invalid number: "+s);
    return v;
}
static int integer(const std::string&s) {
    size_t n=0;
    int v=std::stoi(s,&n);
    if(n!=s.size())throw std::invalid_argument("invalid integer: "+s);
    return v;
}
int main(int argc,char**argv) {
    try {
        web::Recipe r;
        bool overwrite=false;
        std::string out="outputs/radial";
        for(int i=1;
        i<argc;
        ++i) {
            std::string k=argv[i];
            if(k=="--help") {
                std::cout<<"cobweb_lab --preset radial|corner|corridor --seed N --density 0..1 --sag 0..1 --tension 0..1 --broken 0..1 --width .0001..03 --layers 1..3 --atlas 64..8192 --quality 4..48 --out DIR [--overwrite]\n";
                return 0;
            }
            if(k=="--overwrite") { overwrite=true; continue; }
            if(i+1>=argc)throw std::invalid_argument("missing value for "+k);
            std::string v=argv[++i];
            if(k=="--preset")r.preset=v;
            else if(k=="--out")out=v;
            else if(k=="--seed") {
                if(v.empty() || v.find_first_not_of("0123456789")!=std::string::npos)
                    throw std::invalid_argument("seed must contain unsigned decimal digits only");
                size_t n=0;
                r.seed=std::stoull(v,&n);
                if(n!=v.size()||v[0]=='-')throw std::invalid_argument("invalid seed");
            }
            else if(k=="--density")r.density=number(v);
            else if(k=="--sag")r.sag=number(v);
            else if(k=="--tension")r.tension=number(v);
            else if(k=="--broken")r.broken=number(v);
            else if(k=="--width")r.width=number(v);
            else if(k=="--layers")r.layers=integer(v);
            else if(k=="--atlas")r.atlas=integer(v);
            else if(k=="--quality")r.quality=integer(v);
            else throw std::invalid_argument("unknown option "+k);
        }
        auto lab=web::generate(r);
        auto atlas=web::bake(lab);
        web::write(lab,atlas,out,overwrite);
        std::cout<<r.preset<<": "<<lab.patches.size()<<" patches, "<<lab.mesh.faces.size()<<" triangles, "<<lab.fibers.size()<<" baked curves, 1 carrier object -> "<<out<<'\n';
        return 0;
    }
    catch(const std::exception&e) {
        std::cerr<<"error: "<<e.what()<<'\n';
        return 1;
    }
}
