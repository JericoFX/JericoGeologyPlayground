# Goal activo — JericoGeologyPlayground

Desarrollar y verificar en este playground las matemáticas nativas C++ de geología, con prioridad en bakes high→low, mapas de superficie y calidad visual de rocas, cliffs y estratos. Investigar y probar compatibilidad matemática con RBrush leyendo `F:\JericoEngine`, sin modificar ese checkout ni ejecutar Blender. Conservar pruebas, capturas, métricas, procedencia de referencias y un inventario honesto de lo demostrado y lo pendiente para la futura integración M8 en JericoEngine.

## Referencias durables del Goal

Las copias de los documentos del Engine que corresponden a este trabajo están en [`docs/engine-reference-2026-09-24/`](docs/engine-reference-2026-09-24/README.md). [`SOURCE_MANIFEST.json`](docs/engine-reference-2026-09-24/SOURCE_MANIFEST.json) registra para cada copia la ruta original, la ruta local, los bytes y el SHA-256 verificado. Incluye la autoridad M8, el blueprint y matemáticas SDF/geología, ZenUV, SimpleBake, Ucupaint, remesh, segmentación, daño, fractura, diseños de rocas y RBrush, y evidencia matemática de estratos/LOD/coluvión/rockfall. Son referencias documentales congeladas: no sustituyen la autoridad actual del Engine ni implican que la capacidad esté implementada o aceptada.

**Asignación pedida por el owner:** ZenUV guía las matemáticas de UV automática (seams, charts, unwrap, packing y densidad); Ucupaint guía la generación de bakes/mapas y máscaras de superficie; SimpleBake aporta además el flujo de proyección high→low, cage y empaquetado. Todo debe implementarse y comprobarse en C++ nativo.

La instrucción vigente del owner es reconstruir y probar la matemática en C++ nativo. No ejecutar Blender ni copiar código, assets o node graphs de los addons. No escribir en `F:\JericoEngine`. El M8 del Engine decidirá la integración después de pruebas suficientes en el playground.

## Estado verificable

El detalle de código, bakes, imágenes, pruebas y limitaciones está en [`Evidence/2026-09-24-geology-math-checkpoint.md`](Evidence/2026-09-24-geology-math-checkpoint.md). El Goal permanece **ACTIVE/PARTIAL**: UV automática y cuatro niveles de los diez mapas se probaron en seis familias y múltiples LOD high→low. El loop local del cliff tiene región seleccionable por GUID, UV separado y tres LOD con bake sin misses; el candidato de warp vectorial mejora el contorno, pero su Beauty sigue pendiente. El arco y túnel BSP tienen tres LOD y bakes standalone; eso no resuelve el BSP wall LOD2 previo ni equivale a una caverna natural. Faltan filtros/materiales productivos, mejor macroforma y la integración M8.

La [prueba de proyección triplanar](Evidence/2026-09-25-triplanar-projection.md) redujo el facetado de material del cliff LOD0/1/2 y verificó regresiones visuales de arco, túnel y estratos. Una repetición con el mismo binario confirmó [171 artefactos de bake idénticos](Evidence/2026-09-25-layered-repeat-audit.json) en estratos; la comparación antigua cruzaba versiones distintas del algoritmo de cobertura. Las siluetas y la calidad geológica general siguen pendientes.

El [arco cavernoso con macrodeformación XYZ](Evidence/2026-09-25-cave-geology-macro.md) es un experimento opt-in con tres LOD, seis bakes válidos entre arco y control cuadrado, y vistas frontales/oblicuas. Cambia volumen y silueta, pero aún parece un bloque perforado; no se acepta como caverna natural.

El [ensayo de boca-garganta-cámara](Evidence/2026-09-25-cave-profile-rejected.md) quedó rechazado: los cortes BSP solapados crearon una cara degenerada en LOD2 y el perfil axial suave perdió la garganta/cámara en los LOD bajos, con hasta 65.874 misses de proyección. Ese intento mostró que las secciones axiales deben sobrevivir en cada LOD antes del bake.

La [variante axial con reducción desde la macroforma high](Evidence/2026-09-25-cave-axial-lods.md) resolvió ese fallo técnico: tres LOD decrecientes conservan boca/garganta/interior, seis paquetes de bakes tienen cero misses y 266 artefactos de dos corridas con el mismo binario coinciden. Es opt-in; la Beauty sigue mostrando un bloque perforado, por lo que aún falta una caverna natural.

