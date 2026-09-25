# M8 RBrush Shared Resource Authoring Hints

Estado: `PARTIAL` — checkpoint verificable de RBrush Slice 17 Editor. No cierra
Slice 17, M8, Virtualized World, JSVGI ni el Goal maestro.

## Autoridad y decisiones

- Diseño: `docs/superpowers/specs/2026-08-12-m8-shared-resource-authoring-metadata-design.md`.
- Plan: `docs/superpowers/plans/2026-08-12-m8-rbrush-resource-authoring-hints.md`.
- ADR: `docs/decisions/ADR-034-SHARED-VIRTUALIZED-WORLD-AUTHORING-CONTRACTS.md`.
- Diseño/ADR/plan: commit `290dd2e`.
- Producto: commit `396d202`.

El seam nuevo vive en Engine y no depende de Qt, Diligent ni herramientas. Separa
intención persistente de autoría (`ResourceAuthoringHints`) de invalidación
transitoria (`DirtySpatialRegion`). Los hints no son órdenes al runtime: el futuro
residency manager conserva la autoridad sobre presupuestos, promoción y eviction.

## Comportamiento demostrado

- Metadata canónica compartida para surface/VT, geometry detail, world streaming,
  shadow pages y participación JSVGI, con rangos y enums cerrados.
- Dirty AABB finito con máscara explícita para geometry pages, collision,
  navigation, surface pages, shadow pages, GI bricks y world-cell dependencies.
- RBrush IR schema 5 y contenedor 1.4 persisten los cinco grupos de hints.
- Schemas 1-4 y contenedores 1.0-1.3 siguen siendo legibles con defaults neutros.
- Un writer antiguo rechaza hints no neutros; no los pierde silenciosamente.
- Edición pura de un preset y transacción Preview/Cancel/Apply/Undo/Redo reutilizan
  el único camino canónico de reemplazo del pack.
- Reimport conserva los hints del preset estable junto con nombre, grupo y tags.
- Shipping/runtime no adquirieron dependencias de authoring.

## TDD

RED observados:

1. C1083 por ausencia de
   `Jerico/VirtualizedWorld/ResourceAuthoringMetadata.hpp`.
2. C2039 porque `RBrushPresetIR` no tenía `resourceHints`.
3. C1083 por ausencia de
   `Jerico/AssetPipeline/RBrushPresetResourceHints.hpp`.
4. `asset_pipeline.rbrush_update` falló igualdad exacta al perder hints durante
   reimport; quedó GREEN al conservarlos desde el preset estable.

## Verificación final fresca

- Debug focal/integrado: `9/9 PASS`, 29.24 s.
- Release focal/integrado: `9/9 PASS`, 21.66 s.
- Gates de repositorio Release: `4/4 PASS`, 301.34 s:
  - `repository.test_scheduling_contract`;
  - `repository.runtime_only_target_discovery`;
  - `repository.shipping_source_exclusion`;
  - `editor.boundary_audit`.
- `git diff --check` focal: PASS; sólo avisos de conversión LF/CRLF de Git.

## Límites honestos

Este checkpoint no implementa page tables, residency/eviction, geometry streaming,
virtual shadows, JSVGI runtime, render de RBrush ni controles Qt. Sólo evita que
RBrush y los siguientes productores de mundo nazcan sin el contrato de metadata e
invalidación exigido por las nuevas autoridades M8.

## Siguiente capacidad legal

Exponer el primer grupo productivo de propiedades `Tip + Shape Dynamics/Scatter`
en el RBrush Editor usando la transacción existente, con persistencia real,
Preview/Cancel/Apply/Undo/Redo y sin UI vacía.
