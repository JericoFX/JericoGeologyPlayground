# Arco cavernoso y túnel cuadrado: vista de la entrada — 2026-09-25

Estado: **PASS para comparación visual y seis bakes high→low; PARTIAL para caverna natural**. `--cave-axial-lods-bakes-test` conserva las dos formas BSP y añade una cámara frontal cercana (yaw 0, pitch −0,02, distancia 5, objetivo Y −0,5) al high y al LOD2 de cada una. Las cuatro capturas son Vulkan 1280×960 con el mismo material CC0 y sin alterar la geometría, los UV ni los parámetros de bake.

| Forma | High de la entrada | LOD2 de la entrada |
|---|---|---|
| Arco | [imagen](2026-09-25-cave-entrance-candidate/arch/high-entrance-material.png) | [imagen](2026-09-25-cave-entrance-candidate/arch/lod-2/low-entrance-material.png) |
| Túnel cuadrado | [imagen](2026-09-25-cave-entrance-candidate/square/high-entrance-material.png) | [imagen](2026-09-25-cave-entrance-candidate/square/lod-2/low-entrance-material.png) |

Inspección a resolución original: el arco tiene bóveda redondeada y relieve en la boca; el cuadrado conserva techo plano y paredes rectas. En ambos, la abertura atraviesa el bloque y muestra fondo negro. El arco aún se lee como masa perforada: **no hay cámara profunda, ramificación ni iluminación interior**. El túnel cuadrado es un control constructivo, no una forma geológica natural. Las imágenes tampoco prueban navegación o colisión en JericoEngine.

La [auditoría de seis bakes](2026-09-25-cave-entrance-audit.json) confirmó tres LOD decrecientes por forma, error high↔low ≤0,1081 m para el arco y ≤0,0193 m para el cuadrado, cero misses/backfaces, UV automáticos sin interiores sin pintar, sin IDs de región fabricados y alpha coherente de normal, AO, contacto, cavity, edge y demás mapas especialistas y mips. Los hashes high y low coinciden con el [paquete axial previo](2026-09-25-cave-axial-candidate/cave-forms-proof.json); este cambio sólo añade encuadres. Dos corridas del mismo binario dieron [270 PNG/OBJ idénticos](2026-09-25-cave-entrance-replay-audit.json). El [build Release](2026-09-25-cave-entrance-build.log) y [CTest 17/17](2026-09-25-cave-entrance-ctest.log) pasaron. Los bakes del arco midieron 1,23–1,37 s y los del cuadrado 0,68–0,84 s por LOD en una corrida; no son una comparación de rendimiento controlada.

Siguiente capacidad geométrica: crear una cámara conectada detrás de la garganta y una vista interior iluminada, conservando sus secciones en LOD0–2 y repitiendo el gate de bake. JericoEngine siguió en lectura; no se usaron Blender ni OpenVDB.