Las [vistas cercanas del arco y el túnel cuadrado](Evidence/2026-09-25-cave-entrance-view.md) muestran la sección interior high/LOD2 con el mismo material y los seis bakes previos válidos; 270 PNG/OBJ coinciden entre dos corridas. El arco sigue sin cámara profunda ni iluminación interior y permanece **PARTIAL** como caverna natural.

El [perfil opt-in v14 de borde erosionado del cliff](Evidence/2026-09-25-cliff-eroded-rim.md), inspirado sólo en conceptos de ruido del PDF `CascadesDemoSecrets`, retrae la malla según los ejes locales, conserva selección/UV y pasó tres LOD con bakes sin misses y 171 artefactos repetibles. La captura original muestra un borde menos recto, pero todavía una pared recortada y una hendidura suave; la Beauty del cliff continúa **PARTIAL**.

El [perfil opt-in v17 de borde firmado](Evidence/2026-09-25-cliff-signed-rim.md) añade salientes y retrocesos sin plegar las caras. Tres LOD tienen UV automática, mapas y bakes sin misses; 171 artefactos se repitieron y el control v14 conservó sus 161 PNG/OBJ. Arco y túnel cuadrado se regeneraron con el mismo binario y conservaron seis bakes válidos y 270 capturas/meshes idénticos. La silueta del cliff mejoró, pero sigue siendo una pared aislada; las entradas cavernosas siguen sin cámara profunda ni iluminación interior. Todo permanece **PARTIAL** en Beauty.

El [perfil opt-in v15 de horizontes fracturados](Evidence/2026-09-25-layered-fractured-horizons.md) redujo la repetición radial entre capas del preset 11 de 0,902 a 0,671 sin perder contactos exactos. Tres LOD high→low con material automático, UV y mapas pasaron sin misses; 172 artefactos fueron idénticos en replay. Sigue **PARTIAL** en Beauty: todavía se percibe un bloque estratificado con un saliente lateral. Este perfil no es el default del preset ni está en el Engine.

El [preset experimental v16 de fuente tabular](Evidence/2026-09-25-layered-slate-source.md) cambió Cylinder por Cube con triangulación de cap compatible y redujo el espesor; el ancho/alto pasó de 1,28 a 2,13. Pasó contactos, cierre, tres LOD con UV y bakes sin misses y 174 artefactos reproducibles. Las tres capturas originales mejoran la masa de pizarra, pero muestran bandas laterales demasiado regulares y salientes; la Beauty sigue **PARTIAL**. No reemplaza al preset 11 ni modifica el Engine.

La [prueba de ancho de banda de mips de pizarra](Evidence/2026-09-25-slate-mip-bandwidth.md) añadió una captura high a la misma cámara lejana que el low. Rechazó cinco niveles por contaminación entre charts/costo y el atlas 1024² de LOD2 por pérdida de detalle y mayor error visual high→low. La ruta vigente de cuatro niveles/2048² no cambió; aquella comparación estática motivó la prueba temporal siguiente.

El [audit de islas UV en mips](Evidence/2026-09-25-mip-island-coverage.md) amplió el verificador a arco y túnel: 15 bakes actuales de pizarra, cliff, estratos y cavernas no mostraron mezcla entre islas ni muestras del gutter en mips1–3. El candidato antiguo de cinco niveles sí falla, por lo que no se promovió. Esta cobertura no resuelve por sí sola el error angular o la Beauty high→low.

La [secuencia temporal high→low de pizarra](Evidence/2026-09-25-slate-temporal-bake.md) cubrió cinco movimientos pequeños de cámara, material y normal con/sin mips, con 74 artefactos reproducibles. Los mips bajan el cambio temporal del normal ~30 %, pero el MAE espacial frente al high empeora y el residuo temporal casi no cambia. Se mantiene **PARTIAL**: hace falta una referencia temporal más representativa antes de ajustar filtrado o roughness.

**Criterio visual pendiente del cliff:** el loop nativo rompe la continuidad de las bandas y deja una región semántica seleccionable; ZenUV la separa en charts UV dentro del mismo atlas. Las pruebas high→low/LOD y capturas originales avalan esa ruta técnica, pero la zona queda demasiado lisa y con borde visible. Comparar y corregir macroforma, detalle y material frente a las referencias reales antes de declarar aceptada la calidad visual.
