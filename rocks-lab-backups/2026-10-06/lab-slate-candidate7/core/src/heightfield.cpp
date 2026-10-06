#include "rocks/mesh.hpp"
#include "surface_families.hpp"
#include <chrono>
#include <stdexcept>
namespace rocks {
namespace {
std::uint32_t mix(std::uint32_t x){x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;x^=x>>16;return x;}
double rnd(std::uint32_t seed,unsigned i){return double(mix(seed+0x9e3779b9U*i))/4294967295.;}
double kinkNoise(double t,std::uint32_t seed){int i=int(std::floor(t));double f=t-i;auto sample=[&](int k){return 2*double(mix(seed^mix(std::uint32_t(k))))/4294967295.-1;};return (1-f)*sample(i)+f*sample(i+1);}
double ease(double t){t=std::clamp(t,0.,1.);return t*t*(3-2*t);}
double noise2(double x,double y,std::uint32_t seed){int ix=int(std::floor(x)),iy=int(std::floor(y));double fx=ease(x-ix),fy=ease(y-iy),sum=0;for(int a=0;a<2;a++)for(int b=0;b<2;b++){double v=2*double(mix(seed^mix(std::uint32_t(ix+a)*0x8da6b343U)^mix(std::uint32_t(iy+b)*0xd8163841U)))/4294967295.-1;sum+=v*(a?fx:1-fx)*(b?fy:1-fy);}return sum;}
}
static std::vector<std::uint16_t> cliffSamples(std::uint32_t width,std::uint32_t height,std::uint32_t seed,std::uint32_t bedCount,bool fragmentedContacts,const HeightfieldRecipe* parameters=nullptr,HeightfieldSamples* stats=nullptr){
 const double variation=parameters?parameters->thicknessVariation:1, irregularity=parameters?parameters->irregularity:1, fractures=parameters?parameters->fractures:1,tilt=parameters?std::tan(parameters->tiltDegrees*3.14159265358979323846/180):.055;
 if(width<2||height<2||width>4096||height>4096||bedCount<4||bedCount>24)throw std::invalid_argument("procedural heightfield dimensions outside [2,4096]");
 struct Bed{double low,high,level;std::vector<double> joints,offsets,slopes,laminae,retreats;};std::vector<Bed> beds;
 std::vector<double> boundaries{-.62,-.34,-.13,.065,.30,.62},levels{.56,.54,.55,.53,.57};
 if(bedCount!=5||parameters){boundaries.assign(bedCount+1,-.62);levels.assign(bedCount,.55);std::vector<double> thickness;double total=0;for(unsigned i=0;i<bedCount;i++){double w=std::pow(.55+1.2*rnd(seed,3900+i),1.7);if(parameters)w=1+variation*(w-1);thickness.push_back(w);total+=w;}for(unsigned i=0;i<bedCount;i++)boundaries[i+1]=boundaries[i]+1.24*thickness[i]/total;}
 for(unsigned row=0;row<bedCount;row++){
  Bed b{boundaries[row],boundaries[row+1],levels[row]+irregularity*.07*(rnd(seed,3000+row)-.5),{}, {}, {}, {}, {}};
  unsigned count=3+(row%2);for(unsigned c=1;c<count;c++)b.joints.push_back(-.6+1.2*c/count+irregularity*.09*(rnd(seed,3020+row*7+c)-.5));
  for(unsigned c=0;c<count;c++){b.offsets.push_back(irregularity*.30*(rnd(seed,3100+row*7+c)-.5));b.slopes.push_back(irregularity*.22*(rnd(seed,3160+row*7+c)-.5));}
  double mean=0;for(double offset:b.offsets)mean+=offset;mean/=b.offsets.size();for(auto&offset:b.offsets)offset-=mean;
  unsigned laminations=2+(row%3);for(unsigned k=0;k<laminations;k++){b.retreats.push_back(.085*(rnd(seed,3190+row*7+k)-.5));if(k>0)b.laminae.push_back(b.low+(b.high-b.low)*(double(k)/laminations+.055*(rnd(seed,3230+row*7+k)-.5)));}beds.push_back(b);
 }
 std::vector<std::uint16_t> samples; samples.reserve(std::size_t(width)*height);
 for(unsigned py=0;py<height;py++)for(unsigned px=0;px<width;px++){
  double x=double(px)/(width-1)-.5,y=.5-double(py)/(height-1);
  double by=parameters?y+tilt*x+irregularity*(.025*noise2(x*3+4,y*2-3,seed+3210)+.011*noise2(x*13+1,y*5,seed+3211)+.005*noise2(x*31,y*9,seed+3212)):y+.055*x+.025*noise2(x*3+4,y*2-3,seed+3210)+.011*noise2(x*13+1,y*5,seed+3211)+.005*noise2(x*31,y*9,seed+3212);
  double bx=parameters?x+irregularity*(.018*noise2(x*5,y*7,seed+3220)+.006*noise2(x*23,y*19,seed+3221)):x+.018*noise2(x*5,y*7,seed+3220)+.006*noise2(x*23,y*19,seed+3221);
  auto bedHeight=[&](unsigned row){const auto&b=beds[row];unsigned column=0;std::vector<double> warped;
   for(unsigned j=0;j<b.joints.size();j++){double boundary=b.joints[j]+((fragmentedContacts?.60:.08)*(rnd(seed,3240+row*7+j)-.5))*(by-(b.low+b.high)*.5)+.012*noise2(by*12,row+j*.3,seed+3250+j);if(fragmentedContacts)boundary+=.008*kinkNoise(by*31+row*.7,seed+3790+j);warped.push_back(boundary);if(bx>boundary)column++;}
   double lamina=b.retreats[0];for(unsigned k=0;k<b.laminae.size();k++)lamina+=(b.retreats[k+1]-b.retreats[k])*ease((by-b.laminae[k]+.004)/.008);lamina*=.35+.65*ease(.5+.8*noise2(bx*3+row,row,seed+3260));
   auto face=[&](unsigned c){double center=-.6+1.2*(c+.5)/(b.joints.size()+1);return b.level+lamina+b.offsets[c]+b.slopes[c]*(bx-center)+.06*(by-(b.low+b.high)*.5);};double h=face(column);
   for(unsigned j=0;j<warped.size();j++){double d=bx-warped[j],width=.009+.012*rnd(seed,3270+row*7+j);if(fragmentedContacts){double t=std::clamp((by-b.low)/(b.high-b.low),0.,1.);width*=j%2?.5+.8*t:1.3-.8*t;}if(std::abs(d)<width){double t=std::clamp((d+width)/(2*width),0.,1.);h=(1-t)*face(j)+t*face(j+1);}
    double start=j%2?b.low+.02+.10*rnd(seed,3320+row*7+j):b.low-.025,end=j%2?b.high+.025:b.high-.01-.07*rnd(seed,3370+row*7+j);double termination=ease((by-start)/.025)*ease((end-by)/.025);
    h-=fractures*(fragmentedContacts?.75:1.)*(.09+.19*rnd(seed,3420+row*7+j))*std::max(0.,1-std::abs(d)/(width*(.85+.35*noise2(by*11,row+j,seed+3450))))*termination;
   }return h;};
  unsigned row=0;while(row+1<bedCount&&by>boundaries[row+1])row++;double h=bedHeight(row);
  for(unsigned j=1;j<bedCount;j++){double d=by-boundaries[j],width=.013+.013*rnd(seed,3490+j);if(std::abs(d)<width){double t=std::clamp((d+width)/(2*width),0.,1.);h=(1-t)*bedHeight(j-1)+t*bedHeight(j);}
   double activity=ease((noise2(x*4+j,j,seed+3510+j)+.25)/.65);double depth=.012+.08*activity;h-=depth*std::max(0.,1-std::abs(d)/width);
   if(fragmentedContacts){double cx=-.43+.86*rnd(seed,3810+j),u=bx-cx,halfX=.045+.06*rnd(seed,3830+j),halfY=.018+.026*rnd(seed,3850+j);double broken=std::max(0.,1-std::max({std::abs(u)/halfX,(d+.25*u)/halfY,-d/(halfY*.55)}));h-=fractures*(.05+.065*rnd(seed,3870+j))*broken;}

  }
  // Smaller coherent rock-face variation, substantially below bed offsets.
  double nearest=1;for(unsigned j=1;j<bedCount;j++)nearest=std::min(nearest,std::abs(by-boundaries[j]));double fracturedRim=std::exp(-nearest*nearest/(.035*.035));
  h+=irregularity*((.006+.027*fracturedRim)*noise2(x*29+4,y*27-2,seed+3600)+.004*noise2(x*53,y*49,seed+3601));
  // Short branches begin at existing cross-joints instead of random face pits.
  for(unsigned j=0;j<bedCount;j++){const auto&b=beds[j];double start=(b.low+b.high)*.5,cx=b.joints[j%b.joints.size()];if(fragmentedContacts){unsigned branch=j%b.joints.size();cx+=(.60*(rnd(seed,3240+j*7+branch)-.5))*(by-(b.low+b.high)*.5)+.012*noise2(by*12,j+branch*.3,seed+3250+branch)+.008*kinkNoise(by*31+j*.7,seed+3790+branch);}double length=.045+.065*rnd(seed,3690+j),slope=j%2?.65:-.75,u=by-start,d=bx-cx-slope*std::max(0.,u),w=.004+.003*rnd(seed,3730+j);h-=fractures*.055*std::exp(-d*d/(w*w))*ease(u/.012)*ease((length-u)/.025);}
  if(stats){stats->rawMin=std::min(stats->rawMin,h);stats->rawMax=std::max(stats->rawMax,h);stats->clippedLow+=h<0;stats->clippedHigh+=h>1;}
  samples.push_back(std::uint16_t(std::llround(std::clamp(h,0.,1.)*65535)));
 }
 return samples;
}
std::vector<std::uint16_t> proceduralCliffHeightSamples(std::uint32_t width,std::uint32_t height,std::uint32_t seed,std::uint32_t bedCount,bool fragmentedContacts){return cliffSamples(width,height,seed,bedCount,fragmentedContacts);}
// Independent polyhedral outcrop envelopes and finite joints. No source image is sampled.
// Coordinates are fractions of the footprint; the fixed scalar range is shared with v10.
static HeightfieldSamples jointedSamples(const HeightfieldRecipe&r){
 const bool vertical=r.profile=="vertical_channels";const double angle=r.directionDegrees*3.14159265358979323846/180,cs=std::cos(angle),sn=std::sin(angle);
 struct Cleavage{double x,y,c,s,length,width,depth,active,setback;};
 struct Outcrop{double x,y,rx,ry,level,sx,sy,rotation;unsigned key;std::vector<Cleavage> cuts;};std::vector<Outcrop> masses;
 // Non-lattice placement and overlapping supports avoid a periodic block grid.
 for(unsigned i=0;i<42;i++){double scale=r.blockScale*(.48+1.15*rnd(r.seed,5100+i));masses.push_back({1.7*(rnd(r.seed,5200+i)-.5),1.7*(rnd(r.seed,5300+i)-.5),scale*(vertical?.58:1),scale*(vertical?2.1:.72+ .6*rnd(r.seed,5400+i)),.31+.39*rnd(r.seed,5500+i),r.irregularity*.24*(rnd(r.seed,5600+i)-.5),r.irregularity*.25*(rnd(r.seed,5700+i)-.5),r.irregularity*(rnd(r.seed,5800+i)-.5)*(vertical?.45:1.5),i,{}});
  auto&m=masses.back();for(unsigned k=0;k<9;k++){
   unsigned key=i*11+k;double orientation=(k%3==0?1.45:.12)+.65*(rnd(r.seed,7700+key)-.5);
   m.cuts.push_back({1.5*(rnd(r.seed,8200+key)-.5),1.5*(rnd(r.seed,8700+key)-.5),std::cos(orientation),std::sin(orientation),.3+.65*rnd(r.seed,9200+key),.025+.04*rnd(r.seed,9700+key),.025+.06*rnd(r.seed,10200+key),ease((r.fractures-.85*rnd(r.seed,10700+key))/.15),.035+.035*rnd(r.seed,11200+key)});
  }
 }
 struct Joint{double x,y,angle,length,width,depth,key;};std::vector<Joint> joints;
 double position=-.85;
 for(unsigned i=0;i<32;i++){
  position+=r.jointSpacing*(.55+.9*rnd(r.seed,6200+i));if(position>.85)break;
  double a=vertical?(.08+.24*(rnd(r.seed,6100+i)-.5)):(i%3==0?1.12:-.48)+.65*(rnd(r.seed,6100+i)-.5);
  joints.push_back({position,1.4*(rnd(r.seed,6300+i)-.5),a,(vertical?.5:.17)+ (vertical?.7:.5)*rnd(r.seed,6400+i),(.006+.012*rnd(r.seed,6500+i))*(vertical?1.8:1),.10+.16*rnd(r.seed,6600+i),double(i)});
 }
 HeightfieldSamples result;result.samples.reserve(std::size_t(r.width)*r.height);
 for(unsigned py=0;py<r.height;py++)for(unsigned px=0;px<r.width;px++){
  double x=double(px)/(r.width-1)-.5,y=.5-double(py)/(r.height-1),u=cs*x-sn*y,v=sn*x+cs*y;
  double wx=u+r.irregularity*.025*noise2(u*7+2,v*6-1,r.seed+5900),wy=v+r.irregularity*.018*noise2(u*8,v*7,r.seed+5901);
  double h=.22+.045*r.irregularity*noise2(u*3+3,v*3-1,r.seed+5902);
  for(const auto&m:masses){double dx=wx-m.x,dy=wy-m.y,c=std::cos(m.rotation),s=std::sin(m.rotation),a=(c*dx-s*dy)/m.rx,b=(s*dx+c*dy)/m.ry;
   // Unequal bevel planes with a broad sloping central face, rather than domes.
   double radial=std::max({a+.17*b,-a+.11*b,b-.12*a,-b-.22*a,.66*a+.54*b,-.54*a-.69*b});
   double surface=m.level+m.sx*dx+m.sy*dy-1.12*std::max(0.,radial-.68);
   if(surface<=h)continue; // All following edits only lower this candidate.
   // Hierarchical partial cleavage shelves interrupt selected flanks. Their
   // finite footprint prevents full-width horizontal courses and paving cells.
   for(unsigned k=0;k<(vertical?7U:3U);k++){
    unsigned key=m.key*7+k;double tilt=.6*(rnd(r.seed,7100+key)-.5),level=(vertical?-.8:-.55)+(vertical?1.6:1.1)*rnd(r.seed,7200+key),center=.8*(rnd(r.seed,7300+key)-.5);
    double across=b+tilt*a-level,along=a-center,width=.35+.4*rnd(r.seed,7400+key);
    double step=ease(across/.045)*ease((width-std::abs(along))/.18)*ease((.8-across)/.25);
    surface-=r.irregularity*(.035+.075*rnd(r.seed,7500+key))*step;
   }
   double secondary=0;
   for(const auto&cut:m.cuts){double dx=a-cut.x,dy=b-cut.y,across=cut.c*dx-cut.s*dy,along=cut.s*dx+cut.c*dy;
    double end=ease((cut.length*.5-std::abs(along))/.12);
    secondary=std::max(secondary,cut.active*cut.depth*std::max(0.,1-std::abs(across)/cut.width)*end);
    // A neighboring finite planar setback gives the incision a broken lip;
    // it terminates inside this outcrop rather than traversing unrelated faces.
    surface-=r.irregularity*cut.setback*ease(across/.025)*ease((.22-across)/.12)*end;
   }
   surface-=r.jointDepth*secondary;
   h=std::max(h,surface);
  }
  // Subordinate angular scars occupy finite supports, not a full tessellation.
  double incisions=0;
  for(const auto&j:joints){double dx=u-j.x,dy=v-j.y,c=std::cos(j.angle),s=std::sin(j.angle),along=s*dx+c*dy,across=c*dx-s*dy;
   double path=r.irregularity*(.009*kinkNoise(along*15+j.key,r.seed+6800+unsigned(j.key))+.005*noise2(along*24,j.key,r.seed+6801));
   double edge=std::abs(across-path),width=j.width*(.65+.6*ease(along/j.length+.5));
   double taper=ease((j.length*.5-std::abs(along))/(j.length*.19));
   double cut=std::max(0.,1-edge/width)*taper;
   // Broad but shallower asymmetric channel walls create distinct vertical fluting.
   if(vertical&&across>path)cut=std::max(cut,.40*std::max(0.,1-(across-path)/(width*3.6))*taper);
   double activation=ease((r.fractures-.85*rnd(r.seed,7600+unsigned(j.key)))/.15);
   incisions=std::max(incisions,j.depth*cut*activation);
  }
  h-=r.jointDepth*incisions;
  h+=r.irregularity*(.008*noise2(u*31+4,v*(vertical?13:27)-2,r.seed+6900)+.003*noise2(u*67,v*59,r.seed+6901));
  result.rawMin=std::min(result.rawMin,h);result.rawMax=std::max(result.rawMax,h);result.clippedLow+=h<0;result.clippedHigh+=h>1;result.samples.push_back(std::uint16_t(std::llround(std::clamp(h,0.,1.)*65535)));
 }
 return result;
}

void validateHeightfieldRecipe(const HeightfieldRecipe&r){
 auto within=[](double value,double low,double high){return std::isfinite(value)&&value>=low&&value<=high;};
 if((r.profile!="stratified"&&r.profile!="massive_oblique"&&r.profile!="vertical_channels"&&!isSurfaceFamily(r.profile))||!within(r.directionDegrees,-90,90)||!within(r.jointSpacing,.08,.6)||!within(r.jointDepth,0,1)||!within(r.blockScale,.12,.6))throw std::invalid_argument("invalid cliff profile controls");
 if(r.width<2||r.height<2||r.width>4096||r.height>4096||r.cells<2||r.cells>256||r.bedCount<4||r.bedCount>24||!within(r.size,.1,10)||!within(r.relief,0,.5*r.size)||!within(r.backDepth,.02*r.size,.5*r.size)||!within(r.thicknessVariation,0,1)||!within(r.tiltDegrees,-45,45)||!within(r.fractures,0,1)||!within(r.irregularity,0,1))throw std::invalid_argument("heightfield recipe parameters outside documented finite ranges");
 if(isSurfaceFamily(r.profile))validateSurfaceFamily(r);
}
HeightfieldSamples proceduralHeightfield(const HeightfieldRecipe&r){validateHeightfieldRecipe(r);if(isSurfaceFamily(r.profile)){if(r.profile=="shale_laminated")return surfaceFamilySamples(r);auto host=proceduralHeightfield(surfaceFamilyHostRecipe(r));return surfaceFamilySamples(r,&host);}if(r.profile!="stratified")return jointedSamples(r);HeightfieldSamples result;result.samples=cliffSamples(r.width,r.height,r.seed,r.bedCount,r.fragmentedContacts,&r,&result);return result;}
Extraction generateHeightfield(const HeightfieldRecipe&r){auto samples=proceduralHeightfield(r);return heightfieldSolid(samples.samples,{r.width,r.height,r.cells,r.size,r.relief,r.backDepth,true});}
Extraction heightfieldSolid(const std::vector<std::uint16_t>&samples,const HeightfieldSpec&s){
 if(s.width<2||s.height<2||s.width>4096||s.height>4096||samples.size()!=std::size_t(s.width)*s.height||s.cells<2||s.cells>256||!std::isfinite(s.size)||s.size<.1||s.size>10||!std::isfinite(s.relief)||s.relief<0||s.relief>.5*s.size||!std::isfinite(s.backDepth)||s.backDepth<.02*s.size||s.backDepth>.5*s.size)throw std::invalid_argument("invalid bounded heightfield dimensions, samples or scale");
 auto start=std::chrono::steady_clock::now();Extraction result;auto&m=result.mesh;
 auto mm=std::minmax_element(samples.begin(),samples.end());double minimum=*mm.first,span=double(*mm.second)-minimum;
 auto raw=[&](double x,double y){x=std::clamp(x,0.,double(s.width-1));y=std::clamp(y,0.,double(s.height-1));unsigned ix=unsigned(x),iy=unsigned(y),jx=std::min(ix+1,s.width-1),jy=std::min(iy+1,s.height-1);double fx=x-ix,fy=y-iy;return (1-fy)*((1-fx)*samples[std::size_t(iy)*s.width+ix]+fx*samples[std::size_t(iy)*s.width+jx])+fy*((1-fx)*samples[std::size_t(jy)*s.width+ix]+fx*samples[std::size_t(jy)*s.width+jx]);};
 const unsigned n=s.cells,N=n+1;auto id=[&](unsigned x,unsigned y){return y*N+x;};
 // Exact box-area integration of each intersected bilinear source cell.
 // Sparse point taps can alias high-frequency source maps into false slopes.
 for(unsigned y=0;y<N;y++)for(unsigned x=0;x<N;x++){
  double sx=double(x)*(s.width-1)/n,sy=double(n-y)*(s.height-1)/n;
  double halfX=.5*double(s.width-1)/n,halfY=.5*double(s.height-1)/n;
  double x0=std::max(0.,sx-halfX),x1=std::min(double(s.width-1),sx+halfX),y0=std::max(0.,sy-halfY),y1=std::min(double(s.height-1),sy+halfY),integral=0;
  for(unsigned iy=unsigned(std::floor(y0));double(iy)<y1;iy++)for(unsigned ix=unsigned(std::floor(x0));double(ix)<x1;ix++){
   double a=std::max(x0,double(ix)),b=std::min(x1,double(ix+1)),c=std::max(y0,double(iy)),d=std::min(y1,double(iy+1));
   integral+=(b-a)*(d-c)*.25*(raw(a,c)+raw(b,c)+raw(a,d)+raw(b,d));
  }
  double value=integral/((x1-x0)*(y1-y0));
  double height=s.fixedMapping?value/65535.:(span>0?(value-minimum)/span:.5);
  m.positions.push_back({s.size*(double(x)/n-.5),s.size*(double(y)/n-.5),s.relief*height});
 }
 auto face=[&](unsigned a,unsigned b,unsigned c){m.indices.insert(m.indices.end(),{a,b,c});};
 for(unsigned y=0;y<n;y++)for(unsigned x=0;x<n;x++){unsigned a=id(x,y),b=id(x+1,y),c=id(x,y+1),d=id(x+1,y+1);face(a,b,d);face(a,d,c);}
 const std::size_t frontTriangles=m.indices.size()/3;
 std::vector<unsigned> ring;for(unsigned x=0;x<n;x++)ring.push_back(id(x,0));for(unsigned y=0;y<n;y++)ring.push_back(id(n,y));for(unsigned x=n;x>0;x--)ring.push_back(id(x,n));for(unsigned y=n;y>0;y--)ring.push_back(id(0,y));
 std::vector<unsigned> back;for(auto v:ring){auto p=m.positions[v];p.z=-s.backDepth;back.push_back(unsigned(m.positions.size()));m.positions.push_back(p);}unsigned center=unsigned(m.positions.size());m.positions.push_back({0,0,-s.backDepth});
 for(std::size_t i=0;i<ring.size();i++){auto j=(i+1)%ring.size();face(ring[i],back[i],back[j]);face(ring[i],back[j],ring[j]);face(center,back[j],back[i]);}
 m.normals.assign(m.positions.size(),{});m.resistance.assign(m.positions.size(),.5);std::vector<Vec3> normals;std::vector<std::vector<std::size_t>> incident(m.positions.size());
 for(std::size_t i=0;i<m.indices.size();i+=3){auto a=m.indices[i],b=m.indices[i+1],c=m.indices[i+2];auto normal=cross(m.positions[b]-m.positions[a],m.positions[c]-m.positions[a]);normals.push_back(normal);for(auto v:{a,b,c}){m.normals[v]=m.normals[v]+normal;if(i/3<frontTriangles)incident[v].push_back(i/3);}}
 for(std::size_t i=0;i<normals.size();i++)for(unsigned k=0;k<3;k++){
  Vec3 normal=normalized(normals[i]);if(i<frontTriangles){Vec3 sum{};for(auto j:incident[m.indices[3*i+k]])if(dot(normal,normalized(normals[j]))>.5)sum=sum+normals[j];normal=normalized(sum);}m.cornerNormals.push_back(normal);
 }
 for(auto&normal:m.normals)normal=normalized(normal);
 result.extractNormalsMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();result.coreBufferBytes=(m.positions.capacity()+m.normals.capacity()+m.cornerNormals.capacity())*sizeof(Vec3)+m.indices.capacity()*sizeof(std::uint32_t)+m.resistance.capacity()*sizeof(double);return result;
}
}
