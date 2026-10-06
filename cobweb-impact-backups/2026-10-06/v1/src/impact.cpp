#include "impact.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace web::impact {
namespace {
bool finite(V3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
void uvCheck(V2 uv) {
    if (!std::isfinite(uv.x) || !std::isfinite(uv.y) || uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1)
        throw std::invalid_argument("UV must be finite and inside [0,1]^2");
}
uint64_t delayTicks(const Settings& s) {
    // Keep this quotient in the public double time domain: widening .1 before
    // multiplying by 120 would turn an intended 12 ticks into 12 + epsilon.
    return std::max<uint64_t>(2, static_cast<uint64_t>(std::ceil(s.breakDelay / fixedDt)));
}
bool qualifies(const Settings& s, const State& state) {
    return s.breakable && state.impactScore >= s.threshold;
}
double dentAt(const Settings& s, const State& state) {
    const uint64_t delay = delayTicks(s), elapsed = state.tick - state.impactTick;
    if (elapsed >= 2 * delay) return 0;
    const double fraction = double(elapsed <= delay ? elapsed : 2 * delay - elapsed) / double(delay);
    const double strength = s.threshold == 0 ? 1 : std::min(1.0, state.impactScore / s.threshold);
    return (s.maxIndent * strength) * fraction;
}
void checkState(const Settings& s, const State& state, bool replay) {
    uvCheck(state.hitUV);
    if (!std::isfinite(state.impactScore) || state.impactScore < 0 || !std::isfinite(state.dent) ||
        state.dent < 0 || state.dent > s.maxIndent || state.impactTick > state.tick ||
        (state.phase != Phase::Intact && state.phase != Phase::Indenting && state.phase != Phase::Detached) ||
        state.eventCount != (state.phase == Phase::Detached ? 1u : 0u))
        throw std::invalid_argument("invalid panel state");
    if (!replay) return;
    if (!state.hasHit) {
        if (state.phase != Phase::Intact || state.lastHitId != 0 || state.impactTick != 0 ||
            state.impactScore != 0 || state.dent != 0 || state.hitUV.x != .5 || state.hitUV.y != .5)
            throw std::invalid_argument("inconsistent unused panel checkpoint");
        return;
    }
    const uint64_t elapsed = state.tick - state.impactTick, delay = delayTicks(s);
    const bool breaks = qualifies(s, state);
    if (state.phase == Phase::Detached) {
        if (!breaks || elapsed < delay || state.dent != 0)
            throw std::invalid_argument("inconsistent detached checkpoint");
    } else if (state.phase == Phase::Intact) {
        if (breaks || elapsed < 2 * delay || state.dent != 0)
            throw std::invalid_argument("inconsistent recovered checkpoint");
    } else if (elapsed >= (breaks ? delay : 2 * delay) || state.dent != dentAt(s, state)) {
        throw std::invalid_argument("inconsistent indentation checkpoint");
    }
}
uint64_t readUnsigned(std::istream& in) {
    std::string token;
    if (!(in >> token)) throw std::invalid_argument("incomplete checkpoint");
    uint64_t value = 0;
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size())
        throw std::invalid_argument("invalid unsigned checkpoint field");
    return value;
}
V3 unit(V3 v) {
    if (!finite(v)) throw std::invalid_argument("nonfinite deformation direction");
    const double scale = std::max({std::abs(v.x), std::abs(v.y), std::abs(v.z)});
    if (scale == 0) return {};
    v = {v.x / scale, v.y / scale, v.z / scale};
    return v * (1 / std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z));
}
double seedPhase(uint64_t seed) {
    seed += 0x9e3779b97f4a7c15ULL;
    seed = (seed ^ (seed >> 30)) * 0xbf58476d1ce4e5b9ULL;
    seed = (seed ^ (seed >> 27)) * 0x94d049bb133111ebULL;
    seed ^= seed >> 31;
    return double(seed >> 11) * 0x1.0p-53;
}
} // namespace

void validate(const Settings& s) {
    for (double value : {s.threshold, s.maxIndent, s.windAmplitude, s.gustFrequency})
        if (!std::isfinite(value) || value < 0)
            throw std::invalid_argument("threshold, indentation, wind amplitude and frequency must be finite and nonnegative");
    // Twice the quantized delay must fit because nonbreaking recovery takes two ramps.
    const double ticks = std::ceil(s.breakDelay / fixedDt);
    if (!std::isfinite(s.breakDelay) || s.breakDelay <= 0 ||
        !std::isfinite(ticks) || static_cast<long double>(ticks) >
            static_cast<long double>(std::numeric_limits<uint64_t>::max() / 2))
        throw std::invalid_argument("positive finite breakDelay must fit the fixed-step tick range");
    if (!finite(s.windVelocity)) throw std::invalid_argument("wind velocity must be finite");
}

Panel::Panel(Settings settings) : settings_(settings) { validate(settings_); }

bool Panel::receiveImpact(uint64_t hitId, double impactScore, V2 hitUV) {
    uvCheck(hitUV);
    if (!std::isfinite(impactScore) || impactScore < 0)
        throw std::invalid_argument("impact score must be finite and nonnegative artistic units");
    if (state_.phase == Phase::Detached || (state_.hasHit && hitId <= state_.lastHitId) ||
        (state_.phase == Phase::Indenting && qualifies(settings_, state_))) return false;
    state_.hasHit = true;
    state_.lastHitId = hitId;
    state_.impactTick = state_.tick;
    state_.impactScore = impactScore;
    state_.hitUV = hitUV;
    state_.dent = 0;
    state_.phase = Phase::Indenting;
    return true;
}

