# M8 RBrush Dual Brush Editor

Estado: `PARTIAL` — checkpoint verificable de RBrush Slice 17 Editor. Añade
autoría Dual Brush al editor productivo, pero no cierra Slice 17, Slice 18, M8
ni el Goal maestro.

## Autoridad y alcance

- Spec maestro:
  `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md`.
- Diseño:
  `docs/superpowers/specs/2026-08-12-m8-rbrush-dual-brush-editor-design.md`.
- Plan:
  `docs/superpowers/plans/2026-08-12-m8-rbrush-dual-brush-editor.md`.
- Diseño/plan: `2de2565`, `78b70fe`.
- Producto/pruebas: `03baa91`, `0f9b78d`, `7327b5b`, `bea29d8`,
  `ebca08f`.

El vertical reutiliza el IR Dual Brush ya admitido desde ABR, GUID estables,
el contenedor RBrush, el catálogo del proyecto y el único ProjectAsset Replace.
No crea otro formato, catálogo, scanner, History ni ruta de reimportación.

## Comportamiento demostrado

- `Configured` distingue ausencia canónica de una configuración deshabilitada.
- `Enabled` conserva el estado runtime del Dual Brush sin eliminar sus datos.
- `Secondary preset` selecciona un preset del mismo pack por identidad estable y
  persiste su `PrimaryTipGUID`; la UI no solicita un GUID crudo al usuario.
- Blend mode admite los modos comunes y conserva texto admitido desconocido.
- Shape expone diameter, angle, roundness, hardness, spacing, flips,
  interpolation y antialias.
- Scatter expone amount, both axes, control, count, jitter y count control.
- Preview mantiene bytes intactos; Cancel restaura controles; Apply persiste;
  Undo/Redo alternan bytes exactos y reopen vuelve a resolver el preset
  secundario por identidad.
- Dual Brush posee un bit de ownership independiente. Reimport conserva la
  configuración completa o la ausencia poseída; sin ownership, la fuente nueva
  gana.
- IR schema 8 y container 1.7 persisten ese ownership. Schema 7/container 1.6
  siguen legibles y rechazan el bit nuevo al escribir en vez de perderlo.

## TDD observado

1. El RED de schema no compiló porque faltaba
   `RBrushPresetOverrideSection::DualBrush`.
2. El RED de edición no compiló porque `RBrushPresetEditableProperties` no
   contenía `dualBrush`.
3. El RED de reimport reprodujo sustitución de la configuración poseída y de la
   ausencia poseída por los datos entrantes.
4. El RED Qt falló al no encontrar los siete controles Dual Brush requeridos.
5. El primer build GREEN detectó una referencia UI inválida a la constante
   privada `maximumSourceTextBytes`; se sustituyó por el mismo límite local de
   4096 bytes y el build focal pasó.

## Verificación fresca

- Build focal Debug: PASS.
- Test focal Qt Debug: `1/1 PASS`, 1.87 s.
- Matriz integrada Debug: `7/7 PASS`, 22.78 s.
- Build focal Release: PASS, 103.7 s.
- Matriz integrada Release: `7/7 PASS`, 9.46 s.
- Gates finales Debug: `5/5 PASS`, 28.38 s.
- Gates finales Release: `5/5 PASS`, 26.80 s.
- Gates incluidos: Qt admission, scheduling contract, Shipping source
  exclusion, runtime-only target discovery y Qt deployment.
- El deployment gate detectó copias stale de GltfCook en JericoEditor y
  JericoEditorShellTests. Se reconstruyeron los targets oficiales en ambas
  configuraciones y los gates finales pasaron.

## Evidencia visual retenida

- Archivo: `docs/evidence/images/m8-rbrush-dual-brush-editor.png`.
- SHA-256:
  `97F90EC23493E0CD8E0CC5A8BFE2B3A2512162256481BF5EECE974DDFF88AFA5`.
- Tamaño: 46,170 bytes.

La captura proviene del ejecutable Qt real y fue inspeccionada a resolución
original. Muestra el preset principal `Rocks/Cliffs/Cliff Scatter`, el preset
secundario `Rocks/Secondary/Rock Chips Secondary`, blend `overlay`, spacing
90%, controles Shape y el Preview `Configured Off -> On`, con bytes canónicos
sin cambios hasta Apply. No es concept art.

## Límites honestos

Slice 17 aún necesita Texture settings, SDF defaults y los restantes estados
editables exigidos por la autoridad. Slice 18 debe demostrar el canvas y la
pintura real. Este checkpoint no demuestra Brush Engine, renderer, pintura,
cook visual, residency, JSVGI ni Shipping de un resultado pintado.

## Siguiente capacidad legal

Continuar RBrush Slice 17 con `Texture settings`, mediante RED real, ownership
separado, la misma transacción reversible y evidencia Debug/Release. No abrir
Slice 18 ni adelantar M9.
