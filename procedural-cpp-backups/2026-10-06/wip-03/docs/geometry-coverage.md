# Cobertura geométrica nativa

Alcance: los generadores mostrados y sus referencias congeladas, no una librería de modelado general. Este pase separa tres cosas: construcción procedural, validez del asset evaluado/exportado y parecido visual medido. No se ha inferido paridad visual de un JSON ni se han producido nuevos renders en esta auditoría.

Las cuatro comparativas medidas del proyecto cubren `industrial_louvered`, `wood_reinforced`, `cabinet_open` y `damp_old_broken`, con RGB MAE global entre 0,000618 y 0,001590 en PNG RGB 0–1. Su aceptación corresponde a esas recetas y condiciones de comparación; no demuestra identidad de todo el espacio de parámetros. Evidencia: `output/visual-family-final/visual-metrics.json` e `independent-material-final-review.json`.

## Construcción que está portada

Referencias activas: MaterialsR2 19:27 para núcleo/presets/industrial/daño, GlassFractureStudy para el vidrio aprobado, FineWear 19:21, Damp DirectedWIP 19:56 y Furniture 19:56. Los snapshots históricos restantes no sustituyen estas referencias; los manifiestos de fixtures identifican la procedencia.

| Paso de las fuentes Python | Implementación C++ y resultado |
|---|---|
| Normalización, RNG, transformaciones, `Builder`, primitivas, triangulación y UV métricas | `core.cpp` / `scene.hpp`: nodos, prismas, cajas, tablas astilladas, cilindros, remaches, manillas, sockets y grips. RNG reproduce Python; semillas base signed64 y offsets derivados hasta 65 bits sin overflow. |
| Puertas y ventanas de madera, refuerzos, daño aged/deteriorated/broken | `wood.cpp`: frames, hinges, leaf/planks/braces, manillas/cerraduras y sus omisiones. `build_preset` queda representado por `build_wood`; `build_singlepane` conserva marco fijo, junquillos y estados intact/broken/empty. |
| Industrial solid/vented/louvered/window | `industrial.cpp`: chapa, pliegues, marcos, rejilla de alambre, lamas inclinadas, bisagras, cerradura, rebaje y vidrio. Las aberturas de diseño son recortes de volumen, no texturas de transparencia. |
| Macrodaño y cortes manuales | `damage.cpp`: `carve_panel`, `irregular_polygon`, `point_in_panel`; validación de perfiles, partición XZ, conformado, coalescencia de subdivisiones pequeñas, extrusión Y, cancelación de caras internas, materiales/procedencia y volumen incremental. Las cavidades front/back y cortes pasantes se componen sin restar dos veces el solape. |
| Grietas modeladas | `damage.cpp::crack_profiles`: trayectoria irregular, ancho/taper y miters limitados; cavidad principal al 65% de profundidad, tramo profundo al 100% y ramas al 45%. Los perfiles son geometría real y se aplican en orden. |
| Selección y soporte funcional | `localized_damage.cpp`: selección exacta de parte/cara, regiones box/sphere, feather, distancia a contornos reales, keep-outs de muestras/columnas de soporte, transformaciones entre nodos, operación transaccional y actualización del desgaste tras cortar. El override revalida e invalida sockets y grips sin soporte. |
| Vidrio fracturado aprobado | `fracture.cpp`: abanico irregular, hueco, labios astillados, ranuras radiales y parciales, separación, apoyo sobre junquillos, guardia de solapes y fallback de radios rectos conservando los draws RNG. |
| FineWear | `fine_wear.cpp` + `materials.cpp`: reducción exacta de contornos y opt-in frontal; el shader de ruptura fina se evalúa en C++. El perfil de procedencia distingue FineWear del industrial R2. Es modificación de material, no topología añadida. |
| Madera húmeda | `damp_wood.cpp`: cortes por edad, checks longitudinales y del alféizar, material interior expuesto, conservación de soportes y composición con la fractura aprobada. `build_damp_window` se expone como `build_damp_wood`. |
| Armario y banco | `furniture.cpp`: tablas con surcos/chamfers, tornillería, tiradores, etiquetas, astillas, cajones, guías, estructura, tapas y diagonales; barrido continuo conservador de cajones. |
| Evaluación y transporte | `evaluate.cpp`, `materials.cpp`, `png.cpp`, `export_glb.cpp`: bevel y normales nativos, muestreo procedural, atlas/PNG16, tangentes, GLB, jerarquías, IDs y extras de sockets/grips. No hay proceso ni callback Python/Blender en `src`/`include`; Python se usa en pruebas offline y Blender como referencia/visor. |

El gate integrado actual pasa **23/23 suites**. Incluye los guards de schema/unidades/transformaciones, el barrido de hojas y la validación específica de furniture antes de evaluar/hornear. La antigua declaración huérfana `build_asset` se retiró; las entradas públicas de familia están implementadas.

Las APIs manuales operan sobre la geometría fuente, antes de evaluar u hornear. `carve_panel` retorna una pieza y no tiene contexto de escena para invalidar sockets por sí solo: esa política pertenece a `apply_operations`. Tampoco debe aplicarse sobre un asset ya horneado conservando sus capas antiguas de normales/atlas/texturas.

## Gate adicional de variantes GLB

`fixtures/variant-export/cases.json` conserva 19 entradas reproducibles: **11 assets válidos y 8 rechazos deliberados**, contados por separado. El gate final produjo 655 mallas, 220.614 triángulos y 3.930 PNG embebidos, todos verificados contra sus escenas por el validador binario independiente. Se comprobó geometría, índices, normales, tangentes, ambas capas UV, jerarquía, PNG/canales, IDs y metadatos funcionales. Ningún rechazo produjo GLB.

