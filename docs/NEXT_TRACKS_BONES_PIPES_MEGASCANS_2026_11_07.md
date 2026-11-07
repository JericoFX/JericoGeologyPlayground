# Próximas líneas del Playground — huesos, sockets por spline y Megascans

**Fecha:** 2026-11-07 · **Estado:** propuesta/brainstorm, sin cambios de producción.
**Directiva del propietario:** Ocean Lab queda congelado (ni código, ni pruebas, ni aceptación).
Las tres líneas de abajo son las prioridades nuevas; la tercera es **sólo información** por ahora.
Este documento no acredita ninguna capacidad implementada.

Contexto de partida comprobado en el árbol actual:

- Animación: `fitWalkingRig` infiere 0/4/8 apoyos y `fitAxialRig` propone un cuerpo axial;
  hay boca/dentición con contactos Jolt, cadenas IK/FK y escenas `{Cube, Door, Valve, Walking, Spider}`.
- Splines: `Playground::Construction` (RMF → perfil → sweep → UV métrica → repeat → taper/rib/twist/break)
  sobre `PA::ProceduralSpline`, con daño espacial y `followSurface`.
- Pipes/sockets: `Playground::Pipes` con `Socket`/`Module`/`ConnectionCheck` y sidecar `.sockets.json`.
  Los sockets **se declaran a mano**, no se infieren de la malla.
- Superficies: `JE::GeometryAnalysis` (conectividad, componentes, geodésicas, proximidad) y
  `JE::GeometrySections`; `SurfaceFeatures`, `DamageField` y `DamageGeometry` son la autoridad compartida.
- Rocas: `RocksLab` con operaciones propias, EdgeChips, LOD/bakes y galería.

---

## Línea A — De "encontrar patas" a "reconocer el arquetipo de movimiento"

### Diagnóstico

Hoy el reconocimiento es de un solo eje: *¿cuántos apoyos tiene?* (0/4/8) y *¿es alargado?*.
Eso resuelve arañas y gusanos, pero **no distingue un bípedo de un cuadrúpedo visto de frente**,
ni entiende objetos duros articulados (puerta, armario, cajón), que no tienen "patas" sino **pares cinemáticos**.

La pregunta correcta no es "¿qué animal es?" sino **"¿qué arquitectura de movimiento admite esta geometría?"**.
Un armario y un bípedo comparten el mismo problema: hallar ejes, jerarquía y grados de libertad.

### Arquitectura propuesta: tres capas separadas

```
Hechos geométricos  ->  Hipótesis de arquetipo  ->  Rig concreto + validación física
(EVIDENCIA)             (HIPÓTESIS, con score)      (CONSUMIDOR, Jolt)
```

1. **Evidencia** (`JE::GeometryArchetypes`, C++ puro, sin renderer/Jolt/UI):
   por malla se emite un vector de hechos medidos, cada uno con su tolerancia:
   eje dominante y elongación, simetría bilateral (plano y error), número de apoyos estables,
   número de "hojas" (superficies grandes y finas), grosor de cáscara, planaridad de paneles,
   grafo de adyacencia panel-panel, huecos/cavidades, secciones estables, área por componente.
2. **Hipótesis**: un conjunto *ordenado* de arquetipos con puntuación, **contra-hipótesis y motivo de rechazo**.
   Nunca un único resultado silencioso. Si dos arquetipos empatan, se reportan ambos.
3. **Rig**: el consumidor elige la hipótesis aceptada (o la corrección manual) y construye huesos,
   restricciones y contactos esperados. La validación física es la que acepta o rechaza.

### Catálogo inicial de arquetipos