bool Panel::step() {
    if (state_.tick == std::numeric_limits<uint64_t>::max())
        throw std::overflow_error("panel tick exhausted; reset or restore a prior checkpoint");
    ++state_.tick;
    if (state_.phase != Phase::Indenting) return false;
    const uint64_t elapsed = state_.tick - state_.impactTick, delay = delayTicks(settings_);
    if (qualifies(settings_, state_) && elapsed >= delay) {
        state_.phase = Phase::Detached;
        state_.dent = 0;
        state_.eventCount = 1;
        return true;
    }
    state_.dent = dentAt(settings_, state_);
    if (elapsed >= 2 * delay) state_.phase = Phase::Intact;
    return false;
}

bool Panel::collisionEnabled() const { return state_.phase != Phase::Detached; }
bool Panel::resistanceEnabled() const { return collisionEnabled(); }
void Panel::reset() { state_ = {}; }

std::string Panel::checkpoint() const {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<double>::max_digits10);
    out << "WEB_PANEL_IMPACT 1\n"
        << settings_.breakable << ' ' << settings_.threshold << ' ' << settings_.breakDelay << ' '
        << settings_.maxIndent << ' ' << settings_.windVelocity.x << ' ' << settings_.windVelocity.y << ' '
        << settings_.windVelocity.z << ' ' << settings_.windAmplitude << ' ' << settings_.gustFrequency << ' '
        << settings_.seed << '\n'
        << state_.tick << ' ' << int(state_.phase) << ' ' << state_.hitUV.x << ' ' << state_.hitUV.y << ' '
        << state_.impactScore << ' ' << state_.dent << ' ' << state_.eventCount << ' ' << state_.hasHit << ' '
        << state_.lastHitId << ' ' << state_.impactTick << '\n';
    return out.str();
}

void Panel::restore(const std::string& checkpointText) {
    std::istringstream in(checkpointText);
    in.imbue(std::locale::classic());
    std::string magic;
    int version = 0, breakable = 0, phase = 0, hasHit = 0;
    Settings s;
    State state;
    if (!(in >> magic >> version) || magic != "WEB_PANEL_IMPACT" || version != 1 ||
        !(in >> breakable >> s.threshold >> s.breakDelay >> s.maxIndent >> s.windVelocity.x >>
          s.windVelocity.y >> s.windVelocity.z >> s.windAmplitude >> s.gustFrequency))
        throw std::invalid_argument("invalid panel checkpoint header/settings");
    s.seed = readUnsigned(in);
    state.tick = readUnsigned(in);
    if (!(in >> phase >> state.hitUV.x >> state.hitUV.y >> state.impactScore >> state.dent))
        throw std::invalid_argument("invalid panel checkpoint state");
    const uint64_t eventCount = readUnsigned(in);
    if (!(in >> hasHit)) throw std::invalid_argument("invalid checkpoint hit flag");
    state.lastHitId = readUnsigned(in);
    state.impactTick = readUnsigned(in);
    std::string extra;
    if ((in >> extra) || (breakable != 0 && breakable != 1) || (hasHit != 0 && hasHit != 1) ||
        phase < 0 || phase > 2 || eventCount > 1)
        throw std::invalid_argument("invalid checkpoint flags or trailing data");
    s.breakable = breakable != 0;
    state.hasHit = hasHit != 0;
    state.phase = static_cast<Phase>(phase);
    state.eventCount = static_cast<uint32_t>(eventCount);
    validate(s);
    checkState(s, state, true);
    settings_ = s;
    state_ = state;
}

V3 deformedPoint(const Patch& patch, V2 uv, const State& state, const Settings& s) {
    validate(s);
    checkState(s, state, false);
    uvCheck(uv);
    if (patch.triangular) throw std::invalid_argument("whole-panel impact prototype requires a quad patch");
    for (V3 corner : patch.corners) if (!finite(corner)) throw std::invalid_argument("nonfinite patch corner");
    for (V3 bow : patch.bows) if (!finite(bow)) throw std::invalid_argument("nonfinite patch bow");
    if (!finite(patch.bulge)) throw std::invalid_argument("nonfinite patch bulge");
    V3 point = patch.sample(uv);
    if (!finite(point)) throw std::invalid_argument("patch sample overflow");
    // Avoid even adding zero at anchors/boundaries, retaining the exact old path.
    if (state.phase == Phase::Detached || uv.x == 0 || uv.x == 1 || uv.y == 0 || uv.y == 1) return point;
    const double window = 16 * uv.x * (1-uv.x) * uv.y * (1-uv.y);
    if (state.dent != 0) {
        V3 normal = unit(cross(unit(patch.corners[1]-patch.corners[0]), unit(patch.corners[3]-patch.corners[0])));
        if (normal.x == 0 && normal.y == 0 && normal.z == 0) normal = {0,1,0};
        const V2 distance = uv - state.hitUV;
        const double gaussian = std::exp(-(distance.x*distance.x + distance.y*distance.y) / (2*.18*.18));
        // The reference stone approaches from +normal, traveling along -normal.
        point = point - normal * (state.dent * (window * gaussian));
    }
    if (s.windAmplitude != 0 && (s.windVelocity.x != 0 || s.windVelocity.y != 0 || s.windVelocity.z != 0)) {
        const long double cycles = static_cast<long double>(s.gustFrequency) *
            (static_cast<long double>(state.tick) / 120.0L) + seedPhase(s.seed) + .13L*uv.x + .17L*uv.y;
        const double wave = std::sin(double(std::remainder(cycles, 1.0L)) * 6.2831853071795864769);
        point = point + unit(s.windVelocity) * (s.windAmplitude * (window * wave));
    }
    if (!finite(point)) throw std::invalid_argument("deformation overflow");
    return point;
}
} // namespace web::impact
