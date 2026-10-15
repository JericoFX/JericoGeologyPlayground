#pragma once
// Original quaternion composition. No native library or CASC binary decoding.
#include "animation_suite_types.hpp"

namespace JE::AnimationTools::Examples {
// Active qX*qY*qZ; column vectors experience Z, then Y, then X. Radians.
inline Math::Quat ComposeEulerXyz(Math::Vec3 radians) {
    Math::Finite(radians);
    return Math::Multiply(
        Math::Multiply(Math::AxisAngle({1,0,0}, radians.x),
                       Math::AxisAngle({0,1,0}, radians.y)),
        Math::AxisAngle({0,0,1}, radians.z));
}
} // namespace JE::AnimationTools::Examples
