# Generic GPU heightfield erosion provenance

Date: 2026-08-26

## Productive implementation

The retained productive implementation is Jerico-owned generic code. It
ports the already admitted deterministic CPU contract in
`GeologyHeightfieldErosion.cpp` to five Diligent/Vulkan compute stages:

1. rainfall plus cardinal outflow;
2. deterministic water gather;
3. hydraulic capacity plus erosion/deposition;
4. deterministic sediment gather plus evaporation;
5. deterministic thermal/talus gather.

No external source fragment was copied into the productive C++ or HLSL in this
checkpoint. The GPU formulation deliberately uses gather passes and ping-pong
fields instead of unordered particle writes. Height, water, sediment, capacity,
flow and cumulative physical diagnostics stay in VRAM between stages. The
public validation test permits one final readback and rejects intermediate
readbacks.

## Authorized references

The designer explicitly authorized adapting code from the following reference
repositories with citation. That authorization does not replace their licenses.

| Project | Pinned reference | License treatment | Use in this checkpoint |
| --- | --- | --- | --- |
| Hydra | https://github.com/ozikazina/Hydra/tree/6d940bd7aa76aaf560ad9b50658cc24e7df57b20 | MIT; retain required notices for any future literal graft | Conceptual review of multi-pass GPU hydraulic/thermal erosion; no copied code |
| TerraForge3D | https://github.com/Jaysmito101/TerraForge3D/tree/77f87609a2ecdc3c57ec0079f6fcaab7d5682d7c | MIT; retain required notices for any future literal graft | Conceptual review of GPU terrain macroforms/domain warp; no copied code |
| Sebastian Lague Hydraulic Erosion | https://github.com/SebLague/Hydraulic-Erosion/tree/f245576d204978e3186f41c8abbd75c326c6857e | MIT; retain required notices for any future literal graft | Mathematical cross-check only; no copied code |
| Ymir | https://github.com/liminalfield/ymir/tree/912d5e71c1f637762e69d74c940b05a976045768 | GPL-3.0; study concepts only unless Jerico explicitly accepts GPL scope | Architecture and terminology review only; no copied code |

SoilMachine was not used because the inspected upstream state did not provide a
clear enough license file for a productive graft. A README license claim alone
is not treated as sufficient provenance.

## Equivalence and performance contract

- Small deterministic identity: `24x20`, 12 iterations, maximum admitted
  CPU/GPU per-value deviation `0.0005 m`.
- Performance identity: `384x320`, 12 iterations, maximum admitted final-height
  deviation `0.01 m` for the Vulkan float contract.
- Metrics retain their kind: CPU is wall duration; GPU compute is device
  duration; GPU wall duration is reported separately.
- No CPU fallback is permitted. Guard failures return explicit errors before
  dispatch or allocation.
