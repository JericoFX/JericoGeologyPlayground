# F4 native Diligent ImGui diagnostic preview

Date: 2026-09-05. Parent: `92db6c2ce2f5931b5119e34d7739f5541560a832`.
Status: **PASS for the bounded diagnostic workflow; F4 PARTIAL, rock Beauty RED**.
Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.

The owner explicitly authorized a basic native ImGui tool after deferring final
UI work to M10.5. `Tools/GeologyPreview` reuses the admitted Diligent ImGui renderer
and its pinned Win32 backend, RenderDevice, resident GeologyGpuSession and
GeologyCook. No Qt controls were resumed and no new dependency was admitted.
Build/run instructions and limitations: `Tools/GeologyPreview/README.md`.

## Observable workflow and evidence

The real F4 member from `build/windows-vulkan-editor/f4-strata-warp-material-01`
loads `member-1.geology.toml`, `member-1.rmesh`, `member-1.rcoll`, `authored.rmat`
and three cooked textures. The one-material dense mesh must have exact static
collision and matching recipe/source identities; unsupported body physical
overrides reject explicitly. The existing triangle physical palette survives cook.

The initial native runtime RED is `f4-native-preview-red.log`, exit1: the loaded
rock rendered but actual ImGui Generate/phase control rectangles were absent.
All log names here are relative to `build/windows-vulkan-editor`.

Final build `f4-native-preview-reviewed-build.log`: exit0. Native `--verify`
`f4-native-preview-green-04.log`: **exit0, 87.8263784s**, validation enabled on
the existing Windows/RTX3090 profile. It drives the actual ImGui input path:

1. Increment Strata phase from0 to0.05cycles. Only the draft changes.
2. Generate asynchronously. Real vertex/index data changes; triangles3512 ->3280.
   The loop renders12221 frames while work is pending (a responsiveness witness,
   not a performance budget or achieved-FPS measurement).
3. Capture the displayed result before overlay rendering. Readback verifies exact
   saved recipe, mesh bytes and collision bytes, independently of the draft.
4. Edit/generate again and cancel. The first published result remains unchanged.
5. Restore loaded inputs. Recipe, mesh and collision bytes match the originals;
   restored clean framebuffer bytes equal the initial clean frame exactly.

One material, three textures, fallback count0 throughout. Generated recipe hash
`15111637054923143238`; source hash `8071145322725865162`. Same camera/light as
the matched F4 views: yaw25, pitch-15, distance5m, worldY-1.4m, FOV40 degrees.
Inspected original1280x720 images, copied without modification:

| Artifact | SHA256 |
|---|---|
| `images/m8-f4-native-preview-01/panel.bmp` | `949245867A21C10597BE1ED0124A83B75AD41742946C714C74C7F94540430106` |
| `images/m8-f4-native-preview-01/loaded.bmp` | `8F0E44390F487F2E8435088975799F292B2E876CFAAA8E86ADA93A458074903E` |
| `images/m8-f4-native-preview-01/generated.bmp` | `DD11E0DDF3418845E95BE041C7264928887B42432598DDD0BA7E4A8073BC3283` |

Captured products remain in `f4-native-preview-green-04/capture-1`:

| Artifact | SHA256 |
|---|---|
| `mesh.rmesh` | `231499F0E7E79B70046E7905F45ACD06054A56FCC49D813B602BCAC0E58DFB7E` |
| `collision.rcoll` | `54216C5950ABF14A25B71F400F4EF23849E8AA61F3DDF5CBEBFBA91F70985142` |
| `recipe.geology.toml` | `8369508BC7821F4DA4861980D58AB0DD1D3354E869D8E10599FC7D5D911165A3` |

Three CLI negatives pass before creating a Vulkan device: existing output,
output nested inside texture inputs, mismatched collision identity. Their logs
are `f4-native-preview-negative-{existing,nested,identity}.log`; exit1 each,
no rejected output created. Input-file hashes before/after these checks match;
the inventory is `f4-native-preview-input-hashes.txt`.

## Vulkan defect exposed by concurrent operation

`f4-native-preview-green-03.log` failed after91.803s with validation
`UNASSIGNED-Threading-MultipleThreads-Write`: graphics and compute submitted to
the same VkQueue from separate threads. Diligent's pinned `LogicalDevice::GetQueue`
ignored its `queueIndex` argument and passed literal0 to `vkGetDeviceQueue`.
Each command-queue wrapper had its own mutex, so the alias was not synchronized.