| Arquetipo | Evidencia mínima | DOF / huesos | Caso negativo que debe rechazar |
|---|---|---|---|
| `CreatureBiped` | 2 apoyos, columna vertical, simetría bilateral, cabeza sobre el torso | cadena raíz→pelvis→columna→cuello→cabeza; 2 brazos, 2 piernas IK | cuadrúpedo frontal, estatua |
| `CreatureQuadruped` | 4 apoyos, cuerpo alargado, simetría | 4 cadenas + columna | bípedo con brazos bajos |
| `CreatureArthropod` | ≥6 apoyos radiales, cuerpo central, jitter alto | N cadenas desde un cuerpo | mueble con 6 patas talladas |
| `CreatureAxial` | una sola columna, sin ramas | cadena axial + boca | pieza mecánica alargada |
| `HingedPanel` (puerta/hoja/portezuela) | panel fino grande + marco/abertura; bisagra = eje en un borde | 1 revoluta en el eje de bisagra | panel decorativo embutido |
| `Drawer` (cajón) | cara plana inscrita con borde perimetral en 3 lados | 1 prismática según normal de cara | puerta enrasada |
| `Valve` | cuerpo de revolución + eje perpendicular | 1 revoluta (volante) | tubería con brida |
| `RigidFrame` (armario/caja/estructura) | caja con ≥1 hoja hija y base de apoyo | raíz estática + hojas hijas | criatura |

`Wardrobe`/`Cabinet` no necesitan arquetipo propio: son `RigidFrame` con 1..N `HingedPanel`
(y opcionalmente `Drawer`) como hijos. Esto reutiliza el grafo de paneles en vez de duplicar reglas.

### Cómo se calculan (sin nombres de archivo)

- **Eje de bisagra:** RANSAC de eje sobre las aristas del borde compartido entre panel y marco;
  requisito: girar el panel 0–90° no debe generar contactos nuevos con su marco (reutiliza `GeometryContacts`).
  Error de eje medido en mm, no "parece una puerta".
- **Cajón:** detectar el anillo de borde que rodea la cara plana en los 3 lados no accesibles;
  traslación 0..recorrido sin penetrar el marco; carrera máxima medida, no supuesta.
- **Bípedo:** la clave es **separar apoyos de brazos** — hoy ambos son "extremidades".
  Regla propuesta: un apoyo toca el plano de suelo de la malla (o su envolvente inferior) y su extremo
  está por debajo del centro de masa; un brazo cuelga lateralmente sin contacto con ese plano.
  Si hay 4 cadenas y sólo 2 tocan suelo → bípedo, no cuadríptero.
- **Contra-hipótesis obligatoria:** cada aceptación registra qué arquetipo competía y por qué perdió.
  Esto es lo que faltó en el falso positivo de `blind-10` (pieza mecánica con hipótesis bucal).

### Persistencia y UI

- Receta `RigHypothesis v1`: hechos medidos (con hashes de malla), hipótesis ordenadas, aceptada,
  ediciones manuales y su fecha. Sin nombres de archivo dentro del algoritmo; el nombre vive sólo en la UI.
- La corrección manual **gana siempre** y se conserva al re-analizar (si la malla no cambió).
- Barra nueva en Animation Lab: "Arquetipo detectado: … (confianza) · alternativas: …".

### Gates de aceptación propuestos

1. Matriz de confusión sobre corpus etiquetado por el propietario (sin que el algoritmo lea etiquetas):
   bípedo, cuadrúpedo, araña, gusano, puerta, cajón, armario, pieza mecánica (negativo).
2. Error de eje de bisagra ≤ 2 mm y barrido 0–90° sin contactos nuevos.
3. Cajón: recorrido máximo medido y sin penetración en todo el rango.
4. Bípedo: 2 apoyos estables, reposo sin deriva, IK/FK continuo (reutiliza las pruebas existentes).
5. Persistencia round-trip y Undo/Redo de la corrección manual.
6. Reproducción determinista byte a byte de la evidencia nativa.

### Portabilidad al Engine

El clasificador y los hechos son `JE::` puro (sin Diligent, sin ImGui, sin Jolt).
El adaptador Jolt y la UI quedan en `Playground::`. Al integrarse en M8, el Engine recibe
"clasificación + receta", no un algoritmo que dependa del renderer.

---

## Línea B — Sockets y tuberías dirigidos por spline, automáticos y rompibles

### Diagnóstico