| Variantes válidas | Cobertura específica |
|---|---|
| `industrial_vented` | Dos aberturas de diseño, rejilla, lamas y export de sus recortes. |
| `industrial_damage_left`, `industrial_damage_right` | Macrodaño lateral y cavidad; tres operaciones aplicadas en cada una. |
| `industrial_edge_damage` | Doce operaciones de pequeñas mellas compuestas con el hueco de diseño. |
| `industrial_edges_cracks` | Macrodaño, mellas y grieta de profundidad variable con dos ramas. |
| `fine_wear_after`, `dense_wear_continuous` | Catorce/veintidós operaciones, con dos/catorce configuraciones FineWear y grietas de 1 mm en el caso denso. |
| `support_front_override`, `support_front_protected` | El override desactiva sólo el socket/grip frontal y conserva el posterior. Sin override, el informe es `protected`, no se corta y las interfaces siguen habilitadas. |
| `manual_concave_through`, `manual_front_back_overlap` | Perfil cóncavo arbitrario pasante y cavidades front/back cuyo solape atraviesa el espesor; llamadas directas a la API C++ sobre la misma puerta industrial. |

Los ocho rechazos son: profundidad cero; grieta fuera de región; exceso de 12 cortes por pieza; FineWear posterior; parte inexistente; semilla fuera de signed64; perfil manual autointersectado; y configuración de fractura no construible con seguridad. No equivalen a ocho assets generados.

Se solicitó bake **64 sólo para validez binaria**. El empaquetador adaptativo necesitó atlas de 32–512 según pieza y tuvo **cero islas vacías**. Estas imágenes pequeñas no son evidencia visual ni texturas de entrega de alta calidad. No se repitieron renders.

Los datos iniciales quedan en `output/variant-export-gate/manifest.initial.json`. Los **11 GLB se reexportaron y validaron con la convención final de tangentes**, reutilizando las escenas y PNG nativos. Sólo se regeneraron las dos variantes FineWear por su perfil explícito: sus **1.136 PNG permanecieron byte por byte idénticos**. Las fases y hashes están en `manifest.json`; el resumen compacto sellado está en `fixtures/variant-export/verification.json`. Los binarios y el validador no cambiaron durante cada fase final.

Reproducción:

```sh
cmake --build build --target jerico_procedural variant_export_driver export_native_scene
python3 tools/verify_variant_exports.py --bake 64 --jobs 2
# Sólo empaquetar de nuevo escenas C++ ya evaluadas y horneadas:
python3 tools/verify_variant_exports.py --mode reexport --jobs 2
```

## Residuales y límites reales

- **Evaluación:** no se clona íntegramente el modificador Blender. Persisten diferencias documentadas de tessellation/clamp/miter; el máximo muestreado en las piezas difíciles fue 0,514 mm. Las normales mixtas y el atlas finito siguen siendo restricciones técnicas aunque las cuatro comparativas se hayan aceptado.
- **Dominio heredado del daño:** prismas rectangulares alineados a ejes locales o sus cortes analíticos, no Booleanos arbitrarios sobre cilindros, lamas inclinadas o cualquier malla. Perfiles de 3–64 vértices, 12 cortes/pieza, 64 operaciones; se rechazan eliminación completa, degeneraciones y regiones genuinamente demasiado pequeñas. Las grietas requieren ancho mínimo de 1 mm y perfiles enteros dentro de su selección.
- **Ciclo de validación:** las pruebas de soporte son conservadoras sobre muestras/columnas AABB. Los barridos de hojas son SAT muestreado y movimiento sincronizado; los cajones usan AABB continuo conservador. No certifican resistencia, conectividad total, soporte no muestreado ni contacto general de herrajes.
- **Aislamiento de materiales:** la auditoría detectó que un metadato `fine_breakup` ajeno a R2 activaba FineWear. La corrección usa `metadata.native_material_profile="finewear_1921"`; R2 no infiere su shader del flag. La regresión cubre true/ausente/string, 151 muestras originales y `material_scope_tests` sobre muestras, wrappers y atlas; está incluida en el gate integrado. Los dos casos FineWear regenerados conservan exactamente sus PNG y llevan el perfil explícito; el fallo de aislamiento está cerrado por esos gates.
- **Formatos secundarios:** la ruta principal GLB está portada. `write_obj_mtl` ya exporta posiciones mundo, UV atlas, normales explícitas y MTL/texturas relativas en C++, con lectura independiente de 184 piezas de cajonera y 52 de ventana. OBJ no conserva jerarquía/UV métricas secundarias ni transmisión física; Blender 4.3.2 ignora la extensión norm. Estos límites se declaran, sin reparar el importador. No hay una API nativa equivalente a todo el empaquetador Python `export_asset`. GLB requiere IOR uniforme por parte y usa float32/PNG16, no identidad binaria con doubles/shaders originales.
- **Auxiliares:** `point_in_glass` sólo lo usan las pruebas fuente para consultar footprints; no falta un paso del constructor ni de sus soportes. No se expone esa consulta ni `asset_hash` canónico de autoría Python. IDs/footprints se conservan y los archivos GLB tienen SHA-256 propio. La selección de familia es explícita (`--family`), no un clon de `recipe_router.build_recipe`.
- **Fuera de estos generadores:** no se ha integrado Jerico/Diligent ni un runtime de colisiones, masa, joints, destrucción física, animación, AO o lightmaps. Nada de este gate binario lo certifica.
