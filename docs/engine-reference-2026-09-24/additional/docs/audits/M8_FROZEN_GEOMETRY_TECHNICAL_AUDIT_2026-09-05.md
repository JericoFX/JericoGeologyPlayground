# Auditoría técnica de M8 congelado

Fecha: 2026-09-05. Base Git: `a219db9f49f489a425611023799f2261a18471b7`,
sin divergencia con origin/main. Auditoría de la implementación y evidencia
existentes; no se compila, genera, ajusta, cambia código ni ejecuta el motor.
Dos auxiliares READ_ONLY revisaron generador y meshing/LOD; Astra contrastó las
conclusiones con el código, las imágenes originales y los logs conservados.

**Decisión: mantener M8 congelado, experimental e incompleto. Descartar la
continuación basada únicamente en afinar la receta actual. No descartar la
representación volumétrica ni el trabajo reutilizable.**

## 1. Respuesta a la pregunta de viabilidad

| Pregunta | Dictamen y alcance |
| --- | --- |
| ¿La receta actual ya alcanza la referencia? | **No.** Las imágenes muestran una masa demasiado prismática y estratos regulares; la referencia requiere bloques, fracturas y repisas que participen en la geometría. |
| ¿Se resuelve solamente con más subdivisión, semillas, amplitud y desgaste? | **No como solución al objetivo completo.** Esos controles no agregan a esta ruta los grados de libertad ausentes: juntas independientes, separación/desplazamiento de bloques y variación de losas por región. Pueden producir una roca estratificada distinta, no constituyen una capacidad demostrada de modelar esos rasgos. |
| ¿SDF/campos implícitos son matemáticamente incapaces de esas formas? | **No.** Admiten sólidos con aristas, huecos y operaciones booleanas. La imposibilidad general no está sustentada. |
| ¿La implementación completa de Jerico puede alcanzar la calidad final de Hunt? | **No determinado ni garantizado por esta auditoría.** Representabilidad geométrica no equivale a generador integrado, materiales, iluminación, autoría y presupuesto de producción demostrados. |
| ¿El LOD explica la mala forma de las fotos actuales? | **No.** El host de las fotos cocina `denseMaster`, sin solicitar LOD. El fallo de LOD es adicional. |
| ¿Qué cambio de enfoque se justifica? | Construcción estructural explícita de la masa y sus fracturas, con un contrato de conservación geométrica/semántica, antes del desgaste y del detalle. Evaluar la reutilización de operadores existentes; no iniciar otra ronda de parámetros ni reemplazar todo el motor. |

El «no» a seguir afinando es un dictamen de capacidad y de ingeniería sobre esta
ruta acotada. No pretende demostrar que ninguna combinación imaginable de sus
parámetros pueda producir accidentalmente una imagen parecida. El objetivo exige
autoría controlada y resultados reproducibles, no una coincidencia visual.

## 2. Referencia y evidencia que se compararon

La especificación exige cuatro miembros independientes, masa no prismática,
estructura reconocible con Detail desactivado, tres siluetas útiles por miembro,
material cocinado y colisión exacta. Véase
[contrato de producto](../superpowers/specs/2026-09-01-m8-rock-family-final-shaded-visual-quality-repair-design.md).

Las siete capturas del usuario y sus hashes están en
[referencias Hunt](../evidence/M8_F4_HUNT_SHOWDOWN_VISUAL_REFERENCES.md).
Se toman como referencia de modelos 3D: bloques y planos grandes, juntas verticales
y oblicuas, repisas de espesor desigual y roturas que interrumpen los estratos.
No se deduce de una captura la implementación interna de CryEngine, el número de
polígonos, la escala métrica de cada rasgo ni su coste. Una pared del escenario
tampoco equivale a una única roca aislada; se comparan rasgos geométricos, no FPS
ni densidad de polígonos de productos diferentes.

Se inspeccionaron a resolución original las imágenes conservadas de 1280×720:
[control](../evidence/images/m8-f4-post-cut-weathering-01/control.bmp) y
[desgastado](../evidence/images/m8-f4-post-cut-weathering-01/weathered.bmp).
El segundo redondea bordes y escalones; conserva la masa regular del primero.
La iluminación y el material marrón afectan la percepción, pero no explican por
sí solos la falta de estructura grande en la silueta.

