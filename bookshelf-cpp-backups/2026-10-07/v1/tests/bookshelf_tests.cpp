#include "jerico/bookshelf.hpp"
#include "jerico/evaluate.hpp"
#include <iostream>
#include <stdexcept>
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){try{
 auto r=jerico::default_bookshelf_recipe(); auto a=jerico::build_bookshelf(r);
 require(a["parts"].size()>90,"bookshelf requires frame and separated book solids");
 require(a["metadata"]["compartments"].size()==4,"reference has four compartments");
 require(a["metadata"]["items"].size()>=24,"reference needs populated varied groups");
 require(a==jerico::build_bookshelf(r),"same recipe must be identical");
 require(jerico::validate_bookshelf(a).empty(),"support/layout validation failed");
 auto e=jerico::evaluate_scene(a);require(jerico::validate_scene(e).empty(),"evaluated meshes invalid");
 for(auto& item:a["metadata"]["items"]){require(item.contains("obb")&&item.contains("support_id"),"item lacks bounds/support");require(item.contains("grab")&&item["grab"]["enabled"]==true,"item lacks engine grab metadata");}
 for(auto pair: {std::pair<const char*,double>{"width",.3},{"depth",.1},{"height",.5},{"shelf_thickness",.001},{"compartments",20},{"fill",1.5}}){auto bad=r;bad[pair.first]=pair.second;bool threw=false;try{jerico::build_bookshelf(bad);}catch(const std::exception&){threw=true;}require(threw,"unsafe recipe should reject");}
 std::cout<<"PASS bookshelf recipe, deterministic structure and support contracts\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
