# JericoGeologyPlayground — Spline Construction / Pipe UV Lab

**Owner request:** 2026-09-27  
**Status:** IMPLEMENT / VERIFY / LEAVE INTERACTIVE  
**Repository role:** experimental C++/Diligent/ImGui playground. This slice is a proving ground for JericoEngine; it does not modify JericoEngine or declare product integration.

## 0. Direct instruction to the coding agent

Implement and test a reusable **Spline Construction Lab** in this Playground. Do not stop at a design document, compile-only result, or headless unit test.

The owner must be able to launch the normal Playground UI and interactively try the generated constructions.

The task is **not "make a pipe tool"**. Build a small reusable path-construction core and prove several consumers on top of it:

- Pipe
- Cable
- Hose
- Round Duct
- Rectangular Duct
- Cable Tray
- Handrail / tubular rail
- Structural profile run (I/H/C/U/L/T or at least I-beam in the first vertical)

Reuse the same path/sweep/repeat mathematics. Do not create a spline implementation per generator.

Keep the implementation isolated enough that the useful math can later be ported/adapted into JericoEngine's existing Procedural Studio/Spline authority.

## 1. JericoEngine references to inspect before coding

Read the current JericoEngine checkout/reference material and preserve its semantics:

- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/ProceduralSpline.hpp`
- `Tools/ProceduralAuthoring/Source/ProceduralSpline.cpp`
- `docs/JERICO_PROCEDURAL_STUDIO_1_0.md`
- `docs/JERICO_SPLINE_AND_GEOMETRY_MATH_BACKENDS_1_0.md`
- `docs/PROCEDURAL_PWAS_EPLS_MASTER.md`

Important existing Engine facts:

- `ProceduralSpline` already has Polyline, CatmullRom and CubicBezier semantics.
- Control points already carry stable identity, position, Bezier handles, width and roll.
- Existing sampling already produces distance, position, tangent, normal, binormal, width and roll.
- Existing PWAS identity/transactions/cook remain Engine authority.
- Jerico's later math directive selects arc-length evaluation and rotation-minimizing frames (RMF), with TinySpline as a private candidate/selected backend.
- Procedural Studio already describes `Profile + Path + Operations + Pattern + Host + Assembly` and explicitly lists pipes, ducts, cable trays, rails, beams, gutters, fences, bridges and similar path constructions.

Do not modify the Engine from this Playground task.

## 2. Architecture to prove

```text
Editable Path
   |
   +--> Arc-length sampling
   +--> Tangent
   +--> Rotation-Minimizing Frames
   |
   v
Profile2D ----------------------+
                                |
                          Sweep(Profile, Path)
                                |
                    +-----------+-------------+
                    |                         |
             metric UV generation     RepeatAlongPath
                    |                         |
                    +-----------+-------------+
                                |
                          Path Assembly
                                |
                           Playground Mesh
                                |
                         Diligent/Vulkan view