El host usa `denseMaster` en
[Generation.hpp:48](../../Tools/GeologyPreview/Source/Generation.hpp#L48).
Por tanto reparar LOD o acelerar shaders no vuelve convincente esa malla densa.

## 3. La ruta real y su techo estructural

La fuente auditada es
[F4WorkingWeatheringControl.geology.toml](../../Tests/ProceduralAuthoring/Fixtures/F4WorkingWeatheringControl.geology.toml).
No se confunde con una lista de todos los operadores que existen en el engine.

```text
BlockGrammar, ocho celdas ocupadas -> caja fuente -> subdivisión lineal 16
-> ImplicitBlock irregular -> Strata -> dos cortes planos
-> DurabilityField -> cuatro cortes planos
-> [desgaste convexo añadido por el test para el candidato] -> Rebuild
-> campo final -> extracción -> regiones/normales -> cook/colisión/material
```

**Fuente y subdivisión.** La gramática actual tiene profundidad 1 y las ocho reglas
hijas ocupadas; su cobertura greedy produce una caja. `Refine(16)` densifica sus
triángulos sin crear otra forma: 12×16²=3072 triángulos fuente. Más triángulos en
la misma superficie no crean juntas ni relieve. Fuentes:
[BlockGrammar:264/393](../../Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyBlockGrammarSource.hpp#L264),
[materialización:613](../../Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuTopologyPlan.cpp#L613),
[Refine:1606](../../Tools/ProceduralAuthoring/Source/SolidEvaluatorManifold.cpp#L1606).
Cambiar ocupación/reglas podría cambiar la estructura; eso debe evaluarse como
composición estructural, no atribuirlo a la subdivisión o al ruido.

**Masa y cortes.** `IrregularPolyhedral` parte de una envolvente de planos con
variación de offsets; la receta usa una sola masa. Cada PlaneCut conserva el lado
negativo de un plano y descarta el positivo. No conserva dos fragmentos ni los
separa. Nombres como «fracture» en la receta no convierten un recorte por un
semiespacio en una red de fracturas. Fuentes:
[ImplicitBlock:144](../../Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyImplicitBlock.hpp#L144),
[cortes:212](../../Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyWorkingRegionReference.cpp#L212).

**Restricción de integración decisiva.**
[validateWorkingRegionOperators:141](../../Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyWorkingRegionReference.cpp#L141)
admite un ImplicitBlock, un Strata de forma antes del primer corte, cortes exactos
sin rugosidad y, en la variante habilitada, un único desgaste convexo terminal.
Restringe máscaras, transformaciones y fuerza parcial. Otros operadores de
juntas/fracturas existentes no quedan admitidos por tener código o pruebas
aisladas. El propio diagnóstico exige implementar su correspondencia de regiones.
**Hay un límite real de composición y procedencia, no un slider desconocido.**

**Strata cambia forma, pero no modela bloques independientes.** El shader usa:

```text
phase(p) = frac((axis·p + warp(p))/spacing + phaseOffset)
F_p(d) = d + k(p) max(1 − |d|/B, 0)
k(p) = groove(p) − shape(p)
|F_p(d) − d| ≤ |shapeAmplitude| + |grooveAmplitude|
```

En la receta, el límite de cambio del valor de campo es 0.045+0.015=0.060m.
No es una garantía de desplazamiento euclídeo de la superficie: después de estas
operaciones el campo no tiene necesariamente gradiente unitario. El perfil
periódico y el warp desplazan/curvan bandas, pero no proporcionan identidad y
movimiento independiente para losas ni una red de juntas controlable.

La condición `|shape|+|groove| < B(1−10⁻⁵)` preserva el orden de distancias para
cada punto fijo. **No prueba convexidad, monotonicidad espacial ni conservación
topológica de la forma deformada.** No se usa ese argumento para declarar una
imposibilidad que no demuestra. Fuentes:
[shader:2777–2820](../../Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuShaders.hpp#L2777),
[prueba previa y límites](../evidence/M8_F4_STRATA_PHASE_WARP.md).

**El desgaste actual no es un generador de fracturas.** Su retirada es proporcional
a exposición, respuesta convexa y `(1−durabilidad)`. En una región idealmente
plana, con normales constantes en el stencil, la divergencia de normales es cero:
la respuesta de curvatura y el desgaste son cero. Aumentar iteraciones erosiona
zonas que responden; no introduce un mecanismo de propagación de juntas o de
desplazamiento de fragmentos. El muestreo y los bordes pueden alterar ese caso
ideal, por lo que no se promete que toda cara del resultado permanezca intacta.
Fuente: [shader:2951–3023](../../Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuShaders.hpp#L2951).

Corrección importante: aunque el test pone `durability=0`, existe DurabilityField
antes del desgaste. El host activa el bit 32 y el shader consume ese campo; no
es correcto afirmar que ignora la dureza o que usa desgaste uniforme. Fuentes:
[host:6000](../../Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuSessionDiligent.cpp#L6000),
[selección de durabilidad:3011](../../Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuShaders.hpp#L3011).

## 4. Representación y meshing: límite concreto, no condena general

Los campos implícitos pueden representar unión, intersección y diferencia de
sólidos mediante min/max sobre funciones con convención de signo consistente.
Esto conserva la lógica de interior/exterior; no convierte automáticamente el
resultado en distancia euclídea exacta. Es una construcción posible de bloques
y huecos, no una implementación ya integrada en la ruta auditada.

Además, [Dual Contouring of Hermite Data, Ju et al.](https://www.cs.rice.edu/~jwarren/papers/dualcontour.pdf)
demuestra extracción de geometría con aristas usando intersecciones/normales
Hermite y QEF. Refuta la afirmación general «lo volumétrico sólo puede dar rocas
redondeadas». No prueba que los datos ni el solver actuales tengan esa fidelidad.

La ruta actual coloca un vértice por celda con cambio de signo, usa sus ocho
esquinas y une vértices de celdas vecinas. Con Strata o desgaste, toma cruces
interpolados del campo muestreado y normales trilineales; no las intersecciones
analíticas exactas de la fuente original. La rama analítica de working Hermite
puede rechazar cruces múltiples, pero ese rechazo **no certifica** la rama
muestreada actual. Fuentes:
[shader:3821–3946](../../Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuShaders.hpp#L3821),
[selección de rama:7907](../../Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuSessionDiligent.cpp#L7907),
[QEF:82](../../Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuHermiteFit.hpp#L82).

Hay un límite matemático demostrable a resolución fija: una esfera centrada dentro
de una celda, con radio menor que h/2, puede tener las ocho esquinas fuera. El
shader no emite superficie para esa celda aunque exista una superficie interior.
Análogamente, dos cruces entre muestras pueden quedar invisibles al criterio de
signo en los extremos. Un vértice por celda tampoco representa cualquier número
de láminas independientes dentro de ella. Subdividir después la malla no recupera
la información descartada. Eso exige otra resolución o representación de los
rasgos, no más ajustes del QEF a los mismos datos incompletos.

| Rasgo de esta receta | Tamaño nominal | Frente a h=6cm / LOD h=12cm |
| --- | --- | --- |
| Amplitud de forma Strata | 4.5cm | 0.75 / 0.375 celdas |
| Profundidad del surco | 1.5cm | 0.25 / 0.125 celdas |
| Soporte total del surco antes del warp | 2×0.1×0.5=10cm | 1.67 / 0.83 celdas |

Son señales concretas de submuestreo; no un teorema de que ese surco específico
desaparece ni de que un rasgo necesita exactamente dos celdas. El warp cambia
las distancias locales. Tampoco explican la falta de grandes bloques en la masa.

## 5. LOD y normales: qué está probado y qué sigue abierto

El LOD real pide 12 cm, 3000 triángulos y desviación espacial máxima 12 cm; el límite
angular sigue en 35°. El log conservado `f4-lod-region-corrected01.log:389` rechaza
112.727115°, con vértices 691/2526 separados 0.021902 m y la misma región
`E2178D96A95A2C403CBCFF8B9B720ACC`. Evidencia:
[métrica y pruebas](../evidence/M8_F4_LOD_REGION_METRIC.md).

La corrección de la métrica eliminó comparaciones entre regiones distintas.
Sigue buscando el **vértice** fuente más cercano, no el punto más cercano sobre
un triángulo con una correspondencia geométrica adecuada. Una región semántica
tampoco equivale a una superficie de normal constante. Por ello el número prueba
el incumplimiento de la aceptación actual, pero no aísla cuánto proviene de
geometría gruesa, normales o correspondencia. No se baja ni sube el umbral para
resolver esa incertidumbre. Fuente:
[GeologyTopologyMetrics:49](../../Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyTopologyMetrics.hpp#L49).

La finalización reemplaza normales del campo por sumas de productos cruzados de
caras, normalizadas al final: **ya pondera por área**. La hipótesis anterior de
pesos iguales para triángulos diminutos queda descartada. Cambiar triangulación
y vecindad aún puede cambiar las normales. La asignación de región por centroide
más cercano tampoco es transporte exacto de fronteras. Fuentes:
[finalizador:93](../../Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyWorkingRegionReference.cpp#L93),
[proyección:516](../../Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuSessionDiligent.cpp#L516).

**Pendiente preciso:** conservar/inspeccionar el LOD rechazado y su vecindad antes
y después de finalizar; comparar normales geométricas y correspondencia sobre
triángulos. Los logs actuales sólo conservan el par extremo. La auditoría no
ejecutó esa generación y no inventa una causa exacta para los 112.7°.

## 6. Qué se conserva y por qué no se reanuda el ajuste

La evidencia conservada demuestra partes útiles: fuente/receta reproducibles,
campo clasificado, región de los seis cortes, cook, colisión exacta y exportación
estructural acotada. La mejora de exportación retiene 20150 muestras a 6 cm desde
512000 residentes, sin bajar resolución. Véase
[cobertura estructural](../evidence/M8_F4_STRUCTURAL_EXPORT_COVERAGE.md).
Eso no certifica naturalidad, cuatro productos, LOD/streaming ni presupuesto final.

Auditoría de linaje: la forma debe existir en fuente/operadores; el campo debe
retenerla; el mesher debe extraerla; regiones y clasificaciones deben sobrevivir;
render/material y colisión deben consumir el mismo resultado; LOD debe conservar
los rasgos requeridos. Al ampliar fracturas se debe resolver su procedencia hacia
materiales, física, RBrush, cook, destrucción y consultas. No se autoriza eliminar
semántica para admitir otro operador. UI final sigue M10.5; gameplay, navegación,
streaming y otros consumidores permanecen sujetos a sus gates existentes.

La espera de 90.4418 s medida corresponde a preparación de sesión/shaders/pipelines,
no a una demostración de 90 s de generación GPU. La caché experimental sigue sin
validar y no se probó otra vez. Es un defecto de coste/uso independiente de la
capacidad de formar fracturas; repararlo no debe sustituir la decisión geométrica.
Véase [estado de caché](../evidence/M8_F4_SHADER_CACHE_WIP.md).

## 7. Decisión previa a cualquier implementación nueva

Recomendación: sustituir la estrategia «una masa + bandas + recortes + desgaste»
como generador suficiente de la referencia por una composición estructural
controlable de bloques/juntas/repisas. Primero evaluar los operadores geométricos
ya existentes y su integración de regiones. Conservar el field/cook/runtime
cuando cumplan; elegir representación por rasgo. No hay justificación para
descartar todo SDF, importar otro engine o añadir dependencias en este punto.

Antes de autorizar trabajo, el diseño propuesto debe explicar cómo expresa una
junta localizada, un bloque desplazado y una repisa de espesor variable, qué
representación conserva sus caras y qué semántica llega a los consumidores.
No basta con nombrar un algoritmo o mostrar una función aislada.

Si se autoriza una prueba posterior, su condición de parada debe ser explícita:

1. Un único testigo estructural acordado, con identidad de esos rasgos y material
   neutro. Comparar geometría antes del field y después de extraer, sin afinar
   semillas para ocultar pérdidas. Si no se puede expresar la fuente, falla el
   diseño del generador; si se pierde al muestrear, falla esa representación.
2. Verificar el mismo testigo en LOD con correspondencia sobre superficie y
   conservación de rasgos, junto a límites espaciales/topológicos. Si falla,
   localizar la etapa antes de cambiar presupuesto o umbral.
3. Sólo con esa capacidad demostrada, volver a los miembros reales y sus vistas
   finales. Un testigo técnico no sustituye el contrato de las cuatro rocas.

Estas son condiciones propuestas, **no pruebas ejecutadas ni permiso para
reanudar**. No se propone otra búsqueda abierta de parámetros. El resultado de
esta auditoría es el límite identificado y una decisión justificable, no un PASS
del engine ni una promesa de calidad AAA.

## Estado de preservación

Sólo se añadió este informe. El checkout conserva las 193 entradas previas de
`git status --short`, cuyo inventario UTF8/LF sin salto final tenía SHA256
`14e32188c2c563e61006ea6835d989c59a6a1e4a2760b9a963083d372fbae489`.
Incluye el WIP de caché y cambios ajenos, que no fueron corregidos, descartados
ni incluidos en la auditoría como implementación aceptada. M8 permanece congelado.
