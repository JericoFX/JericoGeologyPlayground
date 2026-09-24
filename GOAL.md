# Goal activo — JericoGeologyPlayground

Desarrollar y verificar en este playground las matemáticas nativas C++ de geología, con prioridad en bakes high→low, mapas de superficie y calidad visual de rocas, cliffs y estratos. Investigar y probar compatibilidad matemática con RBrush leyendo `F:\JericoEngine`, sin modificar ese checkout ni ejecutar Blender. Conservar pruebas, capturas, métricas, procedencia de referencias y un inventario honesto de lo demostrado y lo pendiente para la futura integración M8 en JericoEngine.

## Referencias durables del Goal

Las copias de los documentos del Engine que corresponden a este trabajo están en [`docs/engine-reference-2026-09-24/`](docs/engine-reference-2026-09-24/README.md). [`SOURCE_MANIFEST.json`](docs/engine-reference-2026-09-24/SOURCE_MANIFEST.json) registra para cada copia la ruta original, la ruta local, los bytes y el SHA-256 verificado. Incluye la autoridad M8, el blueprint y matemáticas SDF/geología, ZenUV, SimpleBake, Ucupaint, remesh, segmentación, daño, fractura, diseños de rocas y RBrush, y evidencia matemática de estratos/LOD/coluvión/rockfall. Son referencias documentales congeladas: no sustituyen la autoridad actual del Engine ni implican que la capacidad esté implementada o aceptada.

**Asignación pedida por el owner:** ZenUV guía las matemáticas de UV automática (seams, charts, unwrap, packing y densidad); Ucupaint guía la generación de bakes/mapas y máscaras de superficie; SimpleBake aporta además el flujo de proyección high→low, cage y empaquetado. Todo debe implementarse y comprobarse en C++ nativo.

La instrucción vigente del owner es reconstruir y probar la matemática en C++ nativo. No ejecutar Blender ni copiar código, assets o node graphs de los addons. No escribir en `F:\JericoEngine`. El M8 del Engine decidirá la integración después de pruebas suficientes en el playground.

## Estado verificable

El detalle de código, bakes, imágenes, pruebas y limitaciones está en [`Evidence/2026-09-24-geology-math-checkpoint.md`](Evidence/2026-09-24-geology-math-checkpoint.md). El Goal permanece **ACTIVE/PARTIAL**: UV automática y cuatro niveles de los diez mapas se probaron en 17 bakes high→low de las seis familias/LOD. El loop local del cliff tiene región seleccionable por GUID, UV separado y tres LOD con bake sin misses; su aspecto visual sigue pendiente. BSP LOD2 continúa rechazado; faltan filtros/materiales productivos, mejor macroforma y la integración M8.

**Criterio visual pendiente del cliff:** el loop nativo rompe la continuidad de las bandas y deja una región semántica seleccionable; ZenUV la separa en charts UV dentro del mismo atlas. Las pruebas high→low/LOD y capturas originales avalan esa ruta técnica, pero la zona queda demasiado lisa y con borde visible. Comparar y corregir macroforma, detalle y material frente a las referencias reales antes de declarar aceptada la calidad visual.