```

A construction preset is data/configuration over this reusable core.

## 3. Path mathematics

Use double precision for authoring/evaluation calculations.

For a path sample, expose conceptually:

```text
s  = world-space arc distance
P  = position
T  = unit tangent
N  = transported profile X axis
B  = transported profile Y axis
roll
```

Distance-driven operations must use arc length, not equal parameter increments.

Required paths for tests:

- straight
- one bend
- S-curve
- near-straight section
- inflection
- vertical tangent
- reversed path
- closed loop
- deliberately non-uniform control-point spacing

## 4. Rotation-minimizing frames are required

Do not use a world-up reconstruction as the final Sweep frame.

Use RMF / parallel transport with the double-reflection method or a mathematically equivalent verified implementation.

Given a sampled frame:

```text
Tj = tangent
Nj = transported normal
Bj = cross(Tj, Nj)
```

Apply authored roll/twist **after** the base RMF.

Required invariants:

- finite
- normalized/orthogonal within tolerance
- no spontaneous 180-degree flips
- deterministic reverse behavior
- explicit closed-loop seam reconciliation
- stable through vertical/near-zero-curvature regions

If TinySpline from the existing Jerico dependency tree can be consumed cleanly without turning it into public identity, it may be used as an oracle/backend experiment. Otherwise implement the local RMF math in the Playground and compare against a known/reference implementation. Record which path was chosen.

## 5. Sweep mathematics

A 2D profile point is `q_i=(u_i,v_i)`.

For each path frame:

```text
V(j,i) = Pj + Nj * u_i + Bj * v_i
```

Transform profile normals through the same frame.

The first profiles must include:

- Circle
- Ring / hollow pipe
- Rectangle
- RoundedRectangle
- at least one structural profile (I-beam preferred)
- open U/channel profile useful for a cable tray

Do not hard-code pipe-only topology into the sweep core.

## 6. Tessellation / quality

Do not define quality only as fixed `PipeSegments = 32`.

For circular/arc profile sampling use geometric error.

For radius `r`, segment angle `delta` and maximum sagitta/chord error `e`:

```text
e = r * (1 - cos(delta/2))
deltaMax = 2 * acos(clamp(1 - e/r, -1, 1))
Nchord = ceil(abs(phi) / deltaMax)
```

Also respect an angular bound and explicit min/max safety budgets.

Path sampling should refine where curve deviation/tangent change requires it, while remaining bounded.

The UI must expose at least Draft / Preview / Production / Hero-like quality choices or an equivalent error/tolerance control so the owner can visibly compare the result.

## 7. UV generation — analytic/metric first

For ordinary swept surfaces, do **not** send the tube body through automatic atlas unwrap as the first solution.

Let:

- `s_j` = world-space distance along path
- `p_i` = accumulated distance around/along the 2D profile perimeter
- `tileU` = metres per texture repetition along the path
- `tileV` = metres per texture repetition across the profile

Generate:

```text
U(j,i) = s_j / tileU
V(j,i) = p_i / tileV
```

For a circular profile, `p_i = r * theta_i`.

Required UV modes to prototype:

1. **WorldMetric** — default; stable texel scale in metres.
2. **Stretch01** — path maps 0..1.
3. **RepeatCount** — N repeats over the whole run.
4. Optional **SegmentAnchored** — explicit restart/anchor policy for segmented/fitting workflows.

### UV seam

Duplicate the profile seam vertex where required. Geometry at 0° and 360° can share position but must be able to carry distinct UV values.

Expose a `seamAngle` / seam rotation control so the owner can move the seam to a less visible side.

### Caps

Simple planar caps use analytic planar UV. Keep atlas unwrap/xatlas-style work for irregular junctions, boolean results or secondary/bake channels; it is not required for a basic straight/swept body.

### Debug visualization

The owner must be able to inspect:

- lit material
- wireframe
- normals/frame vectors
- UV checker/UV debug material
- preferably seam highlighting

## 8. Normals and tangents

Generate deterministic geometric normals for the swept surface.

For this Playground slice, an analytic longitudinal tangent basis is acceptable for direct visual proof, but record the compatibility requirement with the Engine's future deterministic MikkTSpace/mesh-processing route.

Do not claim final normal-map tangent compatibility unless actually tested.

## 9. RepeatAlongPath

Implement a minimal reusable distance-driven repeated-part placement proof.

Required demonstration:

- pipe hangers/support markers
- cable tray crossbars OR rail sleepers

Spacing is measured in world metres by arc length.

Changing upstream control-point spacing without materially changing the geometric path must not turn equal-distance spacing into equal-parameter artifacts.

## 10. Interactive Playground UI — mandatory

Add a visible mode/tab/panel named approximately **Spline Construction**.

The normal executable must let the owner try this without editing source.

### Preset shelf

At minimum provide one-click presets for:

- Pipe 25 mm
- Pipe 80 mm
- Insulated / hollow Pipe
- Cable / Hose
- Round Duct
- Rectangular Duct
- Cable Tray
- Handrail / Rail
- I-Beam Run

It is acceptable for the first commit to implement a subset and mark the rest clearly pending, but final acceptance of this slice needs at least **five visibly different construction presets**, including Pipe, Cable/Hose, Rectangular Duct and Cable Tray/structural profile.

### Path controls

Expose:

- interpolation: Polyline / Catmull-Rom / CubicBezier or closest admitted subset
- open/closed
- Add point
- Insert point
- Delete point
- Reverse
- selected point XYZ
- Bezier in/out handles when applicable
- roll/twist
- optional width/profile scale along path

Viewport point dragging/gizmo is preferred. If it is not practical in the first vertical, numeric point editing is still mandatory and must be pleasant enough to test the geometry.

### Construction controls

Expose as applicable:

- diameter/radius
- wall thickness
- width/height
- corner radius
- profile selection
- cap start/end
- quality/tolerance
- seam angle
- UV mode
- U metres/tile
- V metres/tile
- repeat count
- support/crossbar spacing
- bend/corner policy placeholder with explicit status
- bend radius where implemented
- material/debug checker

### Live feedback

Show:

- path length
- sample count
- frame count
- vertices
- triangles
- UV range/repetitions
- generation CPU time
- warnings/errors

Edits should update Preview without requiring a rebuild of the executable.

## 11. First fitting/corner experiment

After the base Sweep passes, prototype at least one of:

- smooth continuous bend
- radius bend
- 90° elbow insert

Do not hide gaps/self-intersections. If fitting/junction support is incomplete, make that explicit in the UI.

A later network slice can add Tee/Y/Cross/N-way junctions. Do not make the basic Sweep depend on completing a full pipe-network solver.

## 12. Prefab/preset intent

In this Playground, present the constructions as a **preset shelf** so the owner can test the eventual workflow.

The intended Engine UX is:

```text
Asset/Construction Browser
    -> choose "Steel Pipe 80 mm"
    -> drag/create
    -> two-point spline appears
    -> edit path and parameters
    -> same construction recipe remains editable
