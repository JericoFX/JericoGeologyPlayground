#include "../core/src/surface_families.cpp"
#include <iostream>
#include <iomanip>
using namespace rocks;
struct Point {double u,v;};
int main(){bool failed=false;std::cout<<std::setprecision(17);for(bool limestone:{false,true})for(unsigned seed:{48U,117U,305U}){
 HeightfieldRecipe r;r.profile=limestone?"limestone_dissolution":"slate_foliated";r.seed=seed;r.size=limestone?2:.64;r.relief=limestone?.55:.12;r.backDepth=limestone?.3:.075;r.cells=256;r.width=r.height=1025;r.irregularity=.65;r.fractures=.7;r.thicknessVariation=.4;r.jointDepth=.85;r.layerSpacingMeters=.05;r.plateLengthMeters=.34;r.channelSpacingMeters=.32;r.channelWidthMeters=.075;r.pocketRadiusMeters=.12;r.pocketCount=12;OwnedFaces faces(r,!limestone);SlateStudy slate(r);LimestoneStudy lime(r);
 auto scalar=[&](Point p){double u=p.u,v=p.v;double base=.73+r.irregularity*(.025*noise(u/(r.size*.7),v/(r.size*.7),r.seed+20000)+.004*noise(u/(r.size*.075),v/(r.size*.075),r.seed+20001));double cut=limestone?lime(u,v):slate(u,v);if(limestone)base=lime.host(u,v)+r.irregularity*.01*noise(u/(r.size*.075),v/(r.size*.075),r.seed+20001);cut=std::min(cut,std::max(0.,base-.10));return base-r.jointDepth*cut;};
 for(bool owner:{true,false}){double maxHeight=0,maxCut=0,maxScalar=0;Point worst{};unsigned tested=0,leftID=0,rightID=0;double worstBefore=0,worstAfter=0;
 auto id=[&](Point p){auto q=faces.query(p.u,p.v);return owner?q.first:faces.pairKey(q.first,q.second);};
 for(unsigned y=0;y<33;y++)for(unsigned x=0;x<33;x++)for(unsigned axis=0;axis<2;axis++){if((axis==0&&x==32)||(axis==1&&y==32))continue;Point a{r.size*(x/32.-.5),r.size*(y/32.-.5)},b{a.u+(axis==0?r.size/32:0),a.v+(axis==1?r.size/32:0)};auto initial=id(a);if(initial==id(b))continue;for(unsigned i=0;i<34;i++){Point mid{(a.u+b.u)/2,(a.v+b.v)/2};if(id(mid)==initial)a=mid;else b=mid;}
 auto qa=faces.query(a.u,a.v),qb=faces.query(b.u,b.v);double ha=scalar(a),hb=scalar(b),hd=std::abs(qa.height-qb.height),ca=limestone?lime(a.u,a.v):slate(a.u,a.v),cb=limestone?lime(b.u,b.v):slate(b.u,b.v);maxHeight=std::max(maxHeight,hd);maxCut=std::max(maxCut,std::abs(ca-cb));if(std::abs(ha-hb)>maxScalar){maxScalar=std::abs(ha-hb);worst=a;leftID=id(a);rightID=id(b);worstBefore=ha;worstAfter=hb;}tested++;
 }
 std::cout<<r.profile<<" seed="<<seed<<" switch="<<(owner?"owner":"nearest_pair")<<" boundaries_tested="<<tested<<" height_jump_scalar="<<maxHeight<<" incision_jump_scalar="<<maxCut<<" final_scalar_jump="<<maxScalar<<" final_jump_m="<<maxScalar*r.relief<<" u="<<worst.u<<" v="<<worst.v<<" left_id="<<leftID<<" right_id="<<rightID<<" before="<<worstBefore<<" after="<<worstAfter<<std::endl;if(maxScalar*r.relief>1e-7)failed=true;
 }
}
 return failed?1:0;
}