Hoy `SplineConstruction` sabe barrer un camino y `PipeSockets` sabe *validar* una conexión entre
sockets declarados a mano. Falta el puente: **del trazo al conjunto montado**, y **del hueco de la malla al socket**.

### B1. Compilador de red (camino → conjunto)

```
spline (autoridad PA::ProceduralSpline)
   ↓ muestreo + RMF
PathGraph (tramos + nodos de rama)
   ↓ resolver de uniones
catálogo: codo / te / cruz / reducción / brida / válvula / tapa
   ↓ validación
sweep + colocación + UV métrica + IDs estables
```

Reglas explícitas (las que hacen que "automático" no sea "aleatorio"):

- **Ángulo de unión** → familia de fitting; si no hay fitting compatible en el catálogo, se falla con motivo.
- **Longitud recta mínima** antes/después de un fitting (regla de taller, configurable) y colisión con BVH existente.
- **Diámetro** del tramo manda el fitting; reducciones sólo si el usuario las autoriza.
- **IDs estables** por tramo, por fitting y por tramo-de-tubería entre fittings (no por teselación).
- **Replay determinista**: mismo spline + mismas reglas ⇒ mismo ensamblaje byte a byte.

### B2. Sockets inferidos de la malla (lo que falta hoy)

Inverso del sidecar actual: dada una malla, proponer sockets.

- Abertura real (malla abierta) → `boundary loop` → ajuste de círculo (centro, radio, normal) → `Socket{diameter, position, axis}`.
- Tapa circular (malla cerrada) → detección de disco plano en la superficie → mismo ajuste.
- Rol propuesto por contexto: `Connection` si el diámetro cae en la serie nominal; `StructuralAnchor` si está en la base.
- Siempre **propuesta**: el usuario confirma, corrige o descarta; el sidecar guarda sólo lo confirmado
  y las propuestas rechazadas (para no repetir el error).

### B3. Acoplamiento automático (que "los mesh se acoplen")

Dados socket A y socket B, generar el sweep que **nace y muere exactamente en los frames de los sockets**:

- el tangente del primer y último frame se fija al eje del socket (no al del spline dibujado);
- se resuelve un pequeño problema de ajuste (última recta + codo) y se reporta el **residuo en mm**
  de posición y ángulo; por encima de tolerancia se rechaza, no se "aproxima";
- si el tramo dibujado ya era válido, no se toca (el trazo del autor es la guía canónica — misma filosofía que `followSurface`).

### B4. Válvulas y roturas, todo seleccionable

- **Válvula**: componente paramétrico insertado *en línea* sobre un par de sockets (`in`/`out`),
  con ID propio y paso de flujo declarado. Reutiliza el catálogo de fittings, no un backend nuevo.
- **Rotura**: `DamageGeometry`/`DamagePathCutter` existentes (nada de un cortador paralelo):
  gap + borde irregular + interior fresco + UV, dos fragmentos cerrados, como ya se hace en assemblies.
  El estado (intacta / rota / válvula abierta-cerrada) es dato de la receta, no del renderer.
- **Selección**: por ID, rol, índice y distancia (ya existe en assemblies); arrastrar un fitting debe
  re-resolver los tramos vecinos sin tocar los no afectados.
- **Undo/Redo** de cada colocación y de cada rotura, como una operación, no como N.

### B5. Gates

1. Trazar spline → red generada → 0 colisiones y 0 violaciones de longitud mínima en el corpus de presets.
2. Acoplamiento socket→socket con residuo ≤ 1 mm y ≤ 0.5° (declarado, no promediado).
3. Válvula insertada/quitada sin cambiar los IDs de los tramos vecinos.
4. Rotura: pérdida de volumen, dos sólidos cerrados, interior fresco, replay exacto, resto de la red intacto.
5. Persistencia v+1 con lectura de las versiones anteriores.
6. Capturas originales en la ventana normal (no sólo fixture).

### Riesgo a vigilar

Que el compilador se convierta en **segunda autoridad de spline**. Debe seguir siendo consumidor:
produce peticiones de sweep sobre `PA::ProceduralSpline`, nunca su propia curva.

---

