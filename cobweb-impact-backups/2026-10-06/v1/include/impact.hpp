#pragma once
#include "cobweb.hpp"
#include <cstdint>
#include <string>

namespace web::impact {

inline constexpr double fixedDt = 1.0 / 120.0;

enum class Phase { Intact, Indenting, Detached };

struct Settings {
    bool breakable = true;
    // Artistic caller-supplied impact units, never calibrated force or silk strength.
    double threshold = 1.0;
    // Seconds, rounded up to fixed steps; at least two steps retain a visible
    // indentation sample before removal. .08 seconds is ten fixed steps.
    double breakDelay = .08;
    double maxIndent = .16;
    // Analytic displacement direction; amplitude, not velocity, bounds displacement.
    V3 windVelocity{};
    double windAmplitude = 0;
    double gustFrequency = .7;
    uint64_t seed = 42;
};

struct State {
    uint64_t tick = 0;
    Phase phase = Phase::Intact;
    V2 hitUV{.5, .5};
    double impactScore = 0;
    double dent = 0;
    uint32_t eventCount = 0;
    bool hasHit = false;
    uint64_t lastHitId = 0;
    uint64_t impactTick = 0;
};

void validate(const Settings& settings);

class Panel {
public:
    explicit Panel(Settings settings = {});
    // IDs are monotonically increasing nonces. Repeats/older IDs are ignored.
    // Once a break is scheduled, later hits cannot postpone or cancel it.
    // Returns true only if this hit starts/restarts an indentation.
    bool receiveImpact(uint64_t hitId, double impactScore, V2 hitUV);
    // Exactly one fixed step; true only on the permanent detachment event.
    bool step();
    bool collisionEnabled() const;
    bool resistanceEnabled() const;
    const Settings& settings() const { return settings_; }
    const State& state() const { return state_; }
    void reset();
    // Self-contained, versioned, exact-double text. Restore is atomic and also
    // restores settings. Deterministic replay is for the same build/platform.
    std::string checkpoint() const;
    void restore(const std::string& checkpoint);
private:
    Settings settings_;
    State state_;
};

// Rendering helper only. Detached geometry must be omitted by the caller.
// Dent follows -normalized(cross(corner1-corner0, corner3-corner0)), matching
// a stone approaching from the +normal side and traveling along -normal.
// Zero wind takes exactly the same base+dent path. Quad boundary/anchor offsets
// are exactly zero; triangle samples are intentionally outside this prototype.
V3 deformedPoint(const Patch& patch, V2 uv, const State& state,
                 const Settings& settings);

} // namespace web::impact
