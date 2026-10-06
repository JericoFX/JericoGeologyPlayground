#include "jerico/fine_wear.hpp"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <vector>
namespace jerico {
Json contour_segments(const Json&config){
 using Segment=std::array<V2,2>;std::vector<Segment> segments=config.at("boundary_segments_xz").get<std::vector<Segment>>();
 while(true){
  // Adjacency must be traversed in first-seen order, exactly as Python dict.
  std::map<V2,size_t> lookup;std::vector<std::pair<V2,std::vector<size_t>>> adjacency;
  for(size_t index=0;index<segments.size();++index)for(auto point:segments[index]){auto found=lookup.find(point);if(found==lookup.end()){lookup[point]=adjacency.size();adjacency.push_back({point,{index}});}else adjacency[found->second].second.push_back(index);}
  bool merged=false;
  for(const auto&entry:adjacency){
   const auto point=entry.first;const auto&indices=entry.second;if(indices.size()!=2||indices[0]==indices[1])continue;
   const auto&first=segments[indices[0]];const auto&second=segments[indices[1]];bool has_a=false,has_b=false;V2 a{},b{};
   for(auto v:first)if(v!=point){a=v;has_a=true;break;}
   for(auto v:second)if(v!=point){b=v;has_b=true;break;}
   if(!has_a||!has_b)continue;
   V2 u={a[0]-point[0],a[1]-point[1]},v={b[0]-point[0],b[1]-point[1]};
   if(u[0]*v[1]-u[1]*v[0]==0&&u[0]*v[0]+u[1]*v[1]<0){std::vector<Segment> next;for(size_t i=0;i<segments.size();++i)if(i!=indices[0]&&i!=indices[1])next.push_back(segments[i]);next.push_back({a,b});segments=std::move(next);merged=true;break;}
  }
  if(!merged)break;
 }
 const auto&region=config.at("region");V2 low,high;
 if(region["shape"]=="box"){low={region["min"][0],region["min"][2]};high={region["max"][0],region["max"][2]};}
 else{double radius=region["radius"];low={region["center"][0].get<double>()-radius,region["center"][2].get<double>()-radius};high={region["center"][0].get<double>()+radius,region["center"][2].get<double>()+radius};}
 double width=config.value("width_m",.015);Json result=Json::array();
 for(const auto&segment:segments){bool selected=true;for(size_t axis=0;axis<2;++axis)if(std::max(segment[0][axis],segment[1][axis])<low[axis]-width||std::min(segment[0][axis],segment[1][axis])>high[axis]+width)selected=false;if(selected)result.push_back(segment);}
 return result;
}
Json apply_fine_wear_operations(const Json&scene,const Json&operations){auto result=apply_operations(scene,operations,true);result["metadata"]["native_material_profile"]="finewear_1921";return result;}
Json build_fine_wear(const Json&recipe){
 Json local=recipe;Json operations=local.value("operations",Json::array());
 if(!operations.is_array())throw std::invalid_argument("operations must be a list");
 local.erase("operations");auto scene=build_industrial(local);scene["recipe"]["operations"]=operations;
 return apply_fine_wear_operations(scene,operations);
}
}
