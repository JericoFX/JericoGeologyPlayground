// Native, deterministic port of the frozen approved glass_fracture.py.
// Python is an offline test oracle only; this unit has no runtime dependencies.
#include "jerico/scene.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace jerico {
namespace {
using Polygon=std::vector<V2>;
using Bounds=std::array<double,4>;
constexpr double pi=3.14159265358979323846264338327950288;
constexpr double tau=2*pi;
constexpr double epsilon=1e-10;

V2 lerp(V2 a,V2 b,double t) { return {a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t}; }
double distance(V2 a,V2 b) { return std::hypot(a[0]-b[0],a[1]-b[1]); }
double cross2(V2 a,V2 b,V2 c) { return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]); }
double signed_area(const Polygon&p) {
 double sum=0;
 if(p.empty())return 0;
 for(size_t i=1;i+1<p.size();++i)sum+=cross2(p[0],p[i],p[i+1]);
 return sum/2;
}
double area(const Polygon&p) { return std::abs(signed_area(p)); }
double positive_modulo(double a,double b) {
 double result=std::fmod(a,b);
 if(result<0)result+=b;
 else if(result==0)result=std::copysign(0.0,b);
 return result;
}
bool on_segment(V2 p,V2 a,V2 b) {
 const double length=distance(a,b);
 return std::abs(cross2(a,b,p))<=epsilon*std::max(length,epsilon)
     &&std::min(a[0],b[0])-epsilon<=p[0]&&p[0]<=std::max(a[0],b[0])+epsilon
     &&std::min(a[1],b[1])-epsilon<=p[1]&&p[1]<=std::max(a[1],b[1])+epsilon;
}
bool intersects(V2 a,V2 b,V2 c,V2 d) {
 const double ac=cross2(a,b,c),ad=cross2(a,b,d),ca=cross2(c,d,a),cb=cross2(c,d,b);
 return (ac*ad<0&&ca*cb<0)||on_segment(c,a,b)||on_segment(d,a,b)||on_segment(a,c,d)||on_segment(b,c,d);
}
// Exact source validation, including epsilon, edge reversals and duplicate points.
// The source's returned CCW copy is deliberately ignored by its caller here.
void validate_profile(const Polygon&p) {
 if(p.size()<3||p.size()>64)throw std::invalid_argument("Cutter must contain 3..64 vertices");
 for(const auto&a:p)if(!std::isfinite(a[0])||!std::isfinite(a[1]))throw std::invalid_argument("Cutter point must contain finite numeric coordinates");
 for(size_t i=0;i<p.size();++i)for(size_t j=i+1;j<p.size();++j)
  if(distance(p[i],p[j])<=epsilon)throw std::invalid_argument("Cutter has repeated or coincident vertices");
 const size_t n=p.size();
 for(size_t i=0;i<n;++i){
  const V2 a=p[i],b=p[(i+1)%n],prev=p[(i+n-1)%n];
  for(size_t j=i+1;j<n;++j){
   if(j==i+1||(i==0&&j==n-1))continue;
   if(intersects(a,b,p[j],p[(j+1)%n]))throw std::invalid_argument("Cutter must be a simple, non-self-intersecting polygon");
  }
  if(std::abs(cross2(prev,a,b))<=epsilon*distance(prev,b)){
   // Preserve the source _dot's accumulation order, including the zero Y term.
   double product=0;product+=(a[0]-prev[0])*(b[0]-a[0]);product+=0;product+=(a[1]-prev[1])*(b[1]-a[1]);
   if(product<0)throw std::invalid_argument("Cutter doubles back along an edge");
  }
 }
 if(std::abs(signed_area(p))<=1e-12)throw std::invalid_argument("Cutter has zero or unsupported tiny area");
}
bool contains(const Polygon&p,V2 point,bool boundary=true) {
 bool inside=false;const double x=point[0],z=point[1];
 for(size_t i=0;i<p.size();++i){
  const V2 a=p[i],b=p[(i+1)%p.size()];
  if(on_segment(point,a,b))return boundary;
  if((a[1]>z)!=(b[1]>z)){
   const double crossing=a[0]+(z-a[1])*(b[0]-a[0])/(b[1]-a[1]);
   if(x<crossing)inside=!inside;
  }
 }
 return inside;
}
Polygon clip_rect(Polygon polygon,Bounds bounds) {
 const double x0=bounds[0],x1=bounds[1],z0=bounds[2],z1=bounds[3];
 const std::array<V2,4> corners={V2{x0,z0},V2{x1,z0},V2{x1,z1},V2{x0,z1}};
 for(size_t edge=0;edge<4;++edge){
  if(polygon.empty())return {};
  const V2 a=corners[edge],b=corners[(edge+1)%4];Polygon clipped;
  for(size_t i=0;i<polygon.size();++i){
   const V2 p=polygon[i],q=polygon[(i+1)%polygon.size()];
   const double sp=cross2(a,b,p),sq=cross2(a,b,q);
   if(sp>=0)clipped.push_back(p);
   if((sp>=0)!=(sq>=0))clipped.push_back(lerp(p,q,sp/(sp-sq)));
  }
  polygon=std::move(clipped);
 }
 return polygon;
}
double support(const Polygon&p,double w,double h) {
 const std::array<Bounds,4> strips={Bounds{-w/2-.011,-w/2,-.011,h+.011},Bounds{w/2,w/2+.011,-.011,h+.011},Bounds{-w/2,w/2,-.011,0},Bounds{-w/2,w/2,h,h+.011}};
 double sum=0;
 for(const auto&strip:strips){const auto clipped=clip_rect(p,strip);sum+=clipped.size()>=3?area(clipped):0;}
 return sum;
}
std::pair<V2,double> ray_rect(double cx,double cz,double a,Bounds bounds) {
 const double dx=std::cos(a),dz=std::sin(a),infinity=std::numeric_limits<double>::infinity();
 const double length=std::min(std::abs(dx)>1e-12?((dx>0?bounds[1]:bounds[0])-cx)/dx:infinity,
                              std::abs(dz)>1e-12?((dz>0?bounds[3]:bounds[2])-cz)/dz:infinity);
 return {{cx+length*dx,cz+length*dz},length};
}
bool finite_number(const Json&value) { return value.is_number()&&std::isfinite(value.get<double>()); }
struct Controls { Json base;double energy;int count;V2 impact; };
Controls controls(const Json&recipe) {
 if(!recipe.is_object())throw std::invalid_argument("Recipe must be an object");
 Controls c;c.base=recipe;
 const Json energy=recipe.value("energy",Json(.66));
 const Json count=recipe.value("ray_count",Json(18));
 const Json impact=recipe.value("impact",Json::array({.15,.56}));
 c.base.erase("energy");c.base.erase("ray_count");c.base.erase("impact");
 if(!finite_number(energy)||energy.get<double>()<.1||energy.get<double>()>1)throw std::invalid_argument("energy must be finite in 0.1..1");
 if(!count.is_number_integer()||count.get<double>()<12||count.get<double>()>40)throw std::invalid_argument("ray_count must be integer 12..40");
 if(!impact.is_array()||impact.size()!=2||!finite_number(impact[0])||!finite_number(impact[1])
    ||impact[0].get<double>()<-.3||impact[0].get<double>()>.3||impact[1].get<double>()<.25||impact[1].get<double>()>.75)
  throw std::invalid_argument("impact is [normalized X -0.3..0.3, Z 0.25..0.75]");
 c.energy=energy.get<double>();c.count=count.get<int>();c.impact=impact.get<V2>();return c;
}
struct UnsafeFractureGeometry:std::invalid_argument {using std::invalid_argument::invalid_argument;};

