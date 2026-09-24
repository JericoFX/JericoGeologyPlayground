# M8 Auto-Rig Pro 3.78.50 — Technical Research Input

**Milestone:** M8  
**Category:** rigging / retargeting / IK-FK / skinning / animation export preparation  
**Status:** source inspected; research input ready; native Jerico implementation not started  
**Owner timing:** read and act on this only when M8 is the legal `ACTIVE_SLICE`  
**Runtime dependency:** none intended  
**Source archive SHA-256:** `4db1c18ff0ed3979b11d9086d1b538b32ed9e804e09e7c9ef9b234d4a3d0d44f`

## 1. Package inventory

The supplied package contains the main Auto-Rig Pro extension plus associated
bundles/tools. The core extracted package contains approximately:

- 148 files;
- 68 Python files;
- ~113,954 Python lines.

The extension manifest reports version `3.78.50`. The major inspected source
families are:

```text
auto_rig.py          rig generation / binding
auto_rig_smart.py    smart marker / reference setup
auto_rig_remap.py    animation remapping / retargeting
auto_rig_ge.py       game-engine export preparation
rig_functions.py     IK/FK and runtime rig operations
lib/maths_geo.py     geometry/transform/spline math
FBX/GLTF exporters
rig presets / .blend resources
```

The optional AI model/binary package described by the vendor is **not present in
the supplied core archive**, so this document does not claim to inspect its
internals.

## 2. Why this belongs to M8

Auto-Rig Pro does not belong to geology generation, but M8's Blender/headless
asset-preparation backend benefits from understanding how a mature production tool
handles:

- reference-bone rig construction;
- skeleton mapping;
- rest-pose conversion;
- retargeting;
- IK/FK synchronization;
- pole-vector math;
- skin binding and weight transfer;
- export skeleton normalization;
- batch animation baking.

Jerico's final implementation should fit the existing Jerico rig/animation
architecture and cooked asset formats. Blender/Auto-Rig Pro remains an offline
reference, not a runtime dependency.

## 3. Reusable math inventory

The inspected utility layer contains standard reusable operations worth preserving
as independent Jerico math primitives:

- vector projection onto line/plane/vector;
- signed angle;
- matrix blend;
- look-at/up orientation;
- damped tracking;
- transform comparison;
- vector + roll -> bone orientation matrix;
- bone-axis alignment;
- closest points between two 3D lines;
- line/plane intersection;
- Catmull-Rom spline evaluation;
- curve resampling and length;
- NURBS basis/evaluation;
- smooth cosine interpolation;
- pole-angle computation;
- parent/child transform conversion.

Jerico should not mechanically port the implementation. These are standard
mathematical problems and should be implemented/tests-first under Jerico's own
interfaces.

## 4. Bone orientation from vector + roll

A robust skeleton format needs to define the bone local frame unambiguously.

Conceptually:

```text
input:
  direction d
  roll r

1. normalize d
2. construct a stable rotation that maps canonical bone axis -> d
3. choose a fallback axis when nearly parallel to the canonical reference
4. apply roll around d
5. output orthonormal basis / quaternion / matrix
```

Required Jerico tests:
- +Y/-Y and near-parallel directions;
- zero/near-zero length rejection;
- roundtrip matrix -> direction+roll -> matrix;
- handedness;
- deterministic sign behavior.

## 5. Pole vector / pole angle

For a two-bone IK chain:

```text
A = root
B = joint
C = end
```

derive:
- chain axis `C-A`;
- projected joint direction from B onto plane perpendicular to chain;
- desired pole direction projected onto same plane;
- signed angle around chain axis.

This gives a stable pole angle without guessing from Euler components.

Jerico already wants IK grab points and first-person hands; this math should live in
the common rig/IK layer so authoring and runtime agree.

## 6. IK/FK snapping

The inspected tool repeatedly converts transforms through:

```text
bone local
-> armature/object
-> world
-> target parent/rest frame
-> target local
```

The general Jerico rule should be:

**never snap by copying Euler channels. Snap by solving the target local transform
from the desired world/rig-space transform and the current parent/rest basis.**

For a target bone:

```text
M_local =
    inverse(M_parent_world_or_rig)
  * M_desired_world_or_rig
  * inverse/rest_correction_as_defined_by_contract
```

Exact ordering depends on Jerico's skeleton convention and must be locked in tests.

## 7. Retarget architecture

The inspected retarget pipeline uses:

- source/target bone-name mapping;
- rest-pose relationships;
- helper bones/constraints;
- root/location handling;
- IK/pole controls;
- scale compensation;
- animation baking.

It also contains side-aware and fuzzy name matching, with heuristic fallback.

Jerico should separate the concerns:

```text
SkeletonSemanticMap
RestPoseRetargetMap
RetargetScalePolicy
RootMotionPolicy
IKGoalMap
RetargetSolver
AnimationBakeJob
```

### 7.1 Semantic matching

Prefer explicit semantic tags over names:

```text
Hips
Spine[N]
Neck[N]
Head
Clavicle.L/R
UpperArm.L/R
LowerArm.L/R
Hand.L/R
UpperLeg.L/R
LowerLeg.L/R
Foot.L/R
Toe.L/R
Finger chains...
```

Use fuzzy names only as a suggestion system. Never silently publish a fuzzy mapping
without validation.

### 7.2 Scale normalization

The source contains an automatic height-based source/target scale heuristic.
Jerico should not preserve a magic product-specific multiplier.

Instead define:

```text
scale =
    target_reference_measure
  / source_reference_measure
```