```

Do not create hundreds of unrelated static meshes just to mimic this. The reusable construction definition/preset should own defaults; generated geometry is the preview/output.

Reusable fittings (elbow, valve, flange, reducer, hanger) may later become true reusable prefab parts.

## 13. Automated tests

Add focused native tests for at least:

### Path/frame
- straight RMF
- S-curve RMF
- inflection RMF
- vertical tangent
- reverse
- closed seam
- finite/orthonormal frame checks
- deterministic repeated run

### Sweep
- circle on straight path
- circle on curved path
- rectangle on curved path
- structural/open profile
- caps/winding
- no degenerate triangles for admitted fixtures

### UV
- seam duplicates carry correct UV discontinuity
- U is proportional to true world arc distance
- V is proportional to profile perimeter
- same metres/tile yields comparable density on straight and curved fixtures
- reverse policy is deterministic
- caps are bounded/finite

### Repeat
- equal-distance support placement on non-uniformly parameterized path
- deterministic counts/positions

### Robustness
- duplicate/degenerate control points fail visibly
- zero/invalid radius fails
- extreme requested quality hits a bounded diagnostic rather than unbounded allocation
- NaN/Inf rejected

## 14. Visual/evidence command

Add a reproducible headless/automation command similar to the existing Playground evidence commands, for example:

```text
JericoGeologyPlayground.exe --spline-construction-test Evidence/spline-construction
```

Exact flag name may differ, but retain:

- JSON metrics
- representative screenshots
- UV checker screenshot
- wireframe/frame screenshot
- test recipe/preset values
- deterministic hashes where meaningful

Capture at least:

1. straight Pipe
2. S-curve Pipe with UV checker
3. vertical/inflection Pipe frame proof
4. Rectangular Duct
5. Cable Tray or I-Beam
6. repeated supports/crossbars
7. one high-curvature/bend case

## 15. Acceptance gates

Do **not** mark this slice complete merely because it compiles.

PASS requires all of:

- Release build succeeds.
- Existing CTest suite remains green.
- New focused spline/sweep/UV tests pass.
- Normal Playground executable opens the Spline Construction UI.
- Owner can create/select at least five presets and change their geometry live.
- Owner can edit spline points and immediately see the generated mesh update.
- Metric UV visibly follows distance and does not obviously stretch through bends.
- RMF tests show no spontaneous profile flips on the mandatory fixtures.
- Evidence command retains screenshots + metrics.
- No JericoEngine file is modified by this Playground slice.
- Documentation distinguishes implemented, automated-verified, visually-inspected and still-pending work.

## 16. Non-goals for the first slice

Do not block this experiment on:

- full pipe fluid simulation
- full Tee/Y/Cross network solver
- Jolt dynamic hose/rope
- production prefab/cook integration in JericoEngine
- final MikkTSpace admission
- xatlas integration
- final editor gizmos
- material authoring system
- Shipping runtime procedural regeneration

Those are later integration slices.

## 17. Handoff back to JericoEngine

When the Playground experiment is successful, produce a concise handoff identifying:

- reusable algorithms
- files/classes created
- measured errors/tolerances
- UI controls that proved useful
- test fixtures
- performance
- visual failures
- which pieces map to existing JericoEngine owners:
  - ProceduralSpline / Path Math
  - Procedural Studio Sweep
  - GenerateUV
  - RepeatAlongPath
  - ConstructionDefinition / Presets
  - Asset Browser
  - Geometry IR / rmesh cook
  - Prefab parts/fittings

The handoff must explicitly avoid creating a second spline authority in JericoEngine.

---

## Owner-facing definition of success

The most important usability check is simple:

> Open JericoGeologyPlayground, choose **Spline Construction**, click a preset such as **Pipe 80 mm**, bend/edit the path, change diameter/quality/UV scale/seam, switch to a duct/cable/tray/beam preset, and immediately see the result in the Vulkan viewport.

If the owner cannot do that, the Playground task is not finished.
