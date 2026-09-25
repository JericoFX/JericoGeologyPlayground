# Reproyección temporal de cliff y estratos — 2026-09-25

Estado: **PASS del diagnóstico con correspondencia visible; PARTIAL para filtro y Beauty**. El RED de [`RockReprojectionAudit.py`](../Tests/RockReprojectionAudit.py) rechazó los paquetes previos porque no contenían profundidad ni ID de triángulo. El renderer del playground ahora captura, sólo en los comandos temporales de rock, profundidad lineal en RGB24 (0–100 m) e índice de triángulo + 1 en RGB24, con fondo cero. Su cámara se reconstruye con el mismo yaw, pitch, target y proyección de la captura Vulkan. No se tocó JericoEngine.

La comparación sigue puntos de superficie del cuadro *i* al *i+1*. Excluye dos píxeles de silueta, exige que el triángulo más cercano conserve su ID, interpola **sólo píxeles del mismo triángulo** y exige que la profundidad proyectada y la muestreada difieran ≤0,02 m. Compara high y cada low sólo donde ambos tienen correspondencia y sus profundidades iniciales difieren ≤0,05 m. El autoensayo de cámara inmóvil forma parte del audit reproducible: conservó 78.840/78.840 píxeles elegibles del cliff, con cambio normal medio 0,0062° y color renderizado linealizado prácticamente constante. Cada transición real retuvo al menos 95 % de sus píxeles interiores elegibles.

| Familia / media | Low con mips | Low sin mips de normal |
| --- | ---: | ---: |
| Cliff: cambio normal reproyectado | 0,1176° | 0,1253° |
| Cliff: residuo medio frente al cambio high | 0,1101° | 0,1048° |
| Cliff: error normal espacial high↔low | 0,7436° | 0,3510° |
| Estratos: cambio normal reproyectado | 0,1038° | 0,1380° |
| Estratos: residuo medio frente al cambio high | 0,1302° | 0,1359° |
| Estratos: error normal espacial high↔low | 2,4268° | 1,2728° |

El cliff usó 308.397 píxeles compartidos entre cuatro transiciones y estratos 110.732. Los mips reducen el cambio angular aislado, especialmente en estratos, pero aumentan el error espacial. El residuo medio empeora levemente en cliff y mejora levemente en estratos; esa ganancia temporal no compensa por sí sola la pérdida espacial. Las [métricas completas del cliff](2026-09-25-cliff-reprojection-audit.json) y [de estratos](2026-09-25-strata-reprojection-audit.json) separan media, p95/p99, cobertura y error del color renderizado linealizado. Ese color incluye luz directa, ambiente y especular dependiente de la cámara: **no aísla el material**. Mantener el filtro actual como referencia experimental; estos datos no justifican promover un filtro nuevo ni eliminar mips sin una evaluación visual de escena.

Los [diez mapas base del cliff](2026-09-25-cliff-reprojection-map-audit.json) y [de estratos](2026-09-25-strata-reprojection-map-audit.json) coinciden byte a byte con los paquetes retenidos, con cero misses/backfaces. Las [30 capturas de material/normal antiguas por familia](2026-09-25-rock-reprojection-image-identity.log) también permanecieron idénticas tras añadir los pases. Dos corridas del mismo binario dieron [104/104 artefactos idénticos en cliff](2026-09-25-cliff-reprojection-replay-audit.json) y [104/104 en estratos](2026-09-25-strata-reprojection-replay-audit.json). El build Release y [CTest 17/17](2026-09-25-rock-reprojection-ctest.log) pasaron.

El bake tardó 4,35 s en cliff y 9,40 s en estratos en estas corridas; cada paquete diagnóstico ocupa 35,8 MB y 29,9 MB respectivamente. No hay una medida aislada del coste GPU/captura ni un presupuesto de frame para ese pase, así que no se declara rendimiento. La normal del framebuffer está cuantizada a ocho bits; el high rasterizado puede aliasar, la correspondencia high↔low sigue siendo aproximada donde cambia la geometría y cinco pasos de yaw muy pequeños con una luz no prueban Beauty temporal de producción. El próximo cambio de filtro debe compararse en una secuencia más amplia y con conservación de detalle normal/material, además de la estabilidad reproyectada.

Capturas centrales originales: [cliff high](2026-09-25-cliff-reprojection-candidate/high-material-2.png), [cliff low con mips](2026-09-25-cliff-reprojection-candidate/low-mips-material-2.png), [estratos high](2026-09-25-strata-reprojection-candidate/high-material-2.png) y [estratos low con mips](2026-09-25-strata-reprojection-candidate/low-mips-material-2.png). Las formas geológicas y su Beauty siguen **PARTIAL**.
