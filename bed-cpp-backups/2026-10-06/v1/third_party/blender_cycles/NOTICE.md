# Blender Cycles portions adapted in src/materials.cpp

Version: Blender 4.3.2, official Cycles source distributed with the installed Blender 4.3.2 package.

The native C++ material sampler adapts the original 3D Perlin/fBM equations, Jenkins Lookup3 hash and gradients, and ColorRamp lookup behavior. It retains the source algorithm, float arithmetic, normalization, fractional detail, and 257-entry ColorRamp table semantics. Changes reduce the code to the operations used by Jerico, replace Blender vector types with std::array, and remove Blender runtime dependencies.

## Origins and licenses

- intern/cycles/kernel/svm/noise.h: BSD-3-Clause. Copyright 2009-2010 Sony Pictures Imageworks Inc., et al. All Rights Reserved. Copyright 2011-2022 Blender Foundation. https://github.com/blender/blender/blob/v4.3.2/intern/cycles/kernel/svm/noise.h
- intern/cycles/kernel/svm/fractal_noise.h: Apache-2.0. Copyright 2011-2022 Blender Foundation. https://github.com/blender/blender/blob/v4.3.2/intern/cycles/kernel/svm/fractal_noise.h
- intern/cycles/util/hash.h: Apache-2.0. Copyright 2011-2022 Blender Foundation. https://github.com/blender/blender/blob/v4.3.2/intern/cycles/util/hash.h
- intern/cycles/kernel/svm/ramp.h: Apache-2.0. Copyright 2011-2022 Blender Foundation. https://github.com/blender/blender/blob/v4.3.2/intern/cycles/kernel/svm/ramp.h
- ColorRamp table size was verified against official intern/cycles/blender/util.h, colorramp_to_array: https://github.com/blender/blender/blob/v4.3.2/intern/cycles/blender/util.h#L137-L151

Full license texts are in LICENSE.Apache-2.0 and LICENSE.BSD-3-Clause beside this notice. The Cycles source files were read, not modified. Jerico material-specific masks/graphs are translations of the user's original immutable source snapshots and are not third-party add-on assets.

## SHA-256 of installed source inputs

noise.h 12419f99f4f80963ddd401d3744380e691dab4d9046223c7ab6b680c3cf9bdcc
fractal_noise.h 87ed644943e9d1db5c4a949f851875602150252822aae15409352183b416f875
hash.h b9def855aa97d3a8ab7ce7591592bf1ffd69d292fcdf464b01575fa48e738d12


## Native normal stencil added during parity verification

The forward Bump formula and compact-differential basis also follow these Blender 4.3.2 Cycles files (Apache-2.0, Copyright 2011-2022 Blender Foundation):

- intern/cycles/kernel/svm/displace.h
- intern/cycles/kernel/util/differential.h
- intern/cycles/util/math.h (make_orthonormals)

The basis is transformed world-to-object for rotated nodes. The production footprint derives solely from metric UV texel dimensions. The camera-based original oracle is test-only.
kernel/svm/displace.h d82021e78c61d2317102b4ab88f1f3e8d8aa8372faf7308e83dbc343233cc563
kernel/util/differential.h 1a7e1b1bc15ee3900517e89b6ab0e72f3d70305bea5eab00543fe698cb23bc44
util/math.h f576901f389165d2f759a82c7739b3fc09054848a1c174bf0ae7b28f740a6bc6
