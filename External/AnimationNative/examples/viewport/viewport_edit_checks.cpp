// SPDX-License-Identifier: MIT
// Original pure checks. No UI, renderer, target process or engine is started.
#include <iostream>
#if __has_include("viewport_edit_example.hpp")
#include "viewport_edit_example.hpp"
#include <limits>
#include <stdexcept>

namespace V = JE::Research::ViewportEdit;
int main() {
    try {
        std::size_t checks{};
        const auto require=[&](bool condition,const char* message) {
            ++checks;if(!condition)throw std::runtime_error(message);
        };
        const auto rejects=[&](auto operation,const char* message) {
            bool rejected{};try{operation();}catch(const std::invalid_argument&){rejected=true;}
            require(rejected,message);
        };
        const auto rejects_stale=[&](auto operation,const char* message) {
            bool rejected{};try{operation();}catch(const V::StaleRevision&){rejected=true;}
            require(rejected,message);
        };
        const auto fixture=[] {return V::Document({{1,{1,2,3},false},{2,{4,5,6},true},{3,{-2,0,1},false}});};
        {
            auto document=fixture();const auto initial=document.points();
            V::Drag drag(document,{3,1,2});drag.update_delta({2,3,4});
            require(document.points()==initial && document.revision()==0,"preview must not mutate authoring");
            require(document.undo_count()==0 && document.redo_count()==0,"preview must not add history");
            require(drag.preview().at(0).position==V::Vec3{0,3,5},"preview follows IDs rather than storage order");
            require(drag.preview().at(1).position==V::Vec3{3,5,7},"unlocked point receives absolute start delta");
            require(drag.preview().at(2).position==V::Vec3{4,5,6},"locked point stays at drag start");
            drag.update_delta({5,0,-1});
            require(drag.preview().at(1).position==V::Vec3{6,2,2},"successive deltas must not accumulate");
            require(drag.commit(),"nonempty edit commits");
            require(document.point(1).position==V::Vec3{6,2,2} && document.point(3).position==V::Vec3{3,0,0},
                    "commit publishes selected preview by ID");
            require(document.point(2).position==V::Vec3{4,5,6},"commit preserves locked point");
            require(document.undo_count()==1 && document.revision()==1,"one drag adds exactly one history entry");
            require(!drag.active(),"commit ends drag");
            rejects([&]{(void)drag.commit();},"drag cannot commit twice");
            const auto committed=document.points();
            require(document.undo() && document.points()==initial,"undo restores exact start positions");
            require(document.revision()==2 && document.undo_count()==0 && document.redo_count()==1,"undo advances revision and moves history");
            require(document.redo() && document.points()==committed,"redo restores exact committed positions");
            require(document.revision()==3 && document.undo_count()==1 && document.redo_count()==0,"redo advances revision and moves history");
        }
        {
            auto document=fixture();const auto initial=document.points();
            V::Drag drag(document,{1,3});drag.update_delta({7,8,9});drag.cancel();
            require(document.points()==initial && document.revision()==0 && document.undo_count()==0,"cancel leaves document and history unchanged");
            require(!drag.active(),"cancel ends preview");
            rejects([&]{drag.update_delta({1,0,0});},"cancelled drag rejects further update");
            require(!document.undo() && !document.redo(),"empty history is not an edit");
        }
        {
            auto document=fixture();V::Drag locked(document,{2});locked.update_delta({1,2,3});
            require(!locked.commit() && document.undo_count()==0 && document.revision()==0,"all locked selection commits no change");
            V::Drag zero(document,{1});zero.update_delta({0,0,0});
            require(!zero.commit() && document.undo_count()==0,"zero delta commits no change");
        }
        {
            auto document=fixture();V::Drag stale(document,{1});stale.update_delta({4,0,0});
            V::Drag other(document,{3});other.update_delta({1,0,0});(void)other.commit();
            const auto current=document.points();
            rejects_stale([&]{stale.update_delta({8,0,0});},"changed revision rejects preview update");
            rejects_stale([&]{(void)stale.commit();},"changed revision rejects drag commit");
            require(document.points()==current && document.undo_count()==1 && document.revision()==1,"stale failure is atomic");
            stale.cancel();require(!stale.active(),"stale preview can always be cancelled");
        }
        {
            auto document=fixture();V::Drag first(document,{1});first.update_delta({1,0,0});(void)first.commit();
            require(document.undo(),"undo first edit before divergent branch");
            V::Drag branch(document,{3});branch.update_delta({0,2,0});(void)branch.commit();
            require(document.redo_count()==0 && !document.redo(),"new edit invalidates old redo branch");
        }
        {
            auto document=fixture();V::Drag drag(document,{1,3});drag.update_delta({1,0,0});
            const auto preview=drag.preview();const auto initial=document.points();
            rejects([&]{drag.update_delta({std::numeric_limits<double>::quiet_NaN(),0,0});},"NaN delta rejected");
            rejects([&]{drag.update_delta({0,std::numeric_limits<double>::infinity(),0});},"infinite delta rejected");
            require(drag.preview()==preview && document.points()==initial && document.undo_count()==0,"invalid delta preserves prior preview and document");
        }
        {
            const auto maximum=std::numeric_limits<double>::max();
            V::Document document({{1,{0,0,0},false},{2,{maximum,0,0},false}});
            V::Drag drag(document,{1,2});const auto before=drag.preview();
            rejects([&]{drag.update_delta({maximum,0,0});},"finite delta causing overflow rejected");
            require(drag.preview()==before && document.revision()==0,"overflow after an earlier valid point leaves no partial preview");
        }
        {
            auto document=fixture();
            rejects([&]{V::Drag drag(document,{1,1});},"duplicate selected IDs rejected");
            rejects([&]{V::Drag drag(document,{99});},"unknown selected ID rejected");
            rejects([&]{V::Drag drag(document,{});},"empty selection rejected");
            rejects([&]{V::Document invalid({{1,{0,0,0},false},{1,{1,0,0},false}});},"duplicate document IDs rejected");
            rejects([&]{V::Document invalid({{0,{0,0,0},false}});},"null document ID rejected");
            rejects([&]{V::Document invalid({{1,{0,0,std::numeric_limits<double>::infinity()},false}});},"nonfinite document position rejected");
        }
        std::cout<<"{\"status\":\"pass\",\"checks\":"<<checks<<",\"uiStarted\":false,\"nativeTargetStarted\":false}\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
#else
int main(){std::cerr<<"Original viewport edit core is missing\n";return 1;}
#endif
