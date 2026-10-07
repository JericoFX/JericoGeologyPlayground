#include "jerico/shelf_props.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
namespace jerico {
namespace {
std::pair<V3,V3> bounds(const Json&o){V3 c=o["center"],h=o["half"],extent{};auto a=o["axes"].get<std::array<V3,3>>();for(int k=0;k<3;k++)for(int j=0;j<3;j++)extent[k]+=std::abs(a[j][k])*h[j];return {sub(c,extent),add(c,extent)};}
uint64_t hash(const std::string&id,int64_t seed){uint64_t h=uint64_t(seed)^1469598103934665603ULL;for(char c:id){h^=uint8_t(c);h*=1099511628211ULL;}return h;}
}
Json plan_shelf_prop_placement(const Json&sh,const Json&props){
 if(!sh.contains("metadata")||!sh["metadata"].contains("compartments")||!sh["metadata"].contains("items"))throw std::invalid_argument("Shelf placement requires compartments and items metadata");
 const auto&bays=sh["metadata"]["compartments"];Json occupied=sh["metadata"]["items"],placements=Json::array();int64_t seed=recipe_seed(props["recipe"]["seed"]);
 for(const auto&item:props["metadata"]["items"]){std::string id=item["id"],type=item["type"];auto [lo,hi]=bounds(item["obb"]);V3 size=sub(hi,lo);std::vector<int>rows;int fav=type=="mug"?int(bays.size())-1:type=="lantern"?std::min(1,int(bays.size())-1):0;for(int d=0;d<int(bays.size());d++){int row=(fav+d)%int(bays.size());rows.push_back(row);}Json placement={{"id",id},{"status","no_fit"},{"reason","No supported free gap; existing items remain unchanged"}};
  for(int row:rows){const auto&bay=bays[row];double left=bay["x_min"],right=bay["x_max"],bottom=bay["bottom"],top=bay["top"],ymin=bay["y_min"],ymax=bay["y_max"];if(size[2]>top-bottom-.010||size[1]>ymax-ymin-.016)continue;
   std::vector<V2>blocked;for(const auto&x:occupied)if(x["compartment"]==row){auto [a,b]=bounds(x["obb"]);blocked.push_back({std::max(left,a[0]),std::min(right,b[0])});}std::sort(blocked.begin(),blocked.end());std::vector<V2>gaps;double cursor=left;for(auto block:blocked){if(block[0]>cursor)gaps.push_back({cursor,block[0]});cursor=std::max(cursor,block[1]);}if(cursor<right)gaps.push_back({cursor,right});
   for(auto gap:gaps){double available=gap[1]-gap[0]-size[0]-.012;if(available<0)continue;double f=.35+.30*double(hash(id,seed)%10001)/10000;double xmin=gap[0]+.006+available*f;double ycenter=(ymin+ymax)*.5;V3 shift{xmin-lo[0],ycenter-(lo[1]+hi[1])*.5,bottom-lo[2]};Json obb=item["obb"];obb["center"]=add(obb["center"].get<V3>(),shift);auto [aa,bb]=bounds(obb);double side=std::min(aa[0]-gap[0],gap[1]-bb[0]);placement={{"id",id},{"status","placed"},{"compartment",row},{"support_id",bay["shelf_id"]},{"translation",shift},{"pivot",add(item["pivot"].get<V3>(),shift)},{"obb",obb},{"slot_x",gap},{"clearance_m",{{"side",side},{"front",aa[1]-ymin},{"back",ymax-bb[1]},{"top",top-bb[2]},{"bottom",aa[2]-bottom}}},{"method","deterministic full-height OBB gap; no existing object moved"}};Json occ=item;occ["compartment"]=row;occ["obb"]=obb;occupied.push_back(occ);break;
   }if(placement["status"]=="placed")break;
  }placements.push_back(placement);
 }
 return {{"version",1},{"seed",seed},{"existing_items_unchanged",true},{"minimum_clearance_m",{{"side",.006},{"front_back",.008},{"top",.010}}},{"placements",placements}};
}
}