Json build(const Json&recipe,bool straight_rays) {
 const Controls control=controls(recipe);
 Json scene=build_industrial(control.base);
 // The lead's legacy pane placeholder is discarded below and never exported.
 scene["metadata"].erase("native_legacy_glass_pending");
 const Json normalized=scene.at("recipe");
 if(scene.at("metadata").at("preset")!="industrial_window")throw std::invalid_argument("Fracture study requires industrial_window");
 scene["recipe"]["base_recipe"]=control.base;
 scene["recipe"]["energy"]=control.energy;scene["recipe"]["ray_count"]=control.count;scene["recipe"]["impact"]=control.impact;
 scene["metadata"]["generator"]="JericoProceduralCppLab/fracture.cpp";
 const double w=normalized.at("width"),h=normalized.at("height");
 const Bounds bounds={-w/2-.011,w/2+.011,-.011,h+.011};
 for(auto&p:scene["parts"])if(p["role"]=="GlassPane"){p["glass_study"]=true;p["glass_bounds"]=bounds;}
 if(normalized.value("glazing",std::string("intact"))!="broken")return scene;
 const double cx=control.impact[0]*w,cz=control.impact[1]*h;
 const int64_t seed=normalized.at("seed").get<int64_t>();
 PythonRandom rng(seed,53813);
 std::vector<double> weights;double sum=0;
 for(int i=0;i<control.count;++i){const double weight=rng.uniform(.40,1.65);weights.push_back(weight);sum+=weight;}
 double a=rng.uniform(0,.25);std::vector<double> angles;
 for(double weight:weights){angles.push_back(positive_modulo(a,tau));a+=weight/sum*tau;}
 for(V2 corner_point:{V2{bounds[0],bounds[2]},V2{bounds[1],bounds[2]},V2{bounds[1],bounds[3]},V2{bounds[0],bounds[3]}}){
  const double corner=positive_modulo(std::atan2(corner_point[1]-cz,corner_point[0]-cx),tau);
  angles.erase(std::remove_if(angles.begin(),angles.end(),[&](double angle){return !(std::abs(positive_modulo(angle-corner+pi,tau)-pi)>.085);}),angles.end());
  angles.push_back(corner);
 }
 std::sort(angles.begin(),angles.end());const size_t n=angles.size();std::vector<Polygon> rays;
 for(size_t i=0;i<n;++i){
  a=angles[i];const auto ray_hit=ray_rect(cx,cz,a,bounds);const V2 outer=ray_hit.first;const double dist=ray_hit.second;
  const double prev=positive_modulo(a-angles[(i+n-1)%n],tau),next=positive_modulo(angles[(i+1)%n]-a,tau),space=std::min(prev,next);
  const double rx=w*(.26+.14*control.energy),rz=h*(.26+.14*control.energy);
  const double cos_term=std::cos(a)/rx,sin_term=std::sin(a)/rz;
  const double impact_radius=1/std::sqrt(cos_term*cos_term+sin_term*sin_term);
  const double irregular=1+.18*std::sin(2*a+.7)+.12*std::cos(3*a+1.4)+rng.uniform(-.19,.19);
  const double hole=std::min(.84,impact_radius*irregular/dist);
  const std::array<double,5> levels={hole,hole+(1-hole)*.24,hole+(1-hole)*.53,hole+(1-hole)*.77,1};
  Polygon ray;
  for(size_t j=0;j<levels.size();++j){
   // Draw even on the straight retry: the source preserves every RNG draw.
   const double perturb=j<4?rng.uniform(-.16,.16)*space:0;
   const double aa=a+(straight_rays?0:perturb),t=levels[j];
   ray.push_back(j<4?V2{cx+std::cos(aa)*dist*t,cz+std::sin(aa)*dist*t}:outer);
  }
  rays.push_back(std::move(ray));
 }
 Polygon holepoly;for(const auto&ray:rays)holepoly.push_back(ray[0]);
 double source_area=area(holepoly);Json parts=Json::array(),gap_samples=Json::array(),solid_samples=Json::array();int branches=0;
 Builder builder(scene["recipe"]);constexpr double nominal_gap=.0012;
 for(size_t i=0;i<n;++i){
  const auto&left=rays[i];const auto&right=rays[(i+1)%n];
  Polygon source=left;source.insert(source.end(),right.rbegin(),right.rend());source_area+=area(source);
  Polygon L,R;
  for(size_t j=0;j<5;++j){const double margin=std::min(.08,nominal_gap*.5/distance(left[j],right[j]));L.push_back(lerp(left[j],right[j],margin));R.push_back(lerp(left[j],right[j],1-margin));}
  gap_samples.push_back(lerp(left[2],left[3],.5));
  Polygon outline={L[0]};std::vector<std::pair<size_t,V2>> branch_tips;
  const int k=rng.random()<.60?1:2;
  for(size_t j=1;j<5;++j){
   if(j==static_cast<size_t>(k)){
    branch_tips.push_back({outline.size()+1,L[j]});outline.push_back(L[j]);
    outline.push_back(lerp(L[j],R[j],rng.uniform(.38,.79)));
    outline.push_back(lerp(L[j],L[j+1],rng.uniform(.010,.019)));++branches;
   }else outline.push_back(L[j]);
  }
  outline.push_back(R[4]);
  for(int j=3;j>=0;--j){
   if(j==3&&rng.random()<.66){
    branch_tips.push_back({outline.size()+1,R[j]});outline.push_back(R[j]);
    outline.push_back(lerp(R[j],L[j],rng.uniform(.28,.67)));
    outline.push_back(lerp(R[j],R[j-1],rng.uniform(.011,.024)));++branches;
   }else outline.push_back(R[j]);
  }
  const V2 p=R[0],q=L[0],mid=lerp(p,q,.54),away={mid[0]-cx,mid[1]-cz};const double length=std::hypot(away[0],away[1]);
  const double chip=std::min(rng.uniform(.002,.014)*(i%5?1:2.4),.22*std::min(distance(L[0],L[1]),distance(R[0],R[1])));
  outline.push_back(lerp(p,q,.38));outline.push_back({mid[0]+away[0]/length*chip,mid[1]+away[1]/length*chip});outline.push_back(lerp(p,q,.70));
  const Polygon original=outline;bool valid=false;
  for(double safety_scale:{1.,.60,.30,.10}){
   Polygon candidate=original;
   for(const auto&tip:branch_tips)candidate[tip.first]=lerp(tip.second,original[tip.first],safety_scale);
   try{validate_profile(candidate);}catch(const std::invalid_argument&){continue;}
   outline=std::move(candidate);valid=true;break;
  }
  if(!valid){
   std::set<size_t> omitted;for(const auto&tip:branch_tips){omitted.insert(tip.first);omitted.insert(tip.first+1);}
   outline.clear();for(size_t j=0;j<original.size();++j)if(!omitted.count(j))outline.push_back(original[j]);
   branches-=static_cast<int>(branch_tips.size());
   try{validate_profile(outline);}catch(const std::invalid_argument&){outline=L;outline.insert(outline.end(),R.rbegin(),R.rend());validate_profile(outline);}
  }
  if(signed_area(outline)<0)std::reverse(outline.begin(),outline.end());
  std::ostringstream id;id<<"window.glass.shard."<<std::setw(2)<<std::setfill('0')<<i;
  auto&part=builder.prism(id.str(),"frame",outline,-.002,.002,"glass",0,"GlassPane");
  const size_t capcount=(outline.size()-2)*2;Json materials=Json::array();
  for(size_t j=0;j<capcount;++j)materials.push_back("glass");
  for(size_t j=0;j<outline.size();++j)materials.push_back("glass_edge");
  part["face_materials"]=std::move(materials);part["glass_study"]=true;part["glass_bounds"]=bounds;
  part["footprint_xz"]=outline;part["footprint_area_m2"]=area(outline);part["support_overlap_area_m2"]=support(outline,w,h);
  part["perimeter_bearing_length_m"]=distance(L.back(),R.back());
  part["fracture_provenance"]="Original kinked fan with geometric partial branch slits and chipped lip";
  V2 sample={0,0};for(int axis:{0,2}){double total=0;for(const auto&v:part["faces"][0])total+=part["vertices"][v.get<size_t>()][axis].get<double>();sample[axis==0?0:1]=total/3;}
  solid_samples.push_back(sample);parts.push_back(part);
 }
 // The source retries only UnsafeFractureGeometry. Validation errors remain
 // explicit failures; they must never silently select a different pattern.
 for(size_t i=0;i<parts.size();++i){
  const auto first=parts[i]["footprint_xz"].get<Polygon>();
  for(size_t j=i+1;j<parts.size();++j){
   const auto second=parts[j]["footprint_xz"].get<Polygon>();bool crossing=false;
   for(size_t k=0;k<first.size()&&!crossing;++k){const V2 a=first[k],b=first[(k+1)%first.size()];
    for(size_t l=0;l<second.size();++l){const V2 c=second[l],d=second[(l+1)%second.size()];
     if(cross2(a,b,c)*cross2(a,b,d)<-1e-20&&cross2(c,d,a)*cross2(c,d,b)<-1e-20){crossing=true;break;}
    }
   }
   if(crossing||contains(first,second[0],false)||contains(second,first[0],false))throw UnsafeFractureGeometry("Unsupported fracture configuration: fragment overlap");
  }
 }
 Json combined=Json::array();for(const auto&p:scene["parts"])if(p["role"]!="GlassPane")combined.push_back(p);
 for(const auto&p:parts)combined.push_back(p);
 scene["parts"]=std::move(combined);
 const double pane=(bounds[1]-bounds[0])*(bounds[3]-bounds[2]);double retained=0;for(const auto&p:parts)retained+=p["footprint_area_m2"].get<double>();
 scene["metadata"]["retained_glass_fraction"]=retained/pane;
 scene["metadata"]["glass_hole"]={{"center_xz",{cx,cz}},{"polygon_xz",holepoly},{"area_m2",area(holepoly)},
  {"construction","Physical through-hole plus open radial and partial branch slits; all retained shards bear directly in rebate"}};
 scene["metadata"]["glass_fracture"]={{"impact_xz",{cx,cz}},{"energy_proxy",control.energy},{"ray_count",n},{"branch_crack_count",branches},
  {"pane_area_m2",pane},{"source_partition_area_m2",source_area},{"retained_area_m2",retained},{"removed_area_m2",pane-retained},
  {"gap_samples_xz",gap_samples},{"solid_samples_xz",solid_samples},{"nominal_open_fracture_gap_m",nominal_gap},
  {"sharp_unbeveled_glass_edges",true},{"minimum_thickness_m",.004},
  {"support_policy","Every closed fragment intersects the actual 11 mm glazing bead footprint; no loose suspended islands"},{"simulation",false}};
 if(straight_rays)scene["metadata"]["glass_fracture"]["safety_fallback"]="Straight radial boundaries preserve non-overlap for this off-center or narrow configuration; irregular hole and partial branch cracks remain";
 return scene;
}
} // namespace
Json build_fracture(const Json&recipe) {
 try{return build(recipe,false);}catch(const UnsafeFractureGeometry&){return build(recipe,true);}
}
} // namespace jerico