The repository-owned one-line patch `0002-honor-vulkan-queue-index.patch` forwards
the existing argument. The existing CMake patch runner applies it reproducibly
inside the configured dependency tree. The admitted version/API is unchanged;
external reference trees were not edited. Reapplying reports already applied
(`f4-native-preview-patch-idempotence.log`, exit0). With the corrected backend,
the full concurrent workflow passes with validation active. No second Vulkan
device, disabled validation or frame-loop lock was used as a workaround.

Final review found a required companion correction: the upstream default factory
requests one queue but passes index1 to its command-queue wrapper. The ignored
argument had masked this invalid index. With patch0002 alone, standalone
`JericoGeologyGpuSessionTests --strata-field-proof` stopped in a native
`Runtime assertion failed` dialog before shader creation. The owned test process
927760 was explicitly terminated after81.666s; this is a reproduced assertion
RED, not a normal test exit. Logs: `f4-native-preview-standalone-red.log` and
`f4-native-preview-standalone-red-process.txt`. Patch
`0003-default-vulkan-queue-zero.patch` corrects that one factory argument to0.
Both patches are required together and independently idempotent. No other
GetQueue consumer exists in the pinned Vulkan backend beyond its queue wrapper.
Final target build `f4-native-preview-final-build.log`: exit0. Standalone Strata
proof `f4-native-preview-standalone-green.log`: **exit0, 75.3702916s**;
CPU/GPU maximum parity error1.5731e-7, field hash12696178019175010622,
structural hash3141405104561149360. The proof's SVG is retained as
`f4-native-preview-standalone-green.svg` (the invocation initially used a `.json`
suffix; only that output filename was corrected afterward). Companion patch
reapplication also passes, `f4-native-preview-default-patch-idempotence.log`.

Final combined-backend replay `f4-native-preview-green-05.log`: **exit0,
86.5459303s**, Vulkan validation enabled,12039 busy frames. Panel/loaded/generated
images and captured recipe/mesh/collision match green04 byte for byte, including
all hashes above. The full input/generate/cancel/restore workflow passes again.
Final `render.vulkan_device` recheck passes1/1 in0.80s
(`f4-native-preview-final-render-test.log`). No runtime validation errors remain.

Rebuilt `JericoRenderDeviceTests` with the corrected backend:
`f4-native-preview-render-build.log`, exit0. Focused `render.vulkan_device`
passes1/1 in1.06s (`f4-native-preview-render-test.log`). This does not claim
additional hardware profiles or a complete renderer regression suite.

## Completeness audit and limits

Source parameters -> canonical GPU generation -> classified/structural field ->
canonical cooked mesh/exact collision -> existing material renderer -> real
clean capture. One worker owns session evaluation, and main-thread publication
replaces the preview only after success. Worker cancellation/wait precedes
renderer teardown. Captures publish through a new pending directory; failed
writes retain it for inspection. Input files are read-only; no project scene,
binary asset or editor bridge transaction is modified.

The tool is diagnostic and Tool-only. Existing-layer values, draft revert,
loaded-result restoration and camera controls are available. Layer insertion,
reordering, scene Apply, full undo, localized RBrush and M10.5 UI are not delivered.
Initial shader setup remains non-interruptible (about88s in the prior run);
Cancel/close wait for its safe boundary, clearly stated in the panel and README.
Subsequent evaluation checks canonical cancellation. Captures use actual window
resolution; only the retained1280x720 views above were accepted as this witness.

Downstream physics/gameplay, LOD/streaming, destruction, scripting, animation,
events, Sequencer, audio, particles and Shipping are not changed by the host.
Their existing actual-member F4 gates remain INTEGRATION REQUIRED, owned by their
respective engine subsystems and dependent on the accepted F4 geometry/cooked
products. Full UI is M10.5. No final physical/gameplay/Shipping acceptance follows
from rendering this tool's cooked input.

The images still show broad artificial facets and regular tiers: **Beauty RED**.
Next: return to the shape-aware weathering operator/working-region admission
audit with a bounded RED on the actual member; use this tool for controlled
comparisons, preserving six cuts, source correspondence and semantic lineage.

READ_ONLY auxiliary review covered input/capture identity, worker lifetime and
queue use; findings were addressed or explicitly bounded above. Preserve the
preexisting189-entry checkout inventory (SHA256
`e0407c64c57944500fa01272d526c21f9a248cc7f784799588b3a72ffbf28d08`).
