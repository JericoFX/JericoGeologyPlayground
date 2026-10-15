// SPDX-License-Identifier: MIT
// Original deterministic contract checks; no model, native target or scene mutation.
#include "rig_application.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace MP = JE::Research::MotionPipeline;
namespace {
std::size_t checks{}, failures{};
void Require(bool condition) {
    ++checks;
    if (!condition) throw std::runtime_error("contract assertion failed");
}
bool Equal(MP::Vec3 a, MP::Vec3 b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
template<class F> void Reject(F&& operation) {
    bool rejected = false;
    try { operation(); }
    catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected);
}
template<class F> void Case(const char* name, F&& operation) {
    try { operation(); }
    catch (const std::exception& error) {
        ++failures;
        std::cerr << name << ": " << error.what() << '\n';
    }
}
struct Fixture {
    std::vector<MP::RowBinding> bindings{
        {0, "object-zero", "data-c", MP::PositionChannel::GlobalPosition},
        {1, "object-one", "data-a", MP::PositionChannel::LocalPosition},
        {2, "object-two", "data-b", MP::PositionChannel::GlobalPosition}};
    MP::CurrentRigFrame current{7, "scene-v1", "topology-v1", "scene-space/scene-unit/identity",
        {{"data-c", {1, 2, 3}}, {"data-a", {4, 5, 6}}, {"data-b", {7, 8, 9}},
         {"data-unused", {91, 92, 93}}}, {{"data-a", MP::AxisLockMask{0}},
         {"data-b", MP::AxisLockMask{0}}, {"data-c", MP::AxisLockMask{0}}}};
    MP::PositionProposal proposal{std::string(64, 'a'), std::string(64, 'b'), 7,
        "scene-v1", "topology-v1", "scene-space/scene-unit/identity",
        {{11, 12, 13}, {21, 22, 23}, {31, 32, 33}}, {"data-a", "data-b", "data-c"}};
    MP::PositionApplicationPlan Plan() const {
        return MP::PlanPositionApplication(bindings, current, proposal);
    }
};
}

