# M8 P1 FoldDomain research mapping

## Scope and provenance

This note supports only `M8-GEOLOGY-BLUEPRINT-P1-FOLD-DOMAIN`, the third P1
item in blueprint section 88 and the localized domain transform described in
section 66.

The implementation was designed from the local blueprint authority and the
published Jerico `DirectionalWarp`, `Strata`, `FaultDisplace`, `PlaneCut`,
`ImplicitBlock` and `JointSet` contracts. No external implementation or
reference-engine source was used as a code donor. No source, shader, data
structure, constant, preset or API was copied, translated or ported.

## Existing ownership audit

The repository already owned each adjacent responsibility, but not the
requested localized coherent fold:

- `DirectionalWarp` offsets all three source-domain coordinates with seeded
  3D fractal noise. It owns organic stochastic deformation, not an authored
  periodic fold axis and displacement direction.
- `Strata` evaluates a periodic scalar/profile field for geological layering.
  It does not transform the source sampling domain.
- `FaultDisplace` owns one-sided displacement across a plane. It creates slip,
  not a continuous sinusoidal bend.
- `PlaneCut`, `ImplicitBlock` and `JointSet` own scalar CSG boundaries,
  structural blocks and fracture-plane slabs. None owns source-domain folding.
- the resident GPU session already owns canonical recipe identity, regional
  masks, scalar ping-pong resources, bounded inspection, dirty tracking,
  cancellation, workload/device guards and Surface Nets extraction.

Deepening any one of those contracts with FoldDomain semantics would mix
distinct mathematical ownership. A new typed recipe kind is therefore the
smallest coherent extension; it reuses every existing evaluation and mesh
path rather than creating a parallel geology system.

## Independent Jerico mapping

One canonical `FoldDomain` layer carries:

- a local unit fold axis;
- a local unit displacement direction;
- positive amplitude in world metres;
- positive frequency in cycles per world metre;
- phase in cycles in `[0, 1)`;
- positive local envelope half-extents in metres;
- a normalized envelope falloff fraction in `(0, 1]`;
- the existing translation, unit quaternion and positive non-uniform scale;
- zero seed, because this operator is fully authored and deterministic.

The fold axis is transformed as a covector by inverse scale followed by
rotation and normalization. The displacement direction is transformed as a
vector by forward scale followed by rotation and normalization. Envelope
half-extents receive the physical non-uniform scale, while amplitude and
frequency deliberately remain world-metre and cycles-per-world-metre values.
An oblique-axis CPU witness freezes these semantics.

For world point `p`, translation-relative fold coordinate `u`, envelope weight
`e`, composed strength/mask `w`, amplitude `A`, frequency `f` and phase `phi`,
the source is sampled at:

```text
q = p + displacementDirectionWorld
        * A * sin(2*pi*(f*u + phi)) * e * w
```

The envelope is the product of three cubic smoothstep edge weights in rotated
world space. It has exact finite support: outside the envelope, with zero
strength, or with zero regional mask, `q == p` exactly. The GPU receives the
resolved wave vector, direction, translation, inverse rotation and scaled
envelope through the existing five `float4` layer constants. Canonical
validation rejects values whose derived packed representation could overflow,
underflow or collapse on the GPU.

The productive shader samples the existing resident source field at `q` with
the established trilinear sampler. It introduces no secondary field, mesher,
noise source, preset or proof-only geometry.

## Explicit boundary

This mapping does not admit gradient-based implicit detail warp,
`DirectedCrack`, durability, rockfall, colluvium, imported mesh structural
operators, later P1/P2/M9/Task23, OpenVDB, editor UI or named cliff/rock
presets. It also does not implement the geological material integration; that
is the dedicated next vertical slice required by the system completeness audit.

Completing FoldDomain does not complete P1, the geology blueprint or
JericoEngine 1.0.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
