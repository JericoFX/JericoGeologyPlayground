// Artist-directed surface morphology studies. No image input or geological solver.
#include "surface_families.hpp"
#include <stdexcept>
#include <optional>
#include <array>
#include <set>
namespace rocks {
namespace {
constexpr double pi=3.14159265358979323846;
std::uint32_t hash(std::uint32_t x){x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;x^=x>>16;return x;}
double random(std::uint32_t seed,unsigned key){return double(hash(seed+0x9e3779b9U*key))/4294967295.;}
double smooth(double t){t=std::clamp(t,0.,1.);return t*t*(3-2*t);}
double noise(double x,double y,std::uint32_t seed){int ix=int(std::floor(x)),iy=int(std::floor(y));double fx=smooth(x-ix),fy=smooth(y-iy),sum=0;for(int a=0;a<2;a++)for(int b=0;b<2;b++){double v=2*double(hash(seed^hash(std::uint32_t(ix+a)*0x8da6b343U)^hash(std::uint32_t(iy+b)*0xd8163841U)))/4294967295.-1;sum+=v*(a?fx:1-fx)*(b?fy:1-fy);}return sum;}
double bowl(double q){q=std::abs(q);if(q>=1)return 0;double t=1-q*q;return t*t;}
struct LayerSegment {double low,high,setback;unsigned key;};
struct Layer {double low,high,recession,offset,slope;unsigned key;std::vector<LayerSegment> segments;};
struct LayerStudy {
 const HeightfieldRecipe&r;bool slate;std::vector<Layer> layers;
 explicit LayerStudy(const HeightfieldRecipe&recipe):r(recipe),slate(r.profile=="slate_foliated"){
  double low=-r.size*1.4-r.layerSpacingMeters;
  for(unsigned k=0;low<r.size*1.4+r.layerSpacingMeters;k++){
   double thickness=r.layerSpacingMeters*(1+r.thicknessVariation*.72*(random(r.seed,13000+k)-.5));
   layers.push_back({low,low+thickness,.055+(slate?.28:.21)*random(r.seed,14000+k),r.plateLengthMeters*(random(r.seed,15000+k)-.5),.045*(random(r.seed,16000+k)-.5),k,{}});
   double x=-1.7*r.size;unsigned segment=0;while(x<1.7*r.size){unsigned key=hash(k*191U+segment);double span=r.plateLengthMeters*.72*(.55+1.05*random(r.seed,key+411));layers.back().segments.push_back({x,x+span,.19*r.irregularity*(random(r.seed,key+412)-.5),key});x+=span;segment++;}
   low+=thickness;
  }
 }
 double recession(unsigned row,double u,double v)const{
  const auto&b=layers[row];double warped=u+b.offset+r.irregularity*r.layerSpacingMeters*.38*noise(v/r.layerSpacingMeters,u/r.plateLengthMeters,r.seed+17000);
  auto it=std::upper_bound(b.segments.begin(),b.segments.end(),warped,[](double x,const LayerSegment&seg){return x<seg.high;});if(it==b.segments.end())it=std::prev(it);
  const auto&segment=*it;double span=segment.high-segment.low,fraction=(warped-segment.low)/span;unsigned key=segment.key;
  double plane=b.recession+segment.setback+b.slope*(fraction-.5);
  // Quiet sloping plate interiors; shale has a weaker subsidiary split near one lip.
  double across=(v-b.low)/(b.high-b.low);
  if(slate)plane+=.075*std::clamp(across,0.,1.)+.025*(fraction-.5);
  else plane+=.022*smooth((across-.65)/.15)*( .25+.75*random(r.seed,key+22));
  // Finite transverse break. It stops within each packet and never cuts a full grid.
  double distance=std::min(fraction,1-fraction)*span;
  double width=std::max(r.layerSpacingMeters*.18,r.size/r.cells*.65);
  double termination=smooth((across-.06)/.2)*smooth((.94-across)/.2);
  double active=smooth((r.fractures-.8*random(r.seed,key+26))/.2);
  plane+=active*(slate?.15:.09)*bowl(distance/width)*termination;
  // Angular finite flake-edge setbacks linked to bedding/cleavage, not random pits.
  double center=.15+.7*random(r.seed,key+31),along=std::abs(fraction-center);
  double chip=std::max(0.,1-std::max(along/(slate?.16:.24),std::abs(across-.88+.45*(fraction-center))/(slate?.36:.26)));
  plane+=r.fractures*(slate?.12:.15)*chip;
  return std::clamp(plane,.01,.56);
 }
 double operator()(double u,double v)const{
  const double spacing=r.layerSpacingMeters;
  // Small warp is subordinate to the layer family. Slate keeps straighter cleavage.
  double w=v+r.irregularity*spacing*(slate?.06:.52)*noise(u/r.plateLengthMeters*2,v/(spacing*8),r.seed+17100);
  auto it=std::upper_bound(layers.begin(),layers.end(),w,[](double value,const Layer&b){return value<b.high;});
  unsigned row=unsigned(std::min<std::size_t>(it-layers.begin(),layers.size()-1));
  double cut=recession(row,u,w),bevel=spacing*(slate?.12:.20);
  for(int neighbour=int(row)-1;neighbour<=int(row);neighbour++){
   if(neighbour<0||unsigned(neighbour)+1>=layers.size())continue;
   unsigned j=unsigned(neighbour);double d=w-layers[j].high;
   if(std::abs(d)<bevel){double t=smooth((d+bevel)/(2*bevel));cut=(1-t)*recession(j,u,w)+t*recession(j+1,u,w);}
   double persistence=.38+.62*smooth(.5+.8*noise(u/r.plateLengthMeters*2,double(j),r.seed+17200));
   cut+= (slate?.095:.07)*persistence*bowl(d/(spacing*(slate?.15:.18)));
  }
  return std::clamp(cut,0.,.59);
 }
};
struct FaceSite {double x,y,height,slopeU,slopeV,weight;unsigned key;};
struct FaceQuery {double height=0,distance=1e99,nx=1,ny=0,minimum=1e99;unsigned first=0,second=1;std::array<double,160> values{};};
struct FaceJoint {unsigned a,b,key;double nx,ny,offset;};
struct OwnedFaces {
 const HeightfieldRecipe&r;bool foliated;double aspect,band,beta;std::vector<FaceSite> sites;std::vector<FaceJoint> joints;
 OwnedFaces(const HeightfieldRecipe&recipe,bool slate):r(recipe),foliated(slate),aspect(slate?std::clamp(r.plateLengthMeters/r.layerSpacingMeters,1.8,6.):1.15),band(slate?r.layerSpacingMeters*.22:r.channelWidthMeters*.75),beta(2*band*band){
  unsigned count=slate?unsigned(std::ceil(2*r.size*r.size/(r.plateLengthMeters*r.layerSpacingMeters))):13U;count=std::clamp(count,13U,160U);
  for(unsigned k=0;k<count;k++)sites.push_back({r.size*1.45*(random(r.seed,23000+k)-.5),r.size*1.45*(random(r.seed,23100+k)-.5),(slate?.27:.54)+(slate?.42:.18)*random(r.seed,23200+k),(slate?.6:.10)*(random(r.seed,23300+k)-.5),(slate?.6:.10)*(random(r.seed,23400+k)-.5),slate?r.layerSpacingMeters*r.layerSpacingMeters*.6*r.thicknessVariation*(random(r.seed,23450+k)-.5):0.,k});
  std::set<std::pair<unsigned,unsigned>> pairs;
  for(unsigned i=0;i<count;i++){
   std::vector<std::pair<double,unsigned>> nearest;
   for(unsigned j=0;j<count;j++)if(j!=i){double x=(sites[i].x-sites[j].x)/aspect,y=sites[i].y-sites[j].y;nearest.push_back({x*x+y*y,j});}
   std::sort(nearest.begin(),nearest.end());unsigned keep=count<=24?count-1:std::min(8U,count-1);
   for(unsigned k=0;k<keep;k++){unsigned j=nearest[k].second;pairs.insert({std::min(i,j),std::max(i,j)});}
  }
  for(auto[i,j]:pairs){const auto&a=sites[i];const auto&b=sites[j];double gx=(a.x-b.x)/(aspect*aspect),gy=a.y-b.y,norm=std::hypot(gx,gy);if(norm<1e-12)continue;double offset=((b.x*b.x-a.x*a.x)/(aspect*aspect)+b.y*b.y-a.y*a.y-b.weight+a.weight)/(2*norm);joints.push_back({i,j,pairKey(i,j),gx/norm,gy/norm,offset});}
 }
 double distance2(const FaceSite&s,double u,double v)const{double a=(u-s.x)/aspect,b=v-s.y;return a*a+b*b-s.weight;}
 double plane(const FaceSite&s,double u,double v)const{return s.height+s.slopeU*(u-s.x)/(foliated?r.plateLengthMeters:r.size*.35)+s.slopeV*(v-s.y)/(foliated?r.layerSpacingMeters:r.size*.35);}
 FaceQuery query(double u,double v)const{
  FaceQuery q;double second=1e99;
  for(unsigned k=0;k<sites.size();k++){q.values[k]=distance2(sites[k],u,v);if(q.values[k]<q.minimum){second=q.minimum;q.second=q.first;q.minimum=q.values[k];q.first=k;}else if(q.values[k]<second){second=q.values[k];q.second=k;}}
  double total=0;
  // Global soft minimum: subtracting the same minimum cancels algebraically.
  // No selected owner's normal or identity appears in the face blend.
  for(unsigned k=0;k<sites.size();k++){double w=std::exp(-(q.values[k]-q.minimum)/beta);q.height+=w*plane(sites[k],u,v);total+=w;}
  q.height/=total;
  // Owner/nearest-edge identifiers are diagnostics and seed-event anchors only.
  const auto&a=sites[q.first];for(unsigned k=0;k<sites.size();k++){if(k==q.first)continue;const auto&b=sites[k];double gx=2*(a.x-b.x)/(aspect*aspect),gy=2*(a.y-b.y),norm=std::hypot(gx,gy);if(norm<1e-12)continue;double d=std::max(0.,(q.values[k]-q.minimum)/norm);if(d<q.distance){q.distance=d;q.nx=gx/norm;q.ny=gy/norm;q.second=k;}}
  return q;
 }
 double relevance(const FaceJoint&j,const FaceQuery&q)const{return 1-smooth((std::min(q.values[j.a],q.values[j.b])-q.minimum)/(2*beta));}
 unsigned pairKey(unsigned a,unsigned b)const{return hash(std::min(a,b)*547U+std::max(a,b)*37U);}
};
struct SlateStudy {
 const HeightfieldRecipe&r;OwnedFaces faces;
 explicit SlateStudy(const HeightfieldRecipe&recipe):r(recipe),faces(r,true){}
 double incision(double u,double v)const{
  auto q=faces.query(u,v);double incision=0;
  for(const auto&j:faces.joints){double relevance=faces.relevance(j,q);if(relevance==0)continue;double distance=std::abs(j.nx*u+j.ny*v+j.offset);
   double active=smooth((r.fractures-.75*random(r.seed,j.key+23500))/.25);
   double cut=active*(.055+.075*random(r.seed,j.key+23600))*bowl(distance/(r.layerSpacingMeters*.18));
   double along=-j.ny*u+j.nx*v,center=r.size*(random(r.seed,j.key+23700)-.5);double finite=smooth((r.plateLengthMeters*.25-std::abs(along-center))/(r.plateLengthMeters*.12));double offset=r.layerSpacingMeters*(.35+.3*random(r.seed,j.key+23800));
   cut+=r.fractures*.035*bowl((distance-offset)/(r.layerSpacingMeters*.18))*finite*(random(r.seed,j.key+23900)>.65);
   incision=std::max(incision,relevance*cut);
  }
  return incision;
 }
 double operator()(double u,double v)const{auto q=faces.query(u,v);return std::clamp(.75-q.height+incision(u,v),.015,.60);}
};
struct Hollow {double x,y,radius,aspect,angle,exponent,floor,depth,left;};
struct LimestoneStudy {
 const HeightfieldRecipe&r;OwnedFaces faces;std::vector<Hollow> hollows;
 explicit LimestoneStudy(const HeightfieldRecipe&recipe):r(recipe),faces(r,false){
  for(unsigned k=0;k<r.pocketCount;k++){
   double x=r.size*.8*(random(r.seed,24000+k)-.5),y=r.size*.8*(random(r.seed,24100+k)-.5);auto q=faces.query(x,y);
   // Project a seeded event onto a real host joint, then into one shoulder.
   x-=q.nx*q.distance;y-=q.ny*q.distance;double radius=r.pocketRadiusMeters*(.8+.6*random(r.seed,24200+k));
   double side=(k%2?1.:-1.)*radius*.35;x+=q.nx*side;y+=q.ny*side;
   hollows.push_back({x,y,radius,.8+.7*random(r.seed,24300+k),6.28*random(r.seed,24400+k),2.+1.8*random(r.seed,24500+k),.10+.28*random(r.seed,24600+k),.16+.16*random(r.seed,24700+k),.85+.3*random(r.seed,24800+k)});
  }
 }
 double host(double u,double v)const{return std::clamp(faces.query(u,v).height,.40,.79);}
 double operator()(double u,double v)const{
  auto q=faces.query(u,v);double cut=0;
  for(const auto&j:faces.joints){double relevance=faces.relevance(j,q);if(relevance==0)continue;
   const auto&a=faces.sites[j.a];const auto&b=faces.sites[j.b];unsigned key=j.key;
   bool trunk=(a.x<0)!=(b.x<0);double branch=trunk?1.:(random(r.seed,key+25000)>.73?.60:.12);
   double along=-j.ny*u+j.nx*v,signedDistance=j.nx*u+j.ny*v+j.offset;
   double bend=r.irregularity*r.channelWidthMeters*.45*noise(along/(r.channelSpacingMeters*.7),0,r.seed+key+25900);
   double d=std::abs(signedDistance-bend);
   double asymmetric=(signedDistance>bend?.9:1.1);
   double width=r.channelWidthMeters*(.65+.55*random(r.seed,key+25100))*asymmetric*(.85+.15*noise(along/r.channelSpacingMeters,1,r.seed+key+25901));
   double depth=(.28+.16*random(r.seed,key+25200))*(.80+.20*noise(along/(r.channelSpacingMeters*1.7),2,r.seed+key+25902));
   double local=depth*branch*bowl(d/width);
   if(d<r.size*.22&&branch>=.3){
    double phase=r.channelSpacingMeters*random(r.seed,key+25300);int index=int(std::floor((along+phase)/r.channelSpacingMeters));
    // Evaluate neighbors too: the finite supports vanish before a candidate
    // leaves this three-interval window, so floor() cannot create a cut jump.
    for(int k=index-1;k<=index+1;k++){
     unsigned id=hash(key^std::uint32_t(k));if(random(r.seed,id+26000)<.70)continue;
     double center=(k+.2+.6*random(r.seed,id+25400))*r.channelSpacingMeters-phase;
     double shift=r.channelSpacingMeters*.2*noise(d/(r.channelWidthMeters*2),0,r.seed+id+25500);
     double rillWidth=r.channelWidthMeters*(.28+.16*random(r.seed,id+25600));
     double extent=r.size*(.06+.10*random(r.seed,id+25700)),end=smooth((extent-d)/(r.size*.06));
     local=std::max(local,r.fractures*(.06+.08*random(r.seed,id+25800))*bowl((along-center-shift)/rillWidth)*end*branch);
    }
   }
   cut=std::max(cut,relevance*local);
  }
  for(const auto&p:hollows){double dx=u-p.x,dy=v-p.y,c=std::cos(p.angle),s=std::sin(p.angle),a=(c*dx-s*dy)/p.radius,b=(s*dx+c*dy)/(p.radius*p.aspect);if(a<0)a/=p.left;double radial=std::pow(std::pow(std::abs(a),p.exponent)+std::pow(std::abs(b),p.exponent),1/p.exponent);double shape=1-smooth((radial-p.floor)/(1-p.floor));cut=std::max(cut,p.depth*shape);}
  return std::min(cut,.59);
 }

};

}
bool isSurfaceFamily(const std::string&profile){return profile=="shale_laminated"||profile=="slate_foliated"||profile=="limestone_dissolution";}
HeightfieldRecipe surfaceFamilyHostRecipe(const HeightfieldRecipe&r){
 HeightfieldRecipe host=r;const bool slate=r.profile=="slate_foliated";
 host.profile=slate?"vertical_channels":"massive_oblique";
 host.blockScale=std::clamp(slate?r.plateLengthMeters/(r.size*2.1):r.channelSpacingMeters/r.size*1.8,.12,.6);
 host.jointSpacing=std::clamp(slate?r.layerSpacingMeters/r.size*2:r.channelSpacingMeters/r.size,.08,.6);
 host.jointDepth=.15;host.fractures=r.fractures*.25;
 return host;
}
void validateSurfaceFamily(const HeightfieldRecipe&r){
 auto range=[](double x,double lo,double hi){return std::isfinite(x)&&x>=lo&&x<=hi;};
 const double pitch=std::max(r.size/r.cells,r.size/(std::min(r.width,r.height)-1));
 if(r.width-1<4*r.cells||r.height-1<4*r.cells)throw std::invalid_argument("surface source requires at least four sampling intervals per mesh cell");
 if(r.profile=="limestone_dissolution"){
  if(!range(r.channelSpacingMeters,.015,2*r.size)||!range(r.channelWidthMeters,pitch/(.65*.60*.62),.45*r.channelSpacingMeters)||!range(r.pocketRadiusMeters,pitch/.42,.3*r.size)||r.pocketCount>48)throw std::invalid_argument("limestone features invalid or underresolved: minimum realized half-support requires one mesh cell");
 }else{
  double minimumSpacing=std::max(3*pitch/(1-.36*r.thicknessVariation),pitch/(r.profile=="slate_foliated"?.12:.18));
  if(r.profile=="slate_foliated")minimumSpacing=std::max(minimumSpacing,pitch/(.24*.62*(1-.36*r.thicknessVariation)));
  if(!range(r.layerSpacingMeters,minimumSpacing,.4*r.size)||!range(r.plateLengthMeters,2*r.layerSpacingMeters,2*r.size))throw std::invalid_argument("layered features invalid or underresolved: packet and realized rim supports must be resolved");
 }
}
HeightfieldSamples surfaceFamilySamples(const HeightfieldRecipe&r,const HeightfieldSamples* mathematicalHost){
 if(mathematicalHost&&mathematicalHost->samples.size()!=std::size_t(r.width)*r.height)throw std::invalid_argument("mathematical host dimensions mismatch");
 HeightfieldSamples result;result.samples.reserve(std::size_t(r.width)*r.height);
 const double a=r.directionDegrees*pi/180,cs=std::cos(a),sn=std::sin(a);
 const bool limestone=r.profile=="limestone_dissolution";
 // Separate families share only the proven one-valued extraction backend.
 std::optional<LayerStudy> layers;std::optional<SlateStudy> slate;std::optional<LimestoneStudy> channels;
 if(limestone)channels.emplace(r);else if(r.profile=="slate_foliated")slate.emplace(r);else layers.emplace(r);
 for(unsigned py=0;py<r.height;py++)for(unsigned px=0;px<r.width;px++){
  double x=r.size*(double(px)/(r.width-1)-.5),y=r.size*(.5-double(py)/(r.height-1));
  double u=cs*x-sn*y,v=sn*x+cs*y;
  double base=.73+r.irregularity*((limestone?.045:.025)*noise(u/(r.size*(limestone?.3:.7)),v/(r.size*(limestone?.3:.7)),r.seed+20000)+(limestone?.012:.004)*noise(u/(r.size*.075),v/(r.size*.075),r.seed+20001));
  double cut=limestone?(*channels)(u,v):(slate?(mathematicalHost?slate->incision(v,-u):(*slate)(u,v)):(*layers)(u,v));
  if(mathematicalHost){base=mathematicalHost->samples[std::size_t(py)*r.width+px]/65535.;if(limestone)cut*=.60;}
  else if(limestone)base=channels->host(u,v)+r.irregularity*.01*noise(u/(r.size*.075),v/(r.size*.075),r.seed+20001);
  cut=std::min(cut,std::max(0.,base-.10)); // Explicit positive residual surface above the backing.
  double h=base-r.jointDepth*cut;
  result.rawMin=std::min(result.rawMin,h);result.rawMax=std::max(result.rawMax,h);result.clippedLow+=h<0;result.clippedHigh+=h>1;
  result.samples.push_back(std::uint16_t(std::llround(std::clamp(h,0.,1.)*65535)));
 }
 return result;
}
}
