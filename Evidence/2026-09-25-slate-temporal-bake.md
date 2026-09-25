# Pizarra: bake high→low con cámara en movimiento — 2026-09-25

Estado: **PASS del diagnóstico temporal acotado; PARTIAL para calidad final**. El nuevo comando `--slate-temporal-bake-test <directorio>` evalúa el preset 13 en LOD2 (high 27.648, low 9.408 triángulos), exporta UV automática y normal/AO/contact/cavity/convex-edge y demás mapas especialistas a 2048², y captura 5 vistas Vulkan 1280×960 separadas por yaw ±0,001 radianes a distancia 11,6. Cada vista tiene high, low con cuatro mips y low sin mips de normal; material y mapas especialistas permanecen iguales. La [prueba RED](2026-09-25-slate-temporal-red.log) carecía de esta referencia; el [audit GREEN](2026-09-25-slate-temporal-audit.json) mide el cambio entre cuadros dentro de la roca, excluyendo dos píxeles del borde, y también el residuo respecto del cambio del high.

| Medida media en valores RGB de 8 bits (0–255) | Low con mips | Low sin mips de normal |
|---|---:|---:|
| Cambio temporal de normal | 0,1698 | 0,2410 |
| Cambio temporal de material | 0,1978 | 0,2231 |
| Residuo temporal de normal respecto del high | 0,2994 | 0,2977 |
| Residuo temporal de material respecto del high | 0,3091 | 0,3162 |
| MAE espacial de normal respecto del high | 4,5799 | 2,5355 |
| MAE espacial de material respecto del high | 4,1181 | 3,7165 |

Los mips redujeron el cambio temporal aislado de normal ~30 % y de material ~11 % en esta secuencia. El residuo temporal frente al high casi no cambió y el MAE espacial empeoró: **no se declara que la calidad del bake o la apariencia geológica esté resuelta**. El normal del high rasterizado tampoco es una referencia libre de aliasing. Estas cinco vistas y una luz/material no sustituyen movimiento representativo, otros LOD o evaluación perceptual.

La captura central está disponible como [high](2026-09-25-slate-temporal-candidate/high-material-2.png), [low con mips](2026-09-25-slate-temporal-candidate/low-mips-material-2.png) y [low sin mips de normal](2026-09-25-slate-temporal-candidate/low-no-normal-mips-material-2.png). El [proof](2026-09-25-slate-temporal-candidate/slate-temporal-proof.json) da 0 misses/backfaces y el mismo hash high/low que el control publicado; los diez PNG base del bake, incluidos normal, AO, cavity y edge, también son idénticos por SHA-256 al paquete LOD2 ya retenido de pizarra. El bake CPU fue 7,49 s en una corrida. Los mips de normal añadieron 5.505.024 bytes al buffer estimado del preview, sin medición GPU. El [audit de UV/mapas](2026-09-25-slate-temporal-uv-audit.json) tuvo 0 muestras interiores sin pintar, 0 IDs inventados y 0 desajustes de alpha especialista. Dos corridas del mismo binario coincidieron en [74 artefactos](2026-09-25-slate-temporal-replay-audit.json); Release build y CTest 17/17 pasaron. Sólo se conserva una selección de las 30 capturas y los metadatos en Git; las secuencias completas siguen en `Evidence/` local.

El próximo cambio de filtrado requiere una referencia temporal/material más representativa y una métrica que no premie sólo suavizar; la pizarra continúa demasiado regular en sus bandas laterales. JericoEngine permaneció en lectura, sin Blender ni OpenVDB.
