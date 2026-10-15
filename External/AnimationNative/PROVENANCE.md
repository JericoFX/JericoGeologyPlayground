# Original animation components

Source: private repository `JericoFX/cascadeur-engine-research`, commit
`55982ca6ef330c4a9576f3e10a267f32623ad404`.

This directory preserves the original C++ examples from `examples/cpp`,
`examples/viewport`, `examples/motion_pipeline`, `examples/spd_system` and
`examples/coupled_system`, plus `playground/animation_core.hpp` and the two
original evaluated fixtures. Relative include paths and original license
files are retained. Files were checked byte for byte against that commit.
The upstream repository has no root license declaration; this file does not
assign a new license to files lacking one.

These are the owner's original mathematical examples and authored fixtures.
No Cascadeur code, binaries, neural weights or proprietary sample assets
are included. They do not establish Cascadeur parity. The new Playground
adapter is in `Source/AnimationCore.*`; its standalone validation project is
`Tests/AnimationCore` and requires only the already available JSON header.

The eight-leg fixture has 25 persistent bone IDs and 61 samples over two
seconds. `AnimationCoreTests` compares every world-space sample by ID and
also checks deformation, history, locks, IK and constrained correction.
Release and MSVC AddressSanitizer passed 1808 checks; logs and source hashes
are retained under `Evidence/animation-tools/core-standalone/`.