where the measure is explicitly chosen:
- character height;
- hip-to-head;
- leg length;
- authored scale marker.

Record the chosen measure in the retarget recipe.

## 8. Rest-pose correction

A retargeted rotation should be expressed relative to each skeleton's bind/rest
basis, conceptually:

```text
M_delta_source =
    inverse(M_source_rest)
  * M_source_animated

M_target_animated =
    M_target_rest
  * C
  * M_delta_source
  * inverse(C)
```

where `C` maps the source bone semantic frame to the target semantic frame.

Exact spaces depend on Jerico's hierarchy convention, but the important rule is
to make rest correction explicit and testable.

## 9. Curve / spline math

The package contains Catmull-Rom and NURBS helpers for rig controls.

Jerico already has/needs a single spline/path authority. Do not create an
animation-only duplicate.

Potential uses:
- spline IK;
- control curves;
- procedural rig guides;
- camera/sequence paths;
- cables/ropes and other M8 curve assets.

The rig layer should consume the shared spline evaluator and add rig semantics on
top.

## 10. Skinning / binding

The inspected package supports Blender automatic/heat weighting and also contains a
proxy-based workflow:

```text
source mesh
  -> duplicate/proxy
  -> reduce/remesh/voxel-like simplification
  -> automatic weights on proxy
  -> transfer weights back to original
  -> clean tiny weights
  -> repair unweighted vertices from neighbors
```

For Jerico, this suggests a general offline `SkinBindJob`, not a dependency on
Blender weighting.

Potential Jerico approaches:
- geodesic/bone-distance weighting;
- heat diffusion;
- voxel/volume diffusion;
- proxy solve + barycentric transfer.

### 10.1 Weight constraints

Every publish path should enforce:
- max influences per vertex;
- non-negative weights;
- normalized sum;
- deterministic bone order;
- minimum retained weight;
- no unweighted deforming vertices unless explicitly allowed.

### 10.2 Transfer

When topology differs, transfer should use a geometric map:
- closest surface + barycentric weights;
- cage correspondence;
- known simplification lineage when available.

Avoid nearest-vertex transfer except as an explicit low-quality fallback.

## 11. Export normalization

The official tool supports specialized game-engine export behavior. The transferable
lesson is not “make Jerico export like Unreal”; Jerico should define one canonical
cooked skeleton and animation representation, then import/export adapters around it.

M8 should validate Blender-worker output into Jerico contracts before publication:

```text
external rig/animation
  -> canonical skeleton semantic map
  -> coordinate/unit normalization
  -> bind/rest validation
  -> influence cleanup
  -> animation bake
  -> Jerico cooker
  -> .rskel / .rrig / .ranim
```

No FBX-specific convention should leak into runtime.

## 12. Batch animation baking

A retarget/export job should be designed as a cancellable offline pipeline:

```text
validate
-> map
-> solve frame range
-> resample
-> root-motion extraction
-> curve reduction
-> event preservation
-> validation
-> cook
```

Measure:
- angular error;
- positional error;
- end-effector drift;
- root error;
- key count;
- processing time.

## 13. Suggested M8 components

```text
RigSemanticMapper
RestPoseRetargetSolver
IKFKPoseConverter
PoleVectorSolver
SkinBindJob
SkinWeightTransfer
SkeletonValidation
AnimationBakeJob
BlenderRigAdapter
```

These are asset-preparation capabilities. Runtime animation remains under the
existing Animation/Rigging authority.

## 14. Tests to require

### Retarget
- identical skeleton roundtrip;
- same semantics with different rest axes;
- different limb proportions;
- mirrored sides;
- root-motion walk;
- crouch;
- arm reach;
- 180-degree rotations;
- quaternion sign continuity.

### IK/FK
- FK -> IK -> FK pose error;
- singular straight limb;
- pole crossing;
- near-zero plane normal;
- hand/foot pinning.

### Skin
- capsule limb;
- shoulder joint;
- fingers;
- disconnected accessory;
- high/low proxy transfer;
- max-influence clamp and normalization.

### Export/cook
- coordinate-system roundtrip;
- deterministic byte output;
- no hidden Blender object state;
- source-free package replay.

## 15. License / provenance boundary

Official Auto-Rig Pro documentation describes mixed licensing:
- Blender/Python source components in `src`: GPLv3;
- AI binaries: restrictive copyrighted license;
- assets/presets: separate Royalty-Free/CC0 terms depending on asset.

Therefore:

- do not copy the inspected GPL source into Jerico production;
- do not redistribute or depend on the optional AI binaries;
- do not use bundled icons/preset assets without separately verifying their license;
- generated rigs belong to the end user according to the vendor documentation;
- standard transform/IK/retarget mathematics can be independently implemented from
  first principles.

Official references:
- https://www.lucky3d.fr/auto-rig-pro/doc/
- https://www.lucky3d.fr/auto-rig-pro/doc/remap_doc.html
- https://www.lucky3d.fr/auto-rig-pro/doc/ge_export_doc.html
- https://www.lucky3d.fr/auto-rig-pro/doc/license.html

## 16. M8 decision

**KEEP AS M8 ASSET-PIPELINE RESEARCH INPUT.**

The strongest reusable ideas are not the UI or preset rig. They are:

1. explicit semantic skeleton mapping;
2. rest-space-aware retargeting;
3. correct IK/FK and pole-space math;
4. robust skin binding + transfer;
5. normalized, validated game/cook export.

Jerico should implement those as native/open asset-pipeline capabilities so the
future open-source engine does not require Auto-Rig Pro.
