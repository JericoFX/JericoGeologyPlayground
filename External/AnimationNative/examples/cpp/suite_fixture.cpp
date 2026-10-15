// Original fixed educational fixture producer, not a general scene serializer.
#include "authoring_example.hpp"
#include "evaluation_example.hpp"
#include "skinning_example.hpp"
#include <iomanip>
#include <iostream>
#include <numbers>

namespace Ex = JE::AnimationTools::Examples;
namespace M = JE::AnimationTools::Playground;
void Vector(M::Vec3 value) {
    M::Finite(value);
    std::cout << '[' << value.x << ',' << value.y << ',' << value.z << ']';
}
void Transform(M::RigidTransform value) {
    const auto q = M::Unit(value.rotation);
    std::cout << "{\"position\":"; Vector(value.position);
    std::cout << ",\"rotation_xyzw\":[" << q.x << ',' << q.y << ',' << q.z << ',' << q.w << "]}";
}
int main() {
    try {
        Ex::AuthoringDocument author(Ex::Document{});
        author.AddBone({"root", "", {{1,0,0},{}}});
        author.AddBone({"arm", "root", {{0,2,0},{}}});
        const auto binding = Ex::BuildSkinBinding(author.Get().bones, {"arm", "root"});
        const std::vector<M::Vec3> vertices{{2,2,0}};
        const std::vector<std::vector<M::Influence>> weights{{{0,1}}};
        author.SetKey("arm", {0, {{0,2,0},{}}, {1,0,0}});
        author.SetKey("arm", {2, {{2,2,0},M::AxisAngle({0,1,0},std::numbers::pi)}, {1,0,0}});
        const auto& document = author.Get();
        std::cout << std::setprecision(17);
        std::cout << "{\"schema\":\"own-suite-evaluated-v1\",\"origin\":\"original-cpp\","
            "\"coordinates\":{\"up\":\"Y\",\"handedness\":\"right\",\"length_unit\":\"metre\","
            "\"time_unit\":\"second\",\"quaternion_order\":\"xyzw\",\"scale\":false},\"bones\":[";
        for (std::size_t i=0; i<document.bones.size(); ++i) {
            if (i) std::cout << ',';
            const auto& bone = document.bones[i]; // Fixed IDs only: root/arm.
            std::cout << "{\"id\":\"" << bone.id << "\",\"parent_id\":\"" << bone.parent << "\",\"rest_local\":";
            Transform(bone.local); std::cout << '}';
        }
        std::cout << "],\"tracks\":[{\"bone_id\":\"arm\",\"keys\":[";
        const auto& keys = document.tracks.front().keys;
        for (std::size_t i=0; i<keys.size(); ++i) {
            if (i) std::cout << ',';
            std::cout << "{\"time\":" << keys[i].time << ",\"local\":";
            Transform(keys[i].local);
            std::cout << ",\"position_velocity\":"; Vector(keys[i].position_velocity); std::cout << '}';
        }
        std::cout << "]}],\"mesh\":{\"bind_vertices\":[[2,2,0]],\"joint_ids\":[\"arm\",\"root\"],\"inverse_bind\":[";
        for (std::size_t i=0; i<binding.inverse_bind.size(); ++i) {
            if (i) std::cout << ',';
            Transform(binding.inverse_bind[i]);
        }
        std::cout << "],\"influences\":[[{\"joint\":0,\"weight\":1}]]},\"samples\":[";
        const auto frames = Ex::Bake(document, 0, 2, 0.5);
        for (std::size_t i=0; i<frames.size(); ++i) {
            if (i) std::cout << ',';
            const auto& frame = frames[i];
            const auto posed = Ex::EvaluateLocals(document, frame.time);
            const auto skinned = Ex::SkinMeshPositions(vertices, weights, binding, posed);
            std::cout << "{\"time\":" << frame.time << ",\"evaluated\":true,\"world\":{\"root\":";
            Transform(frame.world.at("root"));
            std::cout << ",\"arm\":"; Transform(frame.world.at("arm"));
            std::cout << "},\"skinned_vertices\":["; Vector(skinned.front()); std::cout << "]}";
        }
        std::cout << "],\"limits\":[\"Original rigid TR/FK/LBS example; no native CASC evaluation, cook, physics or unified playground integration\"]}\n";
        return std::cout.good() ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