int main() {
    // Catch using binding-vector order instead of explicit row, or losing DataId identity.
    Case("three distinct rows and stable DataId order", [] {
        Fixture f;
        const auto p = f.Plan();
        Require(p.changes.size() == 3);
        Require(p.changes[0].data_id == "data-a" && p.changes[0].row == 1);
        Require(p.changes[0].object_id == "object-one");
        Require(p.changes[0].channel == MP::PositionChannel::LocalPosition);
        Require(Equal(p.changes[0].before, {4, 5, 6}));
        Require(Equal(p.changes[0].after, {21, 22, 23}));
        Require(p.changes[1].data_id == "data-b" && p.changes[1].row == 2);
        Require(Equal(p.changes[1].after, {31, 32, 33}));
        Require(p.changes[2].data_id == "data-c" && p.changes[2].row == 0);
        Require(Equal(p.changes[2].after, {11, 12, 13}));
        Require(p.frame == 7 && p.scene_fingerprint == "scene-v1");
        Require(p.topology_fingerprint == "topology-v1");
        Require(p.coordinate_contract == "scene-space/scene-unit/identity");
        Require(p.model_sha256 == std::string(64, 'a') && p.input_sha256 == std::string(64, 'b'));
        Require(Equal(f.current.values_by_data_id.at("data-unused"), {91, 92, 93}));
        Require(Equal(f.current.values_by_data_id.at("data-a"), {4, 5, 6}));
    });
    Case("permuted binding storage keeps explicit row correspondence", [] {
        Fixture f;
        f.bindings = {f.bindings[2], f.bindings[0], f.bindings[1]};
        const auto p = f.Plan();
        Require(p.changes.size() == 3);
        Require(Equal(p.changes[0].after, {21, 22, 23}));
        Require(Equal(p.changes[1].after, {31, 32, 33}));
        Require(Equal(p.changes[2].after, {11, 12, 13}));
    });
    Case("changed explicit row binding changes destination values", [] {
        Fixture f;
        f.bindings[0].row = 2;
        f.bindings[2].row = 0;
        const auto p = f.Plan();
        Require(p.changes.size() == 3);
        Require(Equal(p.changes[1].after, {11, 12, 13}));
        Require(Equal(p.changes[2].after, {31, 32, 33}));
    });
    Case("selection limits the application plan", [] {
        Fixture f;
        f.proposal.selected_data_ids = {"data-b"};
        const auto p = f.Plan();
        Require(p.changes.size() == 1 && p.changes[0].data_id == "data-b");
        Require(Equal(p.changes[0].before, {7, 8, 9}));
        Require(Equal(p.changes[0].after, {31, 32, 33}));
    });
    // Catch overwriting locked axes or reporting an unchanged locked axis as blocked.
    Case("axis locks preserve current values and report blocked requests", [] {
        Fixture f;
        f.current.axis_locks_by_data_id["data-a"] = MP::AxisLockMask{5};
        f.proposal.values_by_row[1] = {99, 77, 66};
        const auto p = f.Plan();
        Require(p.changes.size() == 3);
        Require(Equal(p.changes[0].after, {4, 77, 6}));
        Require(p.changes[0].axis_lock_mask == 5 && p.changes[0].blocked_axes == 5);
        Require(f.current.axis_locks_by_data_id.at("data-a") == 5);
    });
    Case("equal locked axes are not reported blocked", [] {
        Fixture f;
        f.current.axis_locks_by_data_id["data-a"] = MP::AxisLockMask{5};
        f.proposal.values_by_row[1] = {4, 77, 6};
        const auto p = f.Plan();
        Require(p.changes.size() == 3);
        Require(Equal(p.changes[0].after, {4, 77, 6}) && p.changes[0].blocked_axes == 0);
    });
    Case("fully locked selection retains a report without effective edits", [] {
        Fixture f;
        f.proposal.selected_data_ids = {"data-a"};
        f.current.axis_locks_by_data_id["data-a"] = MP::AxisLockMask{7};
        const auto p = f.Plan();
        Require(p.changes.size() == 1);
        Require(Equal(p.changes[0].before, p.changes[0].after));
        Require(p.changes[0].blocked_axes == 7 && p.changes[0].axis_lock_mask == 7);
    });
    Case("locked signed zero is preserved exactly", [] {
        Fixture f;
        f.current.values_by_data_id["data-a"].x = -0.0;
        f.current.axis_locks_by_data_id["data-a"] = MP::AxisLockMask{1};
        f.proposal.values_by_row[1].x = 0.0;
        const auto p = f.Plan();
        Require(p.changes.size() == 3);
        Require(std::signbit(p.changes[0].after.x));
        Require(p.changes[0].blocked_axes == 1);
    });
    // Reject stale context and malformed destinations before any plan can be returned.
    Case("stale scene revision", [] { Fixture f; f.proposal.scene_fingerprint = "scene-v2"; Reject([&] { f.Plan(); }); });
    Case("stale topology revision", [] { Fixture f; f.proposal.topology_fingerprint = "topology-v2"; Reject([&] { f.Plan(); }); });
    Case("different frame", [] { Fixture f; f.proposal.frame = 8; Reject([&] { f.Plan(); }); });
    Case("negative frame", [] { Fixture f; f.proposal.frame = f.current.frame = -1; Reject([&] { f.Plan(); }); });
    Case("coordinate mismatch", [] { Fixture f; f.proposal.coordinate_contract = "different-space"; Reject([&] { f.Plan(); }); });
    Case("undeclared coordinate contract", [] { Fixture f; f.proposal.coordinate_contract = f.current.coordinate_contract = ""; Reject([&] { f.Plan(); }); });
    Case("empty revision", [] { Fixture f; f.proposal.topology_fingerprint = f.current.topology_fingerprint = ""; Reject([&] { f.Plan(); }); });
    Case("cardinality mismatch", [] { Fixture f; f.proposal.values_by_row.pop_back(); Reject([&] { f.Plan(); }); });
    Case("duplicate row", [] { Fixture f; f.bindings[2].row = 0; Reject([&] { f.Plan(); }); });
    Case("out of range row", [] { Fixture f; f.bindings[2].row = 3; Reject([&] { f.Plan(); }); });
    Case("duplicate DataId", [] { Fixture f; f.bindings[2].data_id = "data-a"; Reject([&] { f.Plan(); }); });
    Case("duplicate ObjectId", [] { Fixture f; f.bindings[2].object_id = "object-one"; Reject([&] { f.Plan(); }); });
    Case("empty identity", [] { Fixture f; f.bindings[0].object_id.clear(); Reject([&] { f.Plan(); }); });
    Case("unknown current DataId", [] { Fixture f; f.current.values_by_data_id.erase("data-b"); Reject([&] { f.Plan(); }); });
    Case("unknown selected DataId", [] { Fixture f; f.proposal.selected_data_ids.insert("data-unused"); Reject([&] { f.Plan(); }); });
    Case("missing lock mask", [] { Fixture f; f.current.axis_locks_by_data_id.erase("data-b"); Reject([&] { f.Plan(); }); });
    Case("invalid lock mask", [] { Fixture f; f.current.axis_locks_by_data_id["data-b"] = MP::AxisLockMask{8}; Reject([&] { f.Plan(); }); });
    Case("unsupported channel", [] { Fixture f; f.bindings[0].channel = static_cast<MP::PositionChannel>(99); Reject([&] { f.Plan(); }); });
    Case("invalid model SHA", [] { Fixture f; f.proposal.model_sha256 = "not-a-sha"; Reject([&] { f.Plan(); }); });
    Case("invalid input SHA", [] { Fixture f; f.proposal.input_sha256 = std::string(64, 'q'); Reject([&] { f.Plan(); }); });
    Case("nonfinite current value", [] { Fixture f; f.current.values_by_data_id["data-b"].z = std::numeric_limits<double>::quiet_NaN(); Reject([&] { f.Plan(); }); });
    Case("nonfinite unselected proposal row", [] { Fixture f; f.proposal.selected_data_ids = {"data-a"}; f.proposal.values_by_row[2].y = std::numeric_limits<double>::infinity(); Reject([&] { f.Plan(); }); });
    Case("empty binding", [] { Fixture f; f.bindings.clear(); f.proposal.values_by_row.clear(); f.proposal.selected_data_ids.clear(); Reject([&] { f.Plan(); }); });
    Case("empty selection still validates all rows", [] { Fixture f; f.proposal.selected_data_ids.clear(); f.bindings[2].data_id = "missing"; Reject([&] { f.Plan(); }); });
    Case("valid empty selection returns no changes", [] { Fixture f; f.proposal.selected_data_ids.clear(); Require(f.Plan().changes.empty()); });
    std::cout << "{\"status\":\"" << (failures == 0 ? "pass" : "fail")
              << "\",\"checks\":" << checks << ",\"failures\":" << failures
              << ",\"nativeTargetStarted\":false}\n";
    return failures == 0 ? 0 : 1;
}
