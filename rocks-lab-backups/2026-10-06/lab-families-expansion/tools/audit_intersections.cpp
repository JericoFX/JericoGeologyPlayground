#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
struct V { double x,y,z; V operator+(V b)const{return {x+b.x,y+b.y,z+b.z};} V operator-(V b)const{return {x-b.x,y-b.y,z-b.z};} V operator*(double s)const{return {x*s,y*s,z*s};} };
double dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;} V cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};} double norm(V a){return sqrt(dot(a,a));}
struct T {array<V,3> p; array<int,3> ids; V lo,hi;};
struct P {double x,y; P operator+(P b)const{return{x+b.x,y+b.y};} P operator-(P b)const{return{x-b.x,y-b.y};} P operator*(double s)const{return{x*s,y*s};}}; double cr(P a,P b){return a.x*b.y-a.y*b.x;}
// Intersect the actual plane, not its epsilon-thick slab. Snapping a nearby
// vertex onto the plane can expand a near-parallel slice by an entire edge.
// Spatial tolerances remain in coplanarity, interval and adjacency checks.
vector<V> cut(const T&t,V n,V origin){vector<V> r; double d[3]; for(int i=0;i<3;i++){d[i]=dot(t.p[i]-origin,n);if(d[i]==0)r.push_back(t.p[i]);} for(int i=0;i<3;i++){int j=(i+1)%3;if((d[i]>0&&d[j]<0)||(d[i]<0&&d[j]>0))r.push_back(t.p[i]+(t.p[j]-t.p[i])*(d[i]/(d[i]-d[j])));}return r;}
// 0 clear/legitimate shared boundary, 1 noncoplanar intersection, 2 positive-area coplanar overlap.
int intersect(const T&a,const T&b,double eps){
 V na=cross(a.p[1]-a.p[0],a.p[2]-a.p[0]), nb=cross(b.p[1]-b.p[0],b.p[2]-b.p[0]);double la=norm(na),lb=norm(nb);if(la==0||lb==0)return 0;na=na*(1/la);nb=nb*(1/lb);
 double da[3],db[3];for(int i=0;i<3;i++){da[i]=dot(a.p[i]-b.p[0],nb);db[i]=dot(b.p[i]-a.p[0],na);} auto sep=[&](double*d){return (*min_element(d,d+3)>eps)||(*max_element(d,d+3)<-eps);};if(sep(da)||sep(db))return 0;
 V line=cross(na,nb);double ll=norm(line);
 bool spatiallyCoplanar=true;for(int i=0;i<3;i++)if(abs(da[i])>eps||abs(db[i])>eps)spatiallyCoplanar=false;
 if(ll<1e-10||spatiallyCoplanar){if(!spatiallyCoplanar)return 0;int axis=0;if(abs(na.y)>abs(na.x))axis=1;if(abs(na.z)>(axis==0?abs(na.x):abs(na.y)))axis=2;auto project=[&](V p)->P{if(axis==0)return{p.y,p.z};if(axis==1)return{p.x,p.z};return{p.x,p.y};};vector<P> poly;array<P,3> clip;for(int i=0;i<3;i++){poly.push_back(project(a.p[i]));clip[i]=project(b.p[i]);}double sign=cr(clip[1]-clip[0],clip[2]-clip[0])>=0?1:-1;
 for(int i=0;i<3&&!poly.empty();i++){P c=clip[i],d=clip[(i+1)%3];vector<P> q;for(size_t j=0;j<poly.size();j++){P s=poly[j],e=poly[(j+1)%poly.size()];double ds=sign*cr(d-c,s-c),de=sign*cr(d-c,e-c);bool ins=ds>=0,ine=de>=0;if(ins)q.push_back(s);if(ins!=ine)q.push_back(s+(e-s)*(ds/(ds-de)));}poly=q;}double area=0;for(size_t i=1;i+1<poly.size();i++)area+=cr(poly[i]-poly[0],poly[i+1]-poly[0]);double edge=0;for(int i=0;i<3;i++){edge=max(edge,norm(a.p[(i+1)%3]-a.p[i]));edge=max(edge,norm(b.p[(i+1)%3]-b.p[i]));}return abs(area)>eps*edge*4?2:0; }
 line=line*(1/ll);auto ca=cut(a,nb,b.p[0]),cb=cut(b,na,a.p[0]);if(ca.empty()||cb.empty())return 0;auto range=[&](vector<V>&v){double lo=1e300,hi=-1e300;for(V p:v){double x=dot(p-a.p[0],line);lo=min(lo,x);hi=max(hi,x);}return pair<double,double>{lo,hi};};auto ra=range(ca),rb=range(cb);double lo=max(ra.first,rb.first),hi=min(ra.second,rb.second);if(lo>hi+eps)return 0;
 vector<double> shared;for(int i=0;i<3;i++)for(int j=0;j<3;j++)if(a.ids[i]==b.ids[j])shared.push_back(dot(a.p[i]-a.p[0],line));if(!shared.empty()){auto mm=minmax_element(shared.begin(),shared.end());if(lo>=*mm.first-eps*4 && hi<=*mm.second+eps*4)return 0;}return 1;
}
T tri(array<V,3> p,array<int,3> ids){T t{p,ids,p[0],p[0]};for(V v:p){t.lo={min(t.lo.x,v.x),min(t.lo.y,v.y),min(t.lo.z,v.z)};t.hi={max(t.hi.x,v.x),max(t.hi.y,v.y),max(t.hi.z,v.z)};}return t;}
int main(int argc,char**argv){T a=tri({V{0,0,0},V{2,0,0},V{0,2,0}},{0,1,2});struct F{string name;T b;int expected;};vector<F> f={
 {"crossing",tri({V{.5,.5,-1},V{.5,.5,1},V{1.5,.5,0}},{3,4,5}),1},
 {"separated",tri({V{3,3,-1},V{3,3,1},V{4,3,0}},{3,4,5}),0},
 {"parallel_separated",tri({V{0,0,1},V{2,0,1},V{0,2,1}},{3,4,5}),0},
 {"coplanar_overlap",tri({V{.2,.2,0},V{1,.2,0},V{.2,1,0}},{3,4,5}),2},
 {"coplanar_crossing_overlap",tri({V{1,-1,0},V{3,1,0},V{-1,1,0}},{3,4,5}),2},
 {"coplanar_shared_edge_overlap",tri({V{0,0,0},V{2,0,0},V{1,.5,0}},{0,1,5}),2},
 {"coplanar_disjoint",tri({V{3,3,0},V{4,3,0},V{3,4,0}},{3,4,5}),0},
 {"shared_edge",tri({V{0,0,0},V{2,0,0},V{0,0,1}},{0,1,5}),0},
 {"coplanar_shared_edge",tri({V{0,0,0},V{2,0,0},V{0,-2,0}},{0,1,5}),0},
 {"shared_vertex_only",tri({V{0,0,0},V{-1,0,1},V{0,-1,1}},{0,4,5}),0},
 {"shared_vertex_crossing",tri({V{0,0,0},V{1,1,1},V{1,1,-1}},{0,4,5}),1}};
 for(auto &x:f){int r=intersect(a,x.b,1e-9);cout<<"fixture "<<x.name<<" result="<<r<<" expected="<<x.expected<<"\n";if(r!=x.expected)return 2;}
 // Regression: adjacent triangles in a tiny exported planar arrangement cell.
 // Their unit-normal angular error exceeds 1e-10 although plane separation
 // is below 5e-16. Spatial coplanarity must control this classification.
 T tinyA=tri({V{.4489682472522732,-1.0671415343508033,-.4093732584947945},V{.44896966765843466,-1.067141294929055,-.4093616114599747},V{.4489651646726691,-1.06714073754189,-.40937934456307157}},{1,2,3});
 T tinyB=tri({V{.4489682472522732,-1.0671415343508033,-.4093732584947945},V{.4489699094257158,-1.0671425705814652,-.4093788194613373},V{.44896966765843466,-1.067141294929055,-.4093616114599747}},{1,4,2});
 int tinyResult=intersect(tinyA,tinyB,3.53358e-9);cout<<"fixture tiny_adjacent_planar result="<<tinyResult<<" expected=0\n";if(tinyResult!=0)return 2;
 // Spatial coplanarity must still detect positive-area overlaps, including
 // adjacent triangles that cross onto the same side of their shared edge.
 for(double scale:{1e-5,1.,1e5}){
  double eps=1e-9*scale;
  T plane=tri({V{0,0,0},V{2*scale,0,0},V{0,2*scale,0}},{0,1,2});
  T crossing=tri({V{.2*scale,.2*scale,-eps*.1},V{scale,.2*scale,eps*.1},V{.2*scale,scale,0}},{3,4,5});
  T shared=tri({V{0,0,0},V{2*scale,0,0},V{scale,.5*scale,eps*.1}},{0,1,5});
  for(auto fixture:{pair<const char*,T>{"near_planar_crossing_overlap",crossing},{"near_planar_shared_edge_overlap",shared}}){
   int r=intersect(plane,fixture.second,eps);cout<<"fixture "<<fixture.first<<" scale="<<scale<<" result="<<r<<" expected=2\n";if(r!=2)return 2;
  }
 }
 bool meshFailure=false;
 for(int arg=1;arg<argc;arg++){ifstream in(argv[arg]);vector<V> vs;vector<T> ts;string s;int polygons=0;while(getline(in,s)){istringstream ss(s);string k;ss>>k;if(k=="v"){V v;ss>>v.x>>v.y>>v.z;vs.push_back(v);}else if(k=="f"){vector<int> ids;string x;while(ss>>x){int id=stoi(x);ids.push_back(id>0?id-1:vs.size()+id);}polygons++;for(size_t j=1;j+1<ids.size();j++){array<int,3> ix{ids[0],ids[j],ids[j+1]};ts.push_back(tri({vs[ix[0]],vs[ix[1]],vs[ix[2]]},ix));}}}V lo=vs[0],hi=vs[0];for(V v:vs){lo={min(lo.x,v.x),min(lo.y,v.y),min(lo.z,v.z)};hi={max(hi.x,v.x),max(hi.y,v.y),max(hi.z,v.z)};}double eps=norm(hi-lo)*1e-9;long long tested=0,hit=0,cop=0,deg=0;vector<int> order;for(int i=0;i<(int)ts.size();i++){order.push_back(i);if(norm(cross(ts[i].p[1]-ts[i].p[0],ts[i].p[2]-ts[i].p[0]))<=eps*eps)deg++;}sort(order.begin(),order.end(),[&](int i,int j){return ts[i].lo.x<ts[j].lo.x;});cout<<"MESH "<<argv[arg]<<" vertices="<<vs.size()<<" faces="<<polygons<<" triangles="<<ts.size()<<" eps="<<eps<<" degenerate="<<deg<<"\n";
 for(size_t i=0;i<order.size();i++)for(size_t j=i+1;j<order.size();j++){auto&a=ts[order[i]];auto&b=ts[order[j]];if(b.lo.x>a.hi.x+eps)break;if(a.lo.y>b.hi.y+eps||b.lo.y>a.hi.y+eps||a.lo.z>b.hi.z+eps||b.lo.z>a.hi.z+eps)continue;tested++;int r=intersect(a,b,eps);if(r){hit++;cop+=(r==2);if(hit<=20)cout<<"intersection triangle_1based "<<order[i]+1<<" "<<order[j]+1<<" kind="<<(r==2?"coplanar_area":"noncoplanar")<<"\n";}}
 cout<<"RESULT candidates="<<tested<<" intersections="<<hit<<" coplanar="<<cop<<"\n";
  meshFailure=meshFailure||hit>0||deg>0;
 }
 return meshFailure?2:0;
}
