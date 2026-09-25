# M8 RBrush Slice 27 — Derived 2D SDF Evidence

Date: `2026-08-14`

Status: `PASS` for RBrush Slice 27 only.

JericoEngine 1.0, M8, Sparse SDF 2.0, C04 global and the master Goal remain
`PARTIAL / ACTIVE / NOT COMPLETE`.

## Observable criterion

Authority:

```text
R8 crack mask -> valid 2D signed distance representation
```

The retained product converts immutable canonical R8 or little-endian R16
coverage into a bounded Euclidean signed-distance field with the canonical
convention:

```text
distance < 0  inside
distance = 0  surface
distance > 0  outside
```

RBrush continues storing only its canonical mask. The field and its derivation
key are disposable data suitable for a later shared DDC consumer.

## Product

Plan:

```text
6a0928d docs(rbrush): plan derived 2d sdf
```

Product:

```text
78760d0 feat(procedural): derive rbrush 2d signed distance
```

Owned module:

```text
Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/MaskSignedDistanceField2D.hpp
Tools/ProceduralAuthoring/Source/MaskSignedDistanceField2D.cpp
```

The implementation provides:

- R8 UNORM and canonical little-endian R16 UNORM input;
- finite threshold in `[0,1]` and optional inversion;
- up to eight deterministic isolated-texel cleanup passes that preserve
  connected one-texel cracks;
- a separable linear-time exact squared Euclidean distance transform;
- a one-cell exterior pad, so full masks still have a finite negative field;
- half-texel boundary correction and quantization to `1/4096` texel;
- stable `sourceHash`, `derivationKey` and field-only `contentHash`;
- cooperative cancellation at row/column boundaries;
- dimensions bounded to `4096 x 4096` and `16,777,216` texels;
- explicit invalid, budget and cancellation diagnostics under stage
  `procedural.sdf.mask2d`.

It is a Tools/ProceduralAuthoring module. It does not link Qt, Editor or a
runtime/Shipping owner.

## RED retained

The first configure correctly rejected the unregistered scheduling inventory:

```text
repository.test_scheduling.full_inventory_changed
expected_count=285
actual_count=286
actual_sha256=0b4b11e4e3f1e2a009f01ae4d3bd131952619611510d60111a036f60a2793bfb
```

After admitting that exact inventory, the product RED was causal:

```text
MaskSignedDistanceField2DTests.cpp(1,10): error C1083
Jerico/ProceduralAuthoring/MaskSignedDistanceField2D.hpp: No such file or directory
```

The first broad boundary run later exposed the verifier's stale durable marker:

```text
repository.test_scheduling.manifest_marker_missing|test_count=285
```

Only the matching count/hash/message in `VerifyTestScheduling.cmake` was
updated. The focal scheduling gate and the complete 12-test boundary matrix
then passed.

## Exact focal result

The 7x7 R8 fixture contains one 3x3 solid island:

```text
width=7
height=7
inside=9
center=-1.5
inside_edge=-0.5
outside_edge=0.5
interpolated_boundary=0
source_hash=15843968234389860079
derivation_key=17446709849159978306
content_hash=5529427772609841321
```

The test repeats the derivation, verifies input immutability and compares all
512 possible binary 3x3 masks texel-by-texel against an independent brute-force
Euclidean oracle. It also verifies R8/R16 field equivalence, threshold,
inversion, isolated speck/hole cleanup, connected one-texel crack preservation,
empty/full masks, changed-input invalidation, invalid inputs, budgets and
pre-cancellation.

## Real crack ABR proof

Local immutable fixture, preserved outside Git:

```text
F:\JericoEngine\build\local-fixtures\abr\Free Cracks Photoshop Brushes 10.abr
bytes=23,435,296
SHA-256=0D8875E5E19228A828F3A259348B8B94DB4584849F33BFC66DD8287FB65FEE9D
```

The existing admitted decoder produced fifteen canonical `2500 x 2500` R8
masks. The new product derived all fifteen with finite negative and positive
samples:

| Sample | Inside texels | Minimum | Maximum | Content hash |
|---:|---:|---:|---:|---:|
| 0 | 272823 | -12.3062 | 210.142 | 7105488197914439508 |
| 1 | 555473 | -34.2131 | 136.558 | 10900136213176760984 |
| 2 | 575938 | -21.8606 | 175.264 | 16735690941968283815 |
| 3 | 120280 | -22.9094 | 270.073 | 13194258221006987634 |
| 4 | 525315 | -23.5 | 227.984 | 2549284414572234947 |
| 5 | 348943 | -17.7483 | 199.510 | 15768902846573663742 |
| 6 | 235032 | -17.3044 | 207.062 | 14452948295451733618 |
| 7 | 606202 | -18.605 | 217.734 | 5519577463170577933 |
| 8 | 450105 | -23.5 | 262.604 | 10954355398751771356 |
| 9 | 420295 | -21.3403 | 218.521 | 9679149296403590923 |
| 10 | 570804 | -17.8577 | 141.066 | 10063611182710016355 |
| 11 | 216236 | -13.1016 | 151.622 | 15996821641463449061 |
| 12 | 662909 | -15.6245 | 86.8154 | 15851463836490415424 |
| 13 | 402584 | -17.9392 | 101.289 | 14857637222509214613 |
| 14 | 388100 | -16.5 | 183.98 | 12253071995055223698 |

```text
total_inside=6,351,039
combined_hash=13405004027352938727
Debug elapsed=32.4 s
Release elapsed=5.131 s
Release average including ABR decode=342 ms/mask
```

Debug and Release produced the same fifteen content hashes and combined hash.
The measured cost means a future UI consumer must use a C04 job and DDC; this
slice does not call the transform from Qt.

## Visual evidence

Retained product-derived image:

```text
docs/evidence/images/m8-rbrush-derived-2d-sdf.bmp
dimensions=1204x600
bytes=2,889,654
SHA-256=00E1A01ED8F31E97BC073CC090B00029BF5C8BEF31D4A12A3FAA6822D84C0124
```

The left half is canonical sample 0 from the real ABR. The right half maps the
new signed-distance values: white is the zero-crossing neighbourhood, warm
values are inside and blue values are outside. It was generated by the same
Release test executable and visually inspected after generation.

## Verification

```text
Debug focal build: PASS
Debug focal CTest: 1/1 PASS
R8/R16 + ABR decoder pair: 2/2 PASS
Sparse SDF neighbours: 6/6 PASS
RBrush matrix: 10/10 PASS in 51.39 s
Release focal: 1/1 PASS
Debug integrated build: PASS in 34.8 s
runtime-only/Shipping/source-exclusion/authority/scheduling: 12/12 PASS in 167.26 s
git diff --cached --check: PASS before product commit
```

## Explicit non-claims

This checkpoint does not claim:

- persistent DDC storage or cache hit/miss behavior;
- a Qt/editor job owner or live preview;
- any mutation of `.rbrush` or its schema;
- a 3D SDF stamp, projection or Boolean operation;
- crack/chip geometry;
- Sparse SDF 2.0, C04 global, M8 or the master Goal complete;
- runtime or Shipping consumption of RBrush authoring data.

## Next legal slice

RBrush Slice 28 — SDF Stamp. Its design/plan must preserve this disposable 2D
field boundary and open the first observable operation without prematurely
claiming Slice 29 projections or Slice 30 crack geometry.
