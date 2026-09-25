# Bakes temporales de cliff y estratos — 2026-09-25

Estado: **PASS del diagnóstico reproducible; PARTIAL para filtrado y Beauty finales**. `--cliff-temporal-bake-test` y `--strata-temporal-bake-test` cargan las recetas publicadas del cliff firmado v17 y de horizontes fracturados v15. Regeneran sus high y LOD2 low, exigen los hashes de geometría retenidos, proyectan de nuevo normal, AO, contacto, curvatura, cavity, convex edge, crease, thickness, region ID y height a UV automática 2048² con cuatro niveles, y comprueban los diez PNG base byte a byte contra los paquetes previos. Ambos bakes dieron cero misses/backfaces. JericoEngine permaneció en lectura; no se usaron Blender ni OpenVDB.

Para cada familia se capturaron cinco vistas Vulkan 1280×960 con yaw −0,001 a +0,001 radianes: high, low con cuatro mips y el mismo low sin mips **sólo de normal**. Material, luz, geometría y mapas especialistas son idénticos dentro de cada pareja. Cliff usa distancia 20 y estratos 11,6, como sus controles lejanos; sus puntuaciones no se mezclan entre familias. [`RockTemporalAudit.py`](../Tests/RockTemporalAudit.py) excluye dos píxeles de borde, mide ángulo de normales del renderer en grados y diferencias RGB del material en valores 0–255, con media/p95/p99, cambio temporal, residuo de cambio frente al high y retención del gradiente normal entre píxeles vecinos. El RED anterior carecía del contrato de captura `rock-temporal-proof.json`.

| Familia / medida media | Low con mips | Low sin mips de normal |
| --- | ---: | ---: |
| Cliff: ángulo espacial normal high↔low | 0,756° | 0,358° |
| Cliff: p95 espacial normal | 2,295° | 1,064° |
| Cliff: cambio temporal normal | 0,136° | 0,140° |
| Cliff: residuo temporal normal | 0,141° | 0,129° |
| Cliff: retención de detalle normal vecino | 0,998 | 1,007 |
| Estratos: ángulo espacial normal high↔low | 2,546° | 1,389° |
| Estratos: p95 espacial normal | 8,944° | 5,479° |
| Estratos: cambio temporal normal | 0,107° | 0,136° |
| Estratos: residuo temporal normal | 0,163° | 0,163° |
| Estratos: retención de detalle normal vecino | 0,839 | 0,996 |

Los mips reducen levemente el cambio temporal aislado, pero empeoran la fidelidad espacial y en estratos eliminan ~16 % del detalle normal vecino. El residuo temporal respecto del high no mejora de forma útil; las [métricas completas de cliff](2026-09-25-cliff-temporal-audit.json) y [estratos](2026-09-25-strata-temporal-audit.json) incluyen las colas p95/p99 y el material. **No se modificó el filtro ni se declaró una mejora visual.** Dos corridas del mismo binario dieron [74/74 artefactos idénticos en cliff](2026-09-25-cliff-temporal-replay-audit.json) y [74/74 en estratos](2026-09-25-strata-temporal-replay-audit.json). El build Release y CTest 17/17 pasaron.

Los bakes tardaron 4,34 s (cliff) y 9,58 s (estratos) en una corrida; no son medidas de coste por frame. Los mips añadieron 5.505.024 bytes al recuento de recursos del renderer por paquete, no memoria GPU residente medida. El high rasterizado también puede aliasar; las diferencias de cuadros en píxeles fijos incluyen movimiento de la imagen porque aún no hay reproyección. Cinco pasos diminutos, una luz y estas distancias no prueban estabilidad temporal representativa. El siguiente ensayo de filtrado necesita reproyección o correspondencia de superficie y evaluar conservación de detalle/material junto a la estabilidad, antes de modificar normal o roughness.

Capturas centrales a resolución original: [cliff high](2026-09-25-cliff-temporal-candidate/high-material-2.png), [cliff low con mips](2026-09-25-cliff-temporal-candidate/low-mips-material-2.png), [estratos high](2026-09-25-strata-temporal-candidate/high-material-2.png) y [estratos low con mips](2026-09-25-strata-temporal-candidate/low-mips-material-2.png). Ambas formas siguen necesitando mejor macrogeología y una escena Beauty representativa; estos encuadres lejanos sólo son pruebas de bake.
