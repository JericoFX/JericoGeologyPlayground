# Native geology preview

Small diagnostic host using the admitted Diligent ImGui renderer and Win32 input
backend. Final JericoEditor UI remains M10.5. Requires the Windows Vulkan editor
preset; it is a Tool target and is not linked into Shipping.

Build with `cmake --build build/windows-vulkan-editor --config Release --target JericoGeologyPreview --parallel 4`.
Run `build/windows-vulkan-editor/Tools/GeologyPreview/Release/JericoGeologyPreview.exe`
with these arguments:

```text
--recipe FILE.geology.toml --mesh FILE.rmesh --collision FILE.rcoll
--material FILE.rmat --textures COOKED-TEXTURE-DIRECTORY --output NEW-DIRECTORY
```

Inputs must be a matched document-free BlockGrammar/AperiodicTiling recipe and
its dense geological mesh/exact static collision, with one admitted geological
material and its cooked textures. Body-level physical overrides are rejected;
the existing triangle physical material palette is retained through GeologyCook.
The output directory must be new and outside the input texture directory.

- **Auto update** starts enabled and prepares the resident GPU session. Drag
  numeric values, or Ctrl+click to type an exact number. Edits are coalesced for
  250 ms; a newer edit cancels/discards obsolete work without overwriting the draft.
  Camera, controls and the last valid mesh remain available during generation.
- Select a layer to enable/disable it and edit its typed payload. **Add** inserts
  the chosen template after the selection; **Duplicate** creates a new identity.
  **Remove layer**, **Move up/down**, and bounded **Undo/Redo** operate on the draft.
  The source cannot be removed, duplicated or moved. Invalid recipes show the
  canonical validation error; invalid generation preserves the displayed result.
- The catalog contains 25 templates covering the existing document-free GPU
  operations, including three weathering variants. It is not a claim that every
  ordering/combination is supported. Dependency/order checks remain mandatory.
  Directed cracks require the existing spline workflow. MacroWarp, legacy
  Fracture, CaveSweep and standalone HeightfieldErosion are unavailable in this
  host; no CPU fallback generator is introduced. Existing unsupported payloads
  are preserved. Mask graphs and spline authoring remain in their existing tools.
- **Generate** uses the canonical resident GPU session and cooker. The previous
  result remains visible until the replacement succeeds. Disable Auto update
  to submit manually. The GPU device/session is reused after the first setup.
- **Cancel** discards the pending candidate at a safe boundary. Initial shader
  setup is not interruptible and remains expensive; consult the measured evidence
  for the exact configuration rather than assuming a fixed startup time;
  closing during that setup also waits for safe worker shutdown.
- **Revert edits** restores the displayed recipe; **Restore loaded** restores the
  original loaded recipe, mesh and collision. No source asset or scene is edited.
- **Save draft** writes a new numbered recipe, independently of the current mesh.
  A captured result includes the matched recipe/mesh/collision for reopening with
  the same CLI and material/textures. A draft alone is not a cooked matched set.
- **Capture** saves the currently displayed result, even if the draft differs:
  original-size `beauty.bmp` without the panel, exact cooked mesh/collision,
  rendered recipe and camera/hash sidecar. Each capture gets a new numbered
  directory. Failed writes retain a `.pending` directory for inspection.

There is no scene Apply, localized painting or final editor workflow in this
host. No geometry/LOD threshold is relaxed by the preview. Camera poses start at the F4
matched-view defaults; the panel can be moved or collapsed.

For the bounded Strata fixture, append `--verify`. This drives actual ImGui mouse
input without moving the desktop cursor, checks draft isolation, changed geometry,
capture recipe/mesh/collision, cancellation and byte/pixel-exact restoration.
It runs a hidden native Vulkan window with validation enabled, saves original
1280x720 panel/clean images and fails if the workflow does not finish in six
minutes. Input/output paths and upstream cooked material/textures remain required;
the captures are diagnostic artifacts, not a standalone Shipping package.

`--verify-controls` runs the native add/remove/duplicate/Undo/Redo checks without
starting geology generation. `--verify-live` additionally verifies editing during
GPU work, obsolete-result cancellation, automatic publication of the latest draft,
session reuse and matching saved/captured data. Both use the same input widgets.

**Generation timings** separates setup, evaluation wall time, measured GPU passes
and CPU cook. Evaluation also includes CPU topology/validation and readback; the
GPU pass sum is not end-to-end latency. Initial shader creation is not yet
instantaneous. The host makes no unconditional real-time or all-GPU claim.

For a photograph of already cooked inputs, append `--capture-only` instead.
It renders four frames in a hidden native window, saves `capture-1` with the clean
image, matching recipe/mesh/collision and camera metadata, then exits. It does not
regenerate the recipe and therefore does not certify structural-field export or
the interactive Generate workflow for those inputs.

`--capture-views` saves three native views of the same cooked inputs, without
regeneration or parameter changes; their camera poses are recorded per capture.