## Línea C — Megascans / Fab: qué se puede hacer y qué no (sólo información)

### Hechos verificados (2026-11-06)

- Epic/Quixel liberó **toda** la biblioteca Megascans en Fab, esta vez **permanente**
  (la de 2024 fue temporal). Las fichas se actualizan hasta el **12 de noviembre de 2026**.
- Los términos de **todos** los assets Quixel se actualizaron para **permitir entrenar modelos de IA**
  con Megascans y Megaplants.
- Compras desde el **1 de enero de 2025**: reembolso al saldo de Epic; se conserva licencia y se suman los derechos de IA.
- Fuentes: [Quixel](https://quixel.com/news/update-megascans-are-going-free-again) ·
  [80.lv](https://80.lv/articles/entire-quixel-megascans-library-now-available-for-free) ·
  [The Rookies](https://www.therookies.co/blog/headlines/quixel-megascans-library-now-free-on-fab-platform-again)

### Lo que esto habilita, y lo que no

**Habilita:** usar los escaneos como *supervisión* de los generadores nativos (priors medidos),
y entrenar modelos fuera del núcleo portable.

**No habilita:** redistribuir los escaneos crudos. Este repositorio es **público**
(`JericoFX/JericoGeologyPlayground`, verificado por API). Por tanto:

- Los FBX/EXR/TIFF de Megascans **no se commitean**. Igual que `docs/engine-reference-*`,
  se guarda un **manifiesto** (id, ruta local, bytes, SHA-256, licencia, fecha de descarga),
  y un script de descarga que exige cuenta de Epic.
- Las capturas de evidencia pueden mostrar el asset transformado; el asset fuente no viaja en el repo.

### Ruta de integración propuesta (3 niveles, del más barato al más caro)

**Nivel 0 — Materiales.** Megascans trae albedo/normal/roughness/AO/displacement 8K en PBR estándar.
El importador ya existente (`PipeMaterial` + `NormalBake` + `AutoUv`) puede consumirlos sin cambios
de arquitectura: sólo un conversor de metadatos Fab → struct local. Sirve para rocas, muros y pasarelas
sin tocar geometría.

**Nivel 1 — Priors geométricos (el más interesante para este repo).**
En vez de "entrenar la IA", **medir** el escaneo y usar la medida como parámetro canónico:

1. Analizar el escaneo con `JE::GeometryAnalysis` + `SurfaceFeatures`:
   ángulo de estratos, orientación de fracturas, densidad de cavidades, distribución de tamaños,
   aspecto de silueta, simetría, curvatura por banda.
2. Extraer un `ScanPrior` (JSON) por familia: roca, risco, muro, losa, pasarela.
3. Los generadores nativos (`RocksLab`, `LayerFormation`, muros) consumen el prior como
   **parámetros por defecto con semilla**; el resultado sigue siendo procedural, barato y determinista.
4. Comparar el procedural contra el escaneo: silueta, histograma de normales, cobertura de cavidades.
   Ese es el gate honesto — no "parece una roca".

**Nivel 2 — High→low real.** Escaneo como *high*, malla reducida/procedural como *low*:
el pipeline `MeshReduction` + `NormalBake` ya existe y tiene métricas. Ahí sí se puede usar
un escaneo heroico para una roca concreta y hornear el detalle en la versión de juego.

### Dónde encaja en lo que ya hay

| Pedido del propietario | Pieza existente que lo soporta |
|---|---|
| rocas siguiendo spline | `repeatEnabled` + `followSurface` + `RocksLab` (prior de forma) |
| muros y paredes | ```SplineConstruction``` (perfil rectangular) + `DamageField` para grietas |
| pasarelas / catwalks | `ProceduralAssemblies` (familias existentes) + daño seleccionable |
| material realista | `NormalBake`/`AutoUv` + conversor de material Fab |

### Advertencia de arquitectura

El núcleo portable no debe depender de Torch/ONNX (regla ya escrita en `GOAL.md`).
Cualquier modelo entrenado con los escaneos vive **fuera**: se ejecuta offline y su salida es un JSON
de parámetros versionado. Así el Engine recibe matemática determinista, no una caja negra.

---

## Orden recomendado

1. **A (huesos/arquetipos)** primero: desbloquea bípedos y objetos duros, y su salida (paneles, bisagras,
   sockets de armario) alimenta a B.
2. **B (sockets/pipes por spline)** después: es el consumidor natural de los IDs estables de A.
3. **C** en paralelo sólo como documento/manifiesto: no toca código hasta que el propietario lo autorice.

## Lo que este documento NO declara

- No hay bípedos, puertas, cajones ni armarios reconocidos todavía.
- No hay sockets inferidos ni acoplamiento automático implementados.
- No hay ningún asset de Megascans descargado, analizado ni integrado.
- Ocean Lab no se toca.

## Ideas agregadas después del primer incremento

### Texturas propias a partir de materiales reales (pedido del 2026-11-07)

No se trata sólo de usar el material de Megascans: se trata de **generar o modificar la textura**
de un objeto para que sea distinto, estudiando cómo están hechos los materiales reales.

- **Medición, no copia.** De un set de materiales (madera, metal oxidado, piedra, hormigón) se extraen
  estadísticas: espectro del albedo, distribución de rugosidad, anisotropía del normal, frecuencia y
  orientación de vetas/estrías, densidad de manchas de óxido, tamaño característico del poro.
  Eso se guarda como `MaterialPrior` JSON, igual que el `ScanPrior` geométrico.
- **Variación con semilla.** El mismo generador produce N variantes distinguibles del mismo objeto
  (dos tablones no idénticos) sin salir del estilo del material medido. La variación es un parámetro
  de la receta, no un retoque manual: se puede reproducir y versionar.
- **Reutiliza lo que ya existe:** `NormalBake`, `AutoUv`, `DamageUvTransfer` y la máscara de interior
  fresco. Una textura generada debe pasar por el mismo bake/UV que las demás, no por un camino nuevo.
- **Gate honesto:** comparar contra el material fuente con métricas espectrales y de escala
  (¿la veta tiene el tamaño correcto en metros?), no con "se ve bien". Y distinguir explícitamente
  "material propio generado" de "material de Megascans referenciado".

### Assets modulares sin interior (ejemplo: muebles de Fab)

El caso que planteó el propietario: un mueble modular de Fab trae la **carcasa** y las **frentes de cajón**,
pero el cajón **no existe** como volumen, no hay guías, no hay bisagra, no hay nada detrás de la cara.
Es una cáscara. La conclusión práctica: **el asset aporta la apariencia; la estructura la pone el algoritmo.**

Ruta propuesta, coherente con lo anterior:

1. **Clasificar la cáscara** con `JE::GeometryArchetypes` (`RigidFrame` + hojas `HingedPanel`/`Drawer`).
2. **Reconstruir el interior** de forma procedural: caja de cajón con fondo y laterales, recorrido y
   tope, guías como geometría generada, bisagra en el eje detectado del borde. Todo con IDs estables.
3. **Reemplazar la parte móvil del escaneo** por la pieza generada o por una variante del propio set,
   conservando el material y las UV del asset original para que no se note el cambio.
4. **Validar con física**, no a ojo: el cajón debe abrir sin atravesar la carcasa, la hoja debe girar
   sobre su eje sin contactos nuevos, y el conjunto debe quedar en reposo estable.
5. **Mismo patrón para puertas, portones y válvulas**: el escaneo da la chapa, el algoritmo da el eje,
   el recorrido y los topes.

Esto convierte el problema de "comprar el asset correcto" en "medir la cáscara y completar la mecánica",
que es exactamente lo que el Playground puede probar antes de portarlo al Engine.

## Próximo paso mínimo verificable

RED de A: un corpus con (bípedo, cuadrúpedo frontal, araña, puerta, cajón, armario, pieza mecánica)
donde el algoritmo actual falle de forma reproducible, y una tabla de qué evidencia falta en cada caso.
Ese RED es el que justifica el primer incremento de `JE::GeometryArchetypes`.
