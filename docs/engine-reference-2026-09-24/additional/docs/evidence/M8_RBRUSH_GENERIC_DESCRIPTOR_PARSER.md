# M8 RBrush Generic Descriptor Parser Evidence

## Estado

- Fecha: `2026-08-11`
- Milestone: `M8 Terrain and Surfaces`, `ACTIVE / PARTIAL`
- Slice: `RBrush 9 — Generic Descriptor Parser`
- Estado del slice: `PASS`
- Producto: `18be652 feat(asset-pipeline): parse generic abr descriptors`
- Diseño: `fecaafe docs(rbrush): design generic abr descriptors`
- Plan: `f75be04 docs(rbrush): plan generic abr descriptors`
- Autoridad: `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md`

Este checkpoint cierra sólo el parser genérico read-only de descriptores ABR.
No implementa el semantic mapper, no relaciona todavía `desc -> samp`, no
publica assets y no adelanta UI, Terrain, PWAS/EPLS 2.0 ni runtime.

## RED auténticos

1. El primer test focal no compiló porque todavía no existía
   `Jerico/AssetPipeline/AbrDescriptorParser.hpp`.
2. El inventario de pruebas detectó la incorporación deliberada del test y se
   reconcilió con su hash exacto antes del GREEN.
3. La matriz incremental exigió, uno por uno, los tipos escalares, objetos,
   listas, referencias, strings IDs, Unicode, rangos y bytes raw.
4. OSTypes y reference forms desconocidos demostraron preservación parcial con
   `complete=false`; framing conocido malformado, límites y cancelación
   conservaron errores estables.
5. La regresión integrada reveló y cerró cuatro defectos ajenos al grammar:
   una attestation de Weighted Poisson retenida, una dependencia `simdjson`
   filtrada fuera de su adapter privado, inicialización Vulkan sobre Qt
   `offscreen` y una carrera de cancelación de 250 ms al cambiar proyecto.
6. El gate completo expuso además pruebas transaccionales que se pisaban en el
   almacén global de recovery attestations. Cinco pruebas cortas quedaron
   `RUN_SERIAL`; el preset conserva cuatro workers para las restantes.

## Contrato implementado

- `AbrID` conserva IDs de cuatro bytes y string IDs sin interpretación.
- El documento expone roots, propiedades en orden fuente, valores recursivos,
  bytes raw, offset/rango fuente, `complete` e issues tipados.
- La matriz admite `VlLs`, `Objc`, `GlbO`, `TEXT`, `UntF`, `bool`, `long`,
  `comp`, `doub`, `enum`, `tdta`, `type`, `GlbC`, `alis` y referencias `obj `
  con forms `prop`, `Clss`, `Enmr`, `rele`, `Idnt`, `indx` y `name`.
- Los enteros/doubles big-endian y UTF-16BE se validan y conservan con bounds.
- Profundidad, cantidad de items, strings, raw bytes y fuente total están
  acotados; la cancelación se observa durante el recorrido.
- `AbrInspector` reutiliza el parser compartido; no queda un segundo walker de
  descriptor.
- `abr_inspect --descriptors` produce un dump genérico determinista.
- No se agregó ninguna dependencia ni se cambió un formato Shipping.

## Fixture ABR real

Fixture local inmutable, ignorada y nunca staged:

```text
F:\JericoEngine\build\local-fixtures\abr\Free Cracks Photoshop Brushes 10.abr
bytes: 23,435,296
SHA-256: 0D8875E5E19228A828F3A259348B8B94DB4584849F33BFC66DD8287FB65FEE9D
document_version: 16
roots: 1
complete: true
unknown_types: 0
```

El dump Release demostró los nombres `121`..`135`, diámetro `2500 #Pxl`,
ángulo `0 #Ang`, roundness `100 #Prc`, spacing `25 #Prc`, interpolation
habilitado y flips/toggles restantes deshabilitados.

IDs `sampledData` en orden fuente:

```text
121 c82b7e16-3f15-11e6-9b00-89ae4ef4d136
122 ce31f069-3f15-11e6-9b00-89ae4ef4d136
123 d092c525-3f15-11e6-9b00-89ae4ef4d136
124 d3659474-3f15-11e6-9b00-89ae4ef4d136
125 d365947c-3f15-11e6-9b00-89ae4ef4d136
126 e6763cee-3f15-11e6-9b00-89ae4ef4d136
127 eee2db2b-3f15-11e6-9b00-89ae4ef4d136
128 f13d2276-3f15-11e6-9b00-89ae4ef4d136
129 f398a3c2-3f15-11e6-9b00-89ae4ef4d136
130 f398a3ca-3f15-11e6-9b00-89ae4ef4d136
131 f910b221-3f15-11e6-9b00-89ae4ef4d136
132 fb361817-3f15-11e6-9b00-89ae4ef4d136
133 ff39c669-3f15-11e6-9b00-89ae4ef4d136
134 ff39c671-3f15-11e6-9b00-89ae4ef4d136
135 027d8b7b-3f16-11e6-9b00-89ae4ef4d136
```

## Gates finales

```text
Debug RBrush focal: 6/6 PASS (1.93 s)
Debug repository/RBrush: 7/7 PASS (71.29 s)
Release focal: 3/3 PASS (0.44 s)
Weighted Poisson fixture regression: 10/10 PASS (582.22 s)
Editor regressions iniciales: 5/5 PASS (356.85 s)
Scheduling/transacciones final: 6/6 PASS (44.47 s)
Debug completo final: 249/249 PASS (2764.78 s)
windows-vulkan-runtime configure: PASS (30.2 s)
runtime-only Release build: PASS (40.2 s)
runtime-only artifact-name hits: 0
runtime-only binary-symbol hits: 0
runtime-only target hits: 0
```

La auditoría runtime-only buscó `AbrDescriptorParser`, `AbrInspector`,
`abr_inspect`, `8BIMdesc` y `.abr`. El adapter JSON privado conserva a
`simdjson` fuera de la unidad de caché admitida por el contrato del repositorio.

No hay evidencia visual nueva para Slice 9: es un contrato de datos. La hoja de
contacto de Slice 8 continúa siendo evidencia del decoder de samples y no se
reutiliza como prueba del parser de descriptores. Las imágenes conceptuales del
usuario tampoco son evidencia.

## Límites y continuidad

- M8 y JericoEngine 1.0 permanecen `ACTIVE / PARTIAL`.
- C04 Jobs/Async permanece `PARTIAL` fuera del owner Smart Import/GLTF ya
  demostrado; cooker/import/recook, SDF/PWAS/EPLS/Nav bake y package siguen
  sujetos a GAP-10/PB-06.
- Virtual Texturing/páginas MegaTexture permanece diferido a JericoEngine 1.1.
- La autoridad de rigging in-editor de `b29ff08` fue leída y retenida, pero no
  autoriza adelantar Animation/Rigging fuera del roadmap.
- Próxima acción única: RBrush Slice 10, Semantic Mapper.

Estado: `PASS` (RBrush Slice 9 solamente)

Resultado: parser genérico bounded y determinista probado con fixture real.

Pruebas ejecutadas: focales Debug/Release, regresiones integradas, 249/249 y
runtime-only Release.

Evidencia: este documento y los outputs locales ignorados del dump real.

Faltantes: Slice 10 y slices posteriores; M8/Goal 1.0 no están completos.

Riesgos: C04 global continúa parcial; aún no existe relación semántica
descriptor/sample.

Siguiente slice: RBrush Slice 10 Semantic Mapper.
