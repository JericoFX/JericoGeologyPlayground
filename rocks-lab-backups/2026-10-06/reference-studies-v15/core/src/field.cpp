#include "rocks/mesh.hpp"
#include <numbers>
#include <stdexcept>
namespace rocks {
std::vector<std::string> validateRecipe(const Recipe&r,const GridSpec&g){
 std::vector<std::string> e;auto need=[&](bool p,const char*m){if(!p)e.emplace_back(m);};
 need(r.generatorVersion!="rocks-v6"||r.extractorVersion=="poly-csg-v1","v6 surface detail requires the polyhedral macro route");
 need(r.schemaVersion==2||r.detail==0,"detail is only supported by schema2");need(r.schemaVersion==1||r.schemaVersion==2,"schemaVersion must be 1 or 2");need(r.units=="m","units must be m");bool supported=(r.schemaVersion==1&&r.generatorVersion=="rocks-v2"&&r.family=="stratified")||(r.schemaVersion==2&&((r.generatorVersion=="rocks-v3"&&(r.family=="fractured_wall"||r.family=="irregular_beds"||r.family=="overhang"))||((r.generatorVersion=="rocks-v4"||r.generatorVersion=="rocks-v5"||r.generatorVersion=="rocks-v6")&&(r.family=="common_rock"||r.family=="cliff"||r.family=="strata"))||(r.generatorVersion=="rocks-v7"&&r.family=="weathered_boulder")||(r.generatorVersion=="rocks-v8"&&r.family=="cliff_plateau")||(r.generatorVersion=="rocks-v12"&&(r.family=="columnar_basalt"||r.family=="jointed_granite"))));
 need(supported&&(r.extractorVersion=="paris-mc-v1"||(r.schemaVersion==2&&r.extractorVersion=="poly-csg-v1")),"unsupported generator/extractor/family version");
 if(r.generatorVersion=="rocks-v12"){
  need(r.wear==0&&r.detail==0,"v12 studies require wear=0 and detail=0");
  need(r.extractorVersion==(r.family=="columnar_basalt"?"poly-csg-v1":"paris-mc-v1"),"v12 family extractor mismatch");
  need(r.columnCount>=3&&r.columnCount<=7,"columnCount must be in [3,7]");
  need(std::isfinite(r.studySpacing)&&r.studySpacing>=.3&&r.studySpacing<=.6,"studySpacing must be in [.3,.6]");
  need(std::isfinite(r.roundness)&&r.roundness>=.06&&r.roundness<=.2,"roundness must be in [.06,.2]");
  need(std::isfinite(r.chipping)&&r.chipping>=0&&r.chipping<=1,"chipping must be in [0,1]");
  need(std::isfinite(r.columnLean)&&r.columnLean>=-.15&&r.columnLean<=.15,"columnLean must be in [-.15,.15]");
  need(std::isfinite(r.jointDepth)&&r.jointDepth>=0&&r.jointDepth<=1,"jointDepth must be in [0,1]");
 }
 need(r.generatorVersion!="rocks-v8"||(r.extractorVersion=="poly-csg-v1"&&r.detail==0),"plateau cliff requires the polyhedral route");
 need(r.generatorVersion!="rocks-v7"||(r.extractorVersion=="paris-mc-v1"&&r.wear==0),"weathered boulder requires MC and wear=0");need(r.extractorVersion!="poly-csg-v1"||r.wear==0,"direct polyhedral route currently requires wear=0");need(std::isfinite(r.size)&&r.size>=.1&&r.size<=10,"size must be finite in [0.1,10]");need(g.cells==64||g.cells==128,"grid.cells must be 64 or 128");need(std::isfinite(r.layerThickness)&&(r.generatorVersion=="rocks-v12"?r.layerThickness>0:(r.layerThickness>=(r.extractorVersion=="poly-csg-v1"?.02*r.size:4*1.5*r.size/g.cells)&&r.layerThickness<=.7*r.size)),"layerThickness must resolve at least four cells and fit height");for(double v:{r.contrast,r.wear,r.detail})need(std::isfinite(v)&&v>=0&&v<=1,"contrast/wear/detail must be finite in [0,1]");need(finite(g.min)&&finite(g.max)&&g.min==Vec3{-.75*r.size,-.75*r.size,-.75*r.size}&&g.max==Vec3{.75*r.size,.75*r.size,.75*r.size},"grid domain must equal padded recipe domain");return e;
}
GridSpec makeGrid(const Recipe&r,std::uint32_t c){double a=.75*r.size;return {c,{-a,-a,-a},{a,a,a}};}
double planeValue(Plane p,Vec3 v){return dot(p.normal,v)-p.offset;}
double subtractFields(double h,double c){if(!std::isfinite(h)||!std::isfinite(c))throw std::invalid_argument("nonfinite field");return std::max(h,-c);}
double boundedAmplitude(double request,double feature,double thickness){if(!std::isfinite(request)||!std::isfinite(feature)||!std::isfinite(thickness)||feature<=0||thickness<=0)throw std::invalid_argument("invalid amplitude");double a=.2*std::min(feature,thickness);return std::clamp(request,-a,a);}
static std::uint32_t hash(std::uint32_t x){x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;x^=x>>16;return x;}
static double rand01(std::uint32_t seed,std::uint32_t i){return double(hash(seed+0x9e3779b9U*i))/4294967295.;}
static double valueNoise3(Vec3 p,std::uint32_t seed){
 int ix=int(std::floor(p.x)),iy=int(std::floor(p.y)),iz=int(std::floor(p.z));
 auto fade=[](double t){return t*t*t*(t*(t*6-15)+10);};
 double fx=fade(p.x-ix),fy=fade(p.y-iy),fz=fade(p.z-iz),value=0;
 for(int x=0;x<2;x++)for(int y=0;y<2;y++)for(int z=0;z<2;z++){
  std::uint32_t h=hash(seed^hash(std::uint32_t(ix+x)*0x8da6b343U)^hash(std::uint32_t(iy+y)*0xd8163841U)^hash(std::uint32_t(iz+z)*0xcb1ab31fU));
  double v=2*double(h)/4294967295.-1;value+=v*(x?fx:1-fx)*(y?fy:1-fy)*(z?fz:1-fz);
 }return value;
}
static double weatheredValue(const Recipe&r,Vec3 world,Stage stage){
 double s=r.size,yaw=2*std::numbers::pi*rand01(r.seed,1700),lean=.35*(rand01(r.seed,1701)-.5);Vec3 p=world/s-Vec3{.035*(rand01(r.seed,1702)-.5),.025*(rand01(r.seed,1703)-.5),.04*(rand01(r.seed,1704)-.5)};
 Vec3 q{p.x*std::cos(yaw)+p.z*std::sin(yaw),p.y,-p.x*std::sin(yaw)+p.z*std::cos(yaw)};
 q={q.x*std::cos(lean)-q.y*std::sin(lean),q.x*std::sin(lean)+q.y*std::cos(lean),q.z};
 const Vec3 archetypes[]={{.46,.29,.35},{.34,.44,.31},{.50,.28,.30}};Vec3 axes=archetypes[(r.seed%7)%3];
 axes.x*=.92+.14*rand01(r.seed,1710);axes.y*=.92+.14*rand01(r.seed,1711);axes.z*=.92+.14*rand01(r.seed,1712);
 Vec3 scaled{q.x/axes.x,q.y/axes.y,q.z/axes.z};double base=(length(scaled)-1)*.3*s;
 if(stage==Stage::Base)return base;
 double shape=.032*valueNoise3(p*2.7+Vec3{3.1,7.2,-1.3},r.seed+1720)+.016*valueNoise3(p*5+Vec3{-4.3,2.7,6.1},r.seed+1721);
 double relief=.008*r.detail*valueNoise3(p*9+Vec3{9.1,-6.3,2.2},r.seed+1722);
 return base+s*(shape+relief);
}
// Smooth intersection preserves broad joint faces and a rounded continuous core.
// All coordinates and feature sizes are normalized by size; no sampled assets.
static double graniteValue(const Recipe&r,Vec3 world,Stage stage){
 double yaw=2*std::numbers::pi*rand01(r.seed,3100),c=std::cos(yaw),t=std::sin(yaw);
 Vec3 p=world/r.size,q{c*p.x+t*p.z,p.y,-t*p.x+c*p.z};
 q.x+=.13*q.y;q.z-=.09*q.y;
 const Vec3 archetypes[]={{.48,.33,.36},{.36,.47,.33},{.51,.30,.35}};
 Vec3 axes=archetypes[(r.seed%7)%3];
 axes.x*=.94+.12*rand01(r.seed,3101);axes.y*=.94+.12*rand01(r.seed,3102);
 // Analytic rounded box: broad inherited faces survive independently of
 // the corner radius. Unlike ellipsoid smoothing this cannot wash all faces out.
 Vec3 d{std::abs(q.x)-(axes.x-r.roundness),std::abs(q.y)-(axes.y-r.roundness),std::abs(q.z)-(axes.z-r.roundness)};
 double value=length(Vec3{std::max(d.x,0.),std::max(d.y,0.),std::max(d.z,0.)})+std::min(std::max({d.x,d.y,d.z}),0.)-r.roundness;
 if(stage==Stage::Base)return value*r.size;
 const double blend=.02+.06*r.roundness;
 auto smoothMax=[](double a,double b,double k){double h=std::max(k-std::abs(a-b),0.)/k;return std::max(a,b)+h*h*k*.25;};
 const Vec3 normals[]={{1,.36,.18},{-.87,-.18,-.35},{.30,1,-.26},{-.17,-1,.04},{.25,.09,1},{-.42,.30,-1}};
 for(unsigned i=0;i<6;i++){
  Vec3 n=normalized(normals[i]);double reach=std::abs(n.x)*(axes.x-r.roundness)+std::abs(n.y)*(axes.y-r.roundness)+std::abs(n.z)*(axes.z-r.roundness)+r.roundness;
  double offset=reach*(.62+.32*r.studySpacing/.6+.10*(rand01(r.seed,3120+i)-.5));
  value=smoothMax(value,dot(n,q)-offset,blend);
 }
 // Two shallow corner removals keep the broad faces quiet.
 for(unsigned i=0;i<2&&r.chipping>0;i++){
  Vec3 n=normalized(Vec3{i?-1.:.85,.65+.25*rand01(r.seed,3140+i),.75});
  double reach=std::abs(n.x)*(axes.x-r.roundness)+std::abs(n.y)*(axes.y-r.roundness)+std::abs(n.z)*(axes.z-r.roundness)+r.roundness;
  value=smoothMax(value,dot(n,q)-reach*(1.02-.24*r.chipping),blend);
 }
 return value*r.size;
}
static RockField basaltField(const Recipe&r){
 RockField f{r,{}, {},0,{}};const double s=r.size;
 auto add=[&](std::vector<Plane>&out,Vec3 n,double d){double l=length(n);out.push_back({n/l,d*s/l});};
 // A clipped basal mass physically connects the unequal columns.
 add(f.planes,{1,0,0},.55);add(f.planes,{-1,0,0},.55);
 add(f.planes,{0,1,0},.56);add(f.planes,{0,-1,0},.40);
 add(f.planes,{0,0,1},.40);add(f.planes,{0,0,-1},.36);
 std::vector<Plane> base;add(base,{.10,1,-.12},-.305);
 add(base,{1,0,0},.34);add(base,{-1,0,0},.34);
 add(base,{0,0,1},.19);add(base,{0,0,-1},r.columnCount==3?-.01:.16);f.masses.push_back(base);
 const Vec3 sites[]={{-.30,0,.13},{.015,0,.15},{.31,0,.10},{-.18,0,-.14},{.20,0,-.14},{-.40,0,-.08},{.40,0,-.09}};
 for(unsigned i=0;i<r.columnCount;i++){
  Vec3 center=sites[i];center.x+=.045*(rand01(r.seed,3200+i)-.5);center.z+=.06*(rand01(r.seed,3220+i)-.5);
  double radius=(.15+.035*rand01(r.seed,3240+i))*(.86+.30*r.studySpacing/.6),phase=.45+rand01(r.seed,3260+i)*.5;
  std::vector<Plane> column;
  for(unsigned j=0;j<5;j++){
   double angle=phase+j*2*std::numbers::pi/5+.15*(rand01(r.seed,3280+i*5+j)-.5);
   Vec3 n{std::cos(angle),-r.columnLean*std::cos(angle),std::sin(angle)};
   add(column,n,dot(n,center)+radius*(.92+.12*rand01(r.seed,3330+i*5+j)));
  }
  add(column,{.34*(rand01(r.seed,3400+i)-.5),1,.42*(rand01(r.seed,3420+i)-.5)},.10+.40*rand01(r.seed,3440+i));
  f.masses.push_back(std::move(column));
 }
 // Partial transverse joints, individually restricted in X and depth.
 if(r.jointDepth>0)for(unsigned i=0;i<2;i++){
  double x=i?.29:-.27,y=i?.035:-.105;std::vector<Plane> cut;
  add(cut,{1,0,0},x+.13);add(cut,{-1,0,0},-x+.12);
  add(cut,{.035,1,0},y+.007+.004*r.jointDepth);add(cut,{-.035,-1,0},-y+.007+.004*r.jointDepth);
  add(cut,{0,0,-1},-(.32-.19*r.jointDepth));f.fractures.push_back(std::move(cut));
 }
 return f;
}
static double coherentNoise(double t,std::uint32_t seed){auto cell=static_cast<std::uint32_t>(std::floor(t));double f=t-std::floor(t);double blend=f*f*(3-2*f);return 2*((1-blend)*rand01(seed,cell+300)+blend*rand01(seed,cell+301))-1;}
static double profileNoise(double t,std::uint32_t seed){return .7*coherentNoise(t,seed)+.3*coherentNoise(2*t+7,seed+0x9e3779b9U);}
double resistance(const RockField&f,Vec3 p){if(f.recipe.generatorVersion=="rocks-v12")return .5;double phase=p.y/f.recipe.layerThickness+.2;double fraction=phase-std::floor(phase);double d=std::abs(fraction-.5);double t=std::clamp((d-.10)/.20,0.,1.);double weak=1-t*t*(3-2*t);return .5+f.recipe.contrast*(.5-weak);}
static double host(const RockField&f,Vec3 p,Stage s){double result=-1e100;double setback=(s==Stage::Base||f.recipe.schemaVersion==2)?0:f.amplitude*.7*(1-resistance(f,p));for(auto pl:f.planes){double lateral=std::hypot(pl.normal.x,pl.normal.z);result=std::max(result,planeValue(pl,p)+setback*lateral);}if(s!=Stage::Base&&!f.masses.empty()){double joined=1e100;for(const auto&mass:f.masses){double value=-1e100;for(auto pl:mass)value=std::max(value,planeValue(pl,p));joined=std::min(joined,value);}result=std::max(result,joined);}if(s!=Stage::Base&&f.recipe.schemaVersion==2){for(const auto&cut:f.fractures){double c=-1e100;for(auto pl:cut)c=std::max(c,planeValue(pl,p));result=std::max(result,-c);}}return result;}
// Heightmap-informed structure: uneven connected plateaus with coherent beds
// and staggered cross joints. This is procedural geometry, not a sampled map.
static RockField plateauField(const Recipe&r){
 RockField f{r,{}, {},0,{}};const double s=r.size;auto random=[&](unsigned i){return rand01(r.seed,i);};
 auto add=[&](std::vector<Plane>&planes,Vec3 n,double d){double l=length(n);planes.push_back({n/l,d*s/l});};
 add(f.planes,{1,.08,-.12},.55);add(f.planes,{-1,-.13,.08},.54);add(f.planes,{.12,1,0},.44);add(f.planes,{-.05,-1,0},.44);add(f.planes,{0,0,1},.46);add(f.planes,{0,0,-1},.20);
 add(f.planes,{.9,.6,0},.67);add(f.planes,{-.8,.7,0},.64);add(f.planes,{.7,-.8,0},.64);add(f.planes,{-.8,-.65,0},.67);
 std::vector<Plane> backing;add(backing,{.025,-.04,1},.105);f.masses={backing};
 struct Site{Vec3 p;double weight;unsigned bed;};std::vector<Site> sites;
 const unsigned counts[3]={2,3,4};const double centers[3][4]={{-.34,.24,0,0},{-.43,-.015,.39,0},{-.50,-.17,.18,.48}},levels[3]={-.29,-.035,.29};
 for(unsigned row=0;row<3;row++)for(unsigned col=0;col<counts[row];col++){
  unsigned i=unsigned(sites.size());double x=centers[row][col]+.10*(random(2000+i)-.5),y=levels[row]+.11*(random(2020+i)-.5)+.045*std::sin(4*x+random(1800));
  sites.push_back({{x,y,0},.012*(random(2040+i)-.5),row});
 }
 auto clip=[](const std::vector<Vec3>&polygon,Plane plane){std::vector<Vec3> out;for(std::size_t i=0;i<polygon.size();i++){Vec3 a=polygon[i],b=polygon[(i+1)%polygon.size()];double da=planeValue(plane,a),db=planeValue(plane,b);if(da<=0)out.push_back(a);if((da<=0)!=(db<=0))out.push_back(a+(b-a)*(da/(da-db)));}return out;};
 for(unsigned i=0;i<sites.size();i++){
  std::vector<std::pair<Plane,unsigned>> bisectors;std::vector<Vec3> footprint{{-.8,-.65,0},{.8,-.65,0},{.8,.65,0},{-.8,.65,0}};
  for(unsigned j=0;j<sites.size();j++)if(i!=j){
   const auto&a=sites[i];const auto&b=sites[j];Vec3 n{2*(b.p.x-a.p.x),3.6*(b.p.y-a.p.y),0};double d=b.p.x*b.p.x-a.p.x*a.p.x+1.8*(b.p.y*b.p.y-a.p.y*a.p.y)+a.weight-b.weight,l=length(n);
   Plane plane{n/l,d/l};bisectors.push_back({plane,j});footprint=clip(footprint,plane);
  }
  if(footprint.size()<3)throw std::runtime_error("empty procedural plateau cell");
  std::vector<Plane> block;
  for(auto[plane,j]:bisectors){bool active=false;for(auto p:footprint)if(std::abs(planeValue(plane,p))<1e-8)active=true;if(!active)continue;
   unsigned pair=std::min(i,j)*9+std::max(i,j);double selector=random(2060+pair);double gap=selector<.26?-.005:.003+.018*random(2160+pair);
   block.push_back({plane.normal,(plane.offset-gap)*s});
  }
  const auto&site=sites[i];const double hardness[3]={.82,.34,.73};double height=.205+.045*hardness[site.bed]+.065*(random(2280+i)-.5),nx=.52*(random(2300+i)-.5),ny=.42*(random(2320+i)-.5);
  add(block,{nx,ny,1},height+nx*site.p.x+ny*site.p.y);
  if(i%3==0){double side=i%2?1.:-1.;add(block,{nx+.48*side,ny+.12,1},height+(nx+.48*side)*site.p.x+(ny+.12)*site.p.y+.045);}
  f.masses.push_back(std::move(block));
 }

 return f;
}
// The v4 family host is an irregular convex envelope, never the v3 box.
// Cuts remain finite inside that envelope and forward of its central core.
static RockField geologicalFamily(const Recipe&r){
 RockField f{r,{}, {},0,{}};const double s=r.size;
 auto random=[&](unsigned i){return rand01(r.seed,i);};
 auto plane=[&](std::vector<Plane>&out,Vec3 n,double d){double l=length(n);out.push_back({n/l,d*s/l});};
 const unsigned archetype=(r.seed%7)%3;
 const Vec3 boulderAxes[]={{.48,.28,.37},{.35,.45,.32},{.52,.30,.29}};
 const Vec3 cliffAxes[]={{.31,.49,.26},{.41,.45,.24},{.36,.51,.30}};
 const Vec3 strataAxes[]={{.54,.29,.36},{.47,.36,.34},{.55,.26,.32}};
 Vec3 axes=r.family=="cliff"?cliffAxes[archetype]:r.family=="strata"?strataAxes[archetype]:boulderAxes[archetype];
 if(r.generatorVersion=="rocks-v5"&&r.family=="cliff")axes=archetype==0?Vec3{.53,.42,.29}:archetype==1?Vec3{.45,.48,.28}:Vec3{.55,.38,.27};
 axes.x*=.88+.22*random(401);axes.y*=.88+.22*random(402);axes.z*=.88+.22*random(403);
 // Unequally spaced directions and independently varying support distances
 // produce broad fracture facets, not a regular sphere or a beveled cube.
 const Vec3 directions[]={{1,.12,.14},{-.93,.18,-.16},{.08,1,-.12},{-.17,-1,.13},{.14,.09,1},{-.12,-.15,-1},{.81,.76,.42},{-.79,.51,.82},{.62,-.68,.83},{-.65,-.79,.33},{.48,.59,-.88},{-.77,-.33,-.72},{.03,.82,.71},{.61,-.15,-.91}};
 for(unsigned i=0;i<14;i++){
  Vec3 n=directions[i];n.x+=(random(420+i*3)-.5)*.52;n.y+=(random(421+i*3)-.5)*.46;n.z+=(random(422+i*3)-.5)*.48;n=normalized(n);
  if(r.generatorVersion=="rocks-v5"&&r.family=="common_rock"){
   double yaw=2*std::numbers::pi*random(800),tilt=.42*(random(801)-.5);Vec3 q{n.x*std::cos(yaw)+n.z*std::sin(yaw),n.y,-n.x*std::sin(yaw)+n.z*std::cos(yaw)};
   n={q.x*std::cos(tilt)-q.y*std::sin(tilt),q.x*std::sin(tilt)+q.y*std::cos(tilt),q.z};
  }
  Vec3 scaled{n.x/axes.x,n.y/axes.y,n.z/axes.z};
  plane(f.planes,scaled,.87+.18*random(510+i));
 }
 // Convex subtractive wedges: local exposed recesses with unequal side
 // slopes, oblique floors and tapered ends. No periodic ring grooves.
 auto recess=[&](double cx,double cy,double angle,double span,double height,double floor,unsigned i){
  cx+=(random(i)-.5)*.11+r.detail*.012*profileNoise(i*.31,r.seed);cy+=(random(i+1)-.5)*.09;angle+=(random(i+2)-.5)*.25+r.detail*.08*profileNoise(i*.23,r.seed+91);
  span*=.82+.33*random(i+3);height*=.78+.38*random(i+4);floor+=.05*(random(i+5)-.5);
  Vec3 u{std::cos(angle),std::sin(angle),0},v{-std::sin(angle),std::cos(angle),0};Vec3 center{cx,cy,0};
  std::vector<Plane> cut;auto add=[&](Vec3 n,double d){plane(cut,n,d+dot(n,center));};
  add(u+Vec3{0,0,-.30},span-.09);add(u*-1+Vec3{0,0,-.17},span*.77-.051);
  add(v+Vec3{0,0,-.14},height-.042);add(v*-1+Vec3{0,0,-.42},height*.56-.126);
  add({.13*(random(i+6)-.5),.18*(random(i+7)-.5),-1},-floor);
  f.fractures.push_back(std::move(cut));
 };
 if(r.family=="common_rock"){
  // A broken outside corner is a new support facet, not a pocket drilled
  // into the middle of the rock. These chips preserve a convex boulder.
  for(unsigned i=0;i<3;i++){
   Vec3 n=normalized(Vec3{(i==1?-1.:1.)*(.65+.25*random(610+i)),(i==2?-1.:1.)*(.45+.45*random(620+i)),.7+.2*random(630+i)});
   Vec3 scaled{n.x/axes.x,n.y/axes.y,n.z/axes.z};std::vector<Plane> cut;
   plane(cut,scaled*-1,-(.87+.06*random(640+i)+r.detail*.025*profileNoise(i+.3,r.seed)));f.fractures.push_back(std::move(cut));
  }
 }else if(r.family=="cliff"){
  const double side=archetype==1?-1.:1.;
  recess(-.13*side,.26,archetype==2?.96:1.30,.52,.070,.070,610);
  recess(.34*side,-.22,archetype==1?-.30:.18,.25,.064,.15,630);
  // A crown edge break rather than another central rectangular hole.
  recess(-.28*side,.43,-.24,.23,.046,.18,650);
 }else{
  // Partial bedding ledges enter from alternating silhouette edges.
  // A single tilted bedding frame is shared, with nonperiodic contacts.
  const double tilt=-.05-.10*random(700);
  const double levels[3][2]={{-.15,.11},{-.10,.18},{-.17,.065}};
  for(unsigned i=0;i<2;i++){
   double sign=(i+archetype)%2?1.:-1.;double y=levels[archetype][i]+.015*(random(710+i)-.5);
   double x=sign*.27,front=1;
   for(auto p:f.planes)if(p.normal.z>1e-6)front=std::min(front,(p.offset/s-p.normal.x*x-p.normal.y*y)/p.normal.z);
   double floor=std::max(.10,front-(.05+.035*random(720+i)));
   double half=(i==0?.095:.064)+.014*random(730+i);
   std::vector<Plane> cut;
   plane(cut,{tilt,1,0},y+half);
   plane(cut,{-tilt,-1,0},-y+half);
   plane(cut,{-sign,.25,-.15},.03+.14*random(740+i)+r.detail*.015*profileNoise(i+.2,r.seed));
   plane(cut,{.035,-.05,-1},-floor+.035*x-.05*y);
   f.fractures.push_back(std::move(cut));
  }
 }
 if(r.generatorVersion=="rocks-v5"&&r.family=="strata"){
  f.fractures.clear();
  // Positive rock beds overlap in height and share a continuous rear mass.
  // The outer envelope bounds all of these convex, otherwise open masses.
  f.masses={};std::vector<Plane> backbone;plane(backbone,{0,0,1},-.035);f.masses.push_back(backbone);
  const double boundaries[4]={-.58,-.125,.105,.55};
  const double tilt=-.065-.07*random(900);
  for(unsigned i=0;i<3;i++){
   std::vector<Plane> bed;double front=.18+.13*random(910+i);
   plane(bed,{tilt,1,0},boundaries[i+1]+.014);
   plane(bed,{-tilt,-1,0},-boundaries[i]+.014);
   plane(bed,{-.16+.32*random(920+i),.04+.09*random(930+i),1},front);
   plane(bed,{.65,.08,1},front+.18+.05*random(940+i));
   plane(bed,{-.72,-.10,1},front+.17+.05*random(950+i));
   f.masses.push_back(std::move(bed));
  }
 }
 if(r.generatorVersion=="rocks-v5"&&r.family=="cliff"){
  f.fractures.clear();std::vector<Plane> backbone;plane(backbone,{0,0,1},-.05);f.masses={backbone};
  const double divisions[4]={-.72,-.19,.15,.72};
  const double lean=(archetype==1?-.22:.14)+.12*(random(1000)-.5);
  for(unsigned i=0;i<3;i++){
   std::vector<Plane> buttress;double front=.16+.14*random(1010+i);
   plane(buttress,{1,lean,0},divisions[i+1]+.025);
   plane(buttress,{-1,-lean,0},-divisions[i]+.025);
   plane(buttress,{-.2+.4*random(1020+i),.1*(random(1030+i)-.5),1},front);
   plane(buttress,{.72,.18,1},front+.17);
   plane(buttress,{-.63,-.15,1},front+.15);
   plane(buttress,{.18,1,-.08},.29+.21*random(1040+i));
   f.masses.push_back(std::move(buttress));
  }
 }
 // Normalize the entire construction uniformly if an oblique crown grows
 // too tall. This preserves facets and cut proportions without box caps.
 double extent=0;
 for(std::size_t a=0;a<f.planes.size();a++)for(std::size_t b=a+1;b<f.planes.size();b++)for(std::size_t c=b+1;c<f.planes.size();c++){
  auto p=f.planes[a],q=f.planes[b],t=f.planes[c];double det=dot(p.normal,cross(q.normal,t.normal));if(std::abs(det)<1e-10)continue;
  Vec3 v=(cross(q.normal,t.normal)*p.offset+cross(t.normal,p.normal)*q.offset+cross(p.normal,q.normal)*t.offset)/det;
  bool inside=true;for(auto h:f.planes)if(planeValue(h,v)>1e-9*s){inside=false;break;}
  if(inside)extent=std::max(extent,std::max({std::abs(v.x),std::abs(v.y),std::abs(v.z)}));
 }
 if(extent>.66*s){double scale=.66*s/extent;for(auto& p:f.planes)p.offset*=scale;for(auto& cut:f.fractures)for(auto&p:cut)p.offset*=scale;for(auto& mass:f.masses)for(auto&p:mass)p.offset*=scale;}
 return f;
}
RockField makeField(const Recipe&r){auto e=validateRecipe(r,makeGrid(r,128));if(!e.empty())throw std::invalid_argument(e.front());if(r.generatorVersion=="rocks-v12")return r.family=="columnar_basalt"?basaltField(r):RockField{r,{}, {},0,{}};if(r.generatorVersion=="rocks-v8")return plateauField(r);if(r.generatorVersion=="rocks-v7")return RockField{r,{}, {},0,{}};if(r.generatorVersion=="rocks-v4")return geologicalFamily(r);if(r.generatorVersion=="rocks-v5"||r.generatorVersion=="rocks-v6"){auto macro=r;macro.generatorVersion="rocks-v5";macro.detail=0;auto f=geologicalFamily(macro);f.recipe=r;if(r.detail>0&&r.generatorVersion=="rocks-v5")addEdgeChips(f);return f;}RockField f{r,{}, {},boundedAmplitude(100,r.size*.3,r.layerThickness),{}};double s=r.size;
 f.planes={{{1,0,0},.5*s},{{-1,0,0},.5*s},{{0,1,0},.35*s},{{0,-1,0},.35*s},{{0,0,1},.4*s},{{0,0,-1},.4*s}};
 f.planes.push_back({normalized({1,.34+.18*rand01(r.seed,101),.12+.16*rand01(r.seed,102)}),(.42+.04*rand01(r.seed,103))*s});
 f.planes.push_back({normalized({-1,-.10-.15*rand01(r.seed,104),.08}),(.47+.04*rand01(r.seed,105))*s});
 f.planes.push_back({normalized({.06,.10,1}),.4*s});
 const Vec3 normals[]={{1,.07,-1},{-1,.04,-.8}};
 for(unsigned i=0;i<2;i++){Vec3 n=normals[i];n.x+=(rand01(r.seed,i+1)-.5)*.12;n=normalized(n);f.planes.push_back({n,(.45+.035*rand01(r.seed,i+20))*s});}
 if(r.schemaVersion==2){
 // Large exposed oblique facets break the intact front prism before damage.
 auto facet=[&](Vec3 n,double d){double l=length(n);f.planes.push_back({n/l,d*s/l});};
 if(r.family=="fractured_wall"){facet({.50,.22,1},.43);facet({-.55,-.45,1},.44);facet({-.28,1,.18},.37);}
 else if(r.family=="irregular_beds"){facet({.36,-.12,1},.46);facet({-.42,.20,1},.47);facet({-.18,1,.08},.34);}
 else{facet({.45,.16,1},.47);facet({-.40,-.50,1},.43);facet({.20,1,.08},.36);}
 // Finite asymmetric tapered prisms. z points toward the exposed wall face.
 // All cut floors remain forward of z=-.16*size; the rear slab is untouched.
 auto wedge=[&](double cx,double cy,double angle,double halfLength,double halfWidth,double floor){
  double index=double(f.fractures.size());double na=profileNoise(index*.61+.17,r.seed),nb=profileNoise(index*.61+1.43,r.seed+91),nc=profileNoise(index*.61+2.31,r.seed+173);
  cx+=r.detail*.025*na;cy+=r.detail*.020*nb;angle+=r.detail*.17*nc;halfLength*=1+r.detail*.10*na;halfWidth*=1+r.detail*.18*nb;floor=std::max(-.18,floor+r.detail*.025*nc);
  Vec3 u{std::cos(angle),std::sin(angle),0},v{-std::sin(angle),std::cos(angle),0};
  Vec3 center{cx*s,cy*s,0};double depth=.4-floor;halfWidth=std::max(halfWidth,.06);double slope=(halfWidth-.052)/depth;
  std::vector<Plane> cut;auto add=[&](Vec3 n,double d){double l=length(n);cut.push_back({n/l,(d+dot(n,center))/l});};
  add(u+Vec3{0,0,-.08},halfLength*s-.08*.4*s);
  add(u*-1+Vec3{0,0,-.13},halfLength*s-.13*.4*s);
  add(v+Vec3{0,0,-slope},halfWidth*s-slope*.4*s);
  add(v*-1+Vec3{0,0,-slope*.72},halfWidth*.8*s-slope*.72*.4*s);
  if(r.detail>0){double a=r.detail*.18*(.65+.35*na),b=r.detail*.16*(.65+.35*nb);add(v+u*a+Vec3{0,0,-slope},halfWidth*s+.025*r.detail*s-slope*.4*s);add(v*-1-u*b+Vec3{0,0,-slope*.72},halfWidth*.8*s+.020*r.detail*s-slope*.72*.4*s);}
  add({0,0,-1},-floor*s);add({0,0,1},.8*s);f.fractures.push_back(cut);
 };
 double j=(rand01(r.seed,201)-.5)*.025;
 if(r.family=="fractured_wall"){
  wedge(-.13+j,.03,1.12,.43,.095,-.12);
  wedge(.22,-.07+j,.18,.29,.065,-.035);
  wedge(-.36,.23,-.45,.16,.06,.11);
 }else if(r.family=="irregular_beds"){
  wedge(-.12+j,-.19,-.12,.34,.040,-.055);
  wedge(.15,.015+j,.10,.32,.055,-.10);
  wedge(-.19,.20,-.08,.30,.037,.035);
 }else{
  wedge(-.04+j,-.055,.07,.38,.18,-.15);
  wedge(.32,.20,1.2,.22,.075,.02);
 }
 wedge(.40,.27,-.62,.19,.11,.035);
 wedge(-.39,-.28,.52,.22,.095,-.015);

 }
 for(unsigned i=0;i<4;i++){double angle=(i+.08+.2*rand01(r.seed,i+31))*2*std::numbers::pi/4;Vec3 dir{std::cos(angle),0,std::sin(angle)};double approximate=(-.23+.46*rand01(r.seed,i+41))*s;double y=(std::round(approximate/r.layerThickness-.3)+.3)*r.layerThickness;y=std::clamp(y,-.26*s,.26*s);double lo=0,hi=s;for(int j=0;j<45;j++){double mid=(lo+hi)*.5;if(host(f,dir*mid+Vec3{0,y,0},Stage::Stratified)<0)lo=mid;else hi=mid;}double radius=(.21+.035*rand01(r.seed,i+51))*s;f.cutters.push_back({dir*((lo+hi)*.5+.08*radius)+Vec3{0,y,0},radius});}return f;
}
double erosionMask(const RockField&f,Vec3 p){double m=0;for(auto c:f.cutters){double t=std::clamp((.90-length(p-c.center)/c.radius)/.32,0.,1.);m=std::max(m,t*t*(3-2*t));}return m;}
double fieldValue(const RockField&f,Vec3 p,Stage s){if(f.recipe.generatorVersion=="rocks-v12"&&f.recipe.family=="jointed_granite")return graniteValue(f.recipe,p,s);if(f.recipe.generatorVersion=="rocks-v7")return weatheredValue(f.recipe,p,s);double h=host(f,p,s);if(s!=Stage::Eroded||f.recipe.wear==0)return h;double m=erosionMask(f,p);if(m==0)return h;double cutter=1e100;for(auto c:f.cutters)cutter=std::min(cutter,length(p-c.center)-c.radius);double b=f.recipe.wear*m*(1-resistance(f,p))*f.amplitude;double effective=std::max(cutter,-h-b);return subtractFields(h,effective);}
Sample evaluate(const RockField&f,Vec3 p,Stage s){
 if(!f.masses.empty()&&f.recipe.wear==0){
  // Exact active-plane subgradient. At a tie, input plane/mass order wins;
  // a crease has no unique smooth normal. Exported face normals stay per-corner.
  auto convex=[&](const std::vector<Plane>&planes){Sample q{-1e100,{},resistance(f,p)};for(auto pl:planes){double v=planeValue(pl,p);if(v>q.value){q.value=v;q.gradient=pl.normal;}}return q;};
  auto result=convex(f.planes);
  if(s!=Stage::Base){Sample joined{1e100,{},result.resistance};for(const auto&mass:f.masses){auto q=convex(mass);if(q.value<joined.value)joined=q;}if(joined.value>result.value)result=joined;
   for(const auto&cut:f.fractures){auto q=convex(cut);if(-q.value>result.value){result.value=-q.value;result.gradient=q.gradient*-1;}}
  }return result;
 }
double d=1e-5*f.recipe.size;Vec3 g{};if(s==Stage::Base&&f.recipe.schemaVersion==1){double best=-1e100;for(auto pl:f.planes){double v=planeValue(pl,p);if(v>best){best=v;g=pl.normal;}}}else{g={(fieldValue(f,p+Vec3{d,0,0},s)-fieldValue(f,p-Vec3{d,0,0},s))/(2*d),(fieldValue(f,p+Vec3{0,d,0},s)-fieldValue(f,p-Vec3{0,d,0},s))/(2*d),(fieldValue(f,p+Vec3{0,0,d},s)-fieldValue(f,p-Vec3{0,0,d},s))/(2*d)};}return {fieldValue(f,p,s),g,resistance(f,p)};}
}
