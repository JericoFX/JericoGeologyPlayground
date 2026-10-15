#pragma once
#include "animation_suite_types.hpp"
#include <array>

namespace JE::AnimationTools::Examples {
// Original implementation of the statically observed interval cost formula.
// Here t/T use seconds; curvature has metres/second^2. Native units are unknown.
inline Math::Vec3 BallisticIntervalResidual(Math::Vec3 begin, Math::Vec3 point,
    Math::Vec3 end, double time, double duration, double curvature) {
    Math::Finite(begin); Math::Finite(point); Math::Finite(end);
    Math::Finite(time); Math::Finite(duration); Math::Finite(curvature);
    if (duration <= 0 || time < 0 || time > duration)
        throw std::invalid_argument("invalid ballistic interval");
    const auto value = begin + (end-begin)*(time/duration)
        + Math::Vec3{0,curvature*time*(duration-time)/2,0} - point;
    Math::Finite(value);
    return value;
}
// These are scalar coefficients multiplying 3x3 identity blocks; derived here,
// not recovered native Jacobians. Parameter order is begin/point/end.
inline std::array<double,3> BallisticPositionJacobian(double time, double duration) {
    Math::Finite(time); Math::Finite(duration);
    if (duration <= 0 || time < 0 || time > duration)
        throw std::invalid_argument("invalid ballistic interval");
    const auto u = time/duration;
    return {1-u,-1,u};
}
inline double ExcessDistanceCubic(Math::Vec3 x, Math::Vec3 a,
    Math::Vec3 b, Math::Vec3 c, double threshold) {
    Math::Finite(x); Math::Finite(a); Math::Finite(b); Math::Finite(c);
    Math::Finite(threshold);
    if (threshold < 0) throw std::invalid_argument("negative distance threshold");
    const auto excess = std::max(0.0, Math::Length(x-a+c-b)-threshold);
    const auto result = excess*excess*excess;
    Math::Finite(result);
    return result;
}
} // namespace JE::AnimationTools::Examples
