# Ruta de modelado, superficie, LOD, PWAS y EPLS

Fecha: 2026-09-09. Destinatario: Astra y futura integración M10.5.

**Estado: diseño de integración propuesto; no prueba de backend completo ni
interfaz nativa congelada.** Define la ruta solicitada dentro de la excepción
`Tools/BlenderWorker/**`; no cambia Engine, Editor, RuntimeFormats, ADR ni roadmap.
La autoridad nativa y Git siguen perteneciendo al owner del slice activo.

## 1. Decisión central

JericoEditor conserva intención, selección, recetas, layers y decisiones manuales.
Blender es el productor geométrico offline. El cooker de Jerico valida y transforma
candidatos en productos runtime. PWAS conserva la autoría procedural del mundo y
sus placements; EPLS conserva las capas ambientales. Ninguno ejecuta Geometry
Nodes en Shipping.

**No se genera una roca diferente para cada LOD.** Todos representan el mismo
producto, con su misma identidad espacial y semántica. La cámara cambia su
representación, no el escenario, los placements, la pintura ni la física.

El productor definitivo debe seguir la dirección vigente de diseño independiente.
Los addons comprados son referencias o adaptadores privados explícitos mientras
su uso esté autorizado; no se renombran como código propio. Smart Remesh no se
declara obligatorio ni aceptado hasta superar la comparación high/low.

## 2. Vocabulario inequívoco

Este glosario local no modifica el `CONTEXT.md` global fuera de la carpeta permitida.

**Fuente autoritativa:** intención editable del diseñador, layout BSP o asset
fuente identificado, con revisiones, límites, exclusiones y decisiones manuales.
No es la última malla que dejó un proceso Blender.

**HIGH:** superficie detallada de referencia para forma y baking. Es un producto
de autoría; no equivale a LOD0 ni se incluye por defecto en Shipping.

**LOD0:** malla de juego de máxima calidad admitida. Puede tener muchas menos
caras que HIGH; conserva su apariencia dentro del perfil de error aprobado.

**LOD1..N:** representaciones más económicas del mismo producto. No son nuevas
recetas, nuevas seeds ni versiones independientes del mundo.

**Preview:** representación temporal y económica para editar. Nunca sustituye
silenciosamente HIGH, un LOD final, collision o un resultado aplicable.

**PWAS:** Procedural World Authoring System. Orquesta propuestas de autoría;
sus módulos de placement distribuyen elementos respetando soporte, reglas,
presupuestos y decisiones manuales.

**EPLS:** Environmental Procedural Layer System. Compone historia ambiental y
capas superficiales. Es el nombre correcto del sistema mencionado como «elsd».
Puede solicitar dressing, pero no corta la estructura del mundo directamente.

**Layer de geometría:** operación editable que cambia forma o campos base:
strata, fractura, corte, desgaste geométrico, decal con relieve real.

**Layer EPLS:** intención ambiental y sus máscaras/pintura, separada de la receta
geométrica. Una máscara mojada no es por sí sola una modificación de collision.

**Superficie de soporte:** superficie canónica admitida y versionada que consultan
placement y reproyección. No depende del LOD visible en cada frame.

## 3. Ruta principal

```text
JericoEditor: layout/asset + selección + recetas/layers + locks
                         │
                         ▼
Snapshot de fuentes, BSP/semántica y espacio protegido
                         │
                         ▼
Blender: modelado base → layers geométricos → HIGH + campos base
                         │
                         ▼
Validación de HIGH: forma, regiones, cavidades, seams, keep-clear
                         │
                         ▼
Retopología/reducción candidata → LOD0 de juego
                         │
                         ▼
UV + triangulación fijada + cage + bake HIGH → LOD0
                         │
                         ▼
LOD1..N + transferencia/rebake + validación contra HIGH
                         │
                         ▼
Jerico: superficie/producto candidato validado
            ┌────────────┼─────────────────┐
            ▼            ▼                 ▼
     PWAS placement   EPLS pages    Collision/fracture/nav
            └────────────┼─────────────────┘
                         ▼
Preview/Diff integral → validación → Apply transaccional
                         │
                         ▼
Cook Jerico → assets/páginas runtime → package source-free
```

El diagrama expresa dependencias, no exige ejecutar todas las etapas en serie.
Tras validar el soporte, collision y parte de PWAS/EPLS pueden evaluarse en jobs
independientes admitidos por el coordinador existente. El Apply final espera el
conjunto coherente de resultados requeridos, no mezcla revisiones.

Las validaciones que puedan invalidar la operación ocurren antes de publicar
Apply. La recomputación posterior de tiles/pages no debe dejar un mundo aplicado
con conectividad inválida; el owner nativo debe resolverlo mediante staging y
las transacciones existentes, no un segundo journal privado.

## 4. Dos entradas, una misma ruta de producto

### Asset reutilizable: roca, columna, prop, item

1. El diseñador define forma, escala física, origen/pivote y semántica.
2. Blender genera HIGH y la familia admitida LOD0..N.
3. Jerico cocina el asset/prefab y sus dependencias.
4. PWAS elige variantes de ese catálogo y coloca instancias.
5. EPLS puede añadir variaciones ambientales por superficie/instancia según
   los consumidores admitidos, sin duplicar una malla por cada mancha.

Las instancias rígidas comparten geometría y LOD; sus transformaciones, tintes y
bindings permitidos son datos de instancia. Una deformación que cambia realmente
la forma requiere una variante nueva y validada, no escalar sin límite un collider.

### Geometría ligada al nivel: cueva, cliff, pared, carretera

1. BSP/layout aporta regiones sólidas, vacíos, portales y volúmenes protegidos.
2. La receta trabaja sólo sobre regiones seleccionadas y permitidas.
3. HIGH mantiene entrada/salida y dimensiones jugables; los chunks comparten
   identidad de seams y zonas de evaluación con halo.
4. Se producen LOD, collision y soporte por producto/celda.
5. PWAS coloca dressing sobre el soporte admitido; EPLS compone el ambiente.

La cueva no se inventa ignorando el BSP. El modelo visual decora o desarrolla una
envolvente autorizada. Abrir un túnel nuevo requiere un cambio estructural explícito.

## 5. Modelado y Layer System

Orden inicial explícito de una receta geológica:

1. Fuente/base y envolvente autorizada.
2. Forma macro: masa, columnas, estratos, bloques, pared o cavidad autorizada.
3. Cortes estructurales y spline destructivo.
4. Desgaste geométrico y detalle de escala media.
5. Decal 3D que realmente altera la malla, si la receta lo declara.
6. Campos base y clasificación de superficies finales/expuestas.
7. Revalidación completa de espacio protegido.

Este orden se persiste; no se reordena para acelerar el cook. Si el usuario cambia
el orden, cambia la identidad de evaluación y se recalculan sus dependientes.

Cada layer conserva GUID, tipo/versión, seed, parámetros, selección, include/exclude,
estado visible/locked y referencias a fuentes. Se deriva la seed de identidad
estable; no de thread, reloj, posición en una lista o nombre visible del objeto.

Selección de región usa GUID y revisión de fuente. Selección de loop usa topología
identificada y una cadena ordenada de aristas. Tras remesh los índices ya no son
autoridad: se exige correspondencia validada o se marca la selección como pendiente
de resolución. No se proyecta al elemento «más cercano» si pertenece a otra pared,
otro chunk o al lado opuesto de una lámina fina.

### Decal y spline: separar tres resultados

| Resultado solicitado | Etapa/owner | Consecuencia |
| --- | --- | --- |
| Suciedad, grieta pintada, cambio de normal sin relieve | Material/EPLS | No regenera HIGH ni collision |
| Overlay geométrico fino, independiente | Producto decal + placement | Tiene soporte, bounds y política LOD propios; no fusionar sin intención |
| Mordida, perforación o corte real | Layer geométrico/BSP/Fracture | Regenera superficie, LOD y derivados afectados |

El mismo gesto o preset puede solicitar las tres operaciones, pero mantiene sus
identidades y responsabilidades. EPLS reacciona a la cara recién expuesta; no
finge una perforación transitable mediante un cambio de color.

## 6. HIGH → LOD0: remesh no equivale a optimización aceptada

Se trabaja sobre una copia de HIGH. La fuente detallada y los campos semánticos
se retienen para comparar, hornear y reproyectar.

La selección del algoritmo es política explícita del perfil: preservar topología
y simplificar cuando sea suficiente; retopologizar cuando haya un beneficio
demostrado. Un addon gratuito/comprado o un algoritmo propio cruza la misma
interface de candidatos y debe superar los mismos criterios. No hay fallback
silencioso de algoritmo, calidad o licencia.

Se fijan antes de ejecutar: presupuesto de triángulos/vértices, errores máximos
en metros y píxeles, restricciones de seams/regiones, recursos y timeout. El
target de caras es orientativo si el solver lo trata como sugerencia. El conteo
de triángulos del producto final es obligatorio: un n-gon puede esconder muchos.

Bloquear o ponderar especialmente:

- boca de cueva, portales, piso transitable y proximidad a keep-clear;
- límites de chunk/celda y seams compartidos;
- cambios de región, material y PhysicalMaterial;
- silueta característica, bordes de estrato y separación de columnas;
- anchors/bonds de destrucción y superficies de contacto;
- seams UV si se pretende conservar el atlas.

Rechazar geometría no finita, degeneración, agujeros nuevos, caras invertidas,
non-manifold no permitido, desaparición/fusión de regiones o cambio de conectividad
funcional. No confundir un túnel válido con un agujero accidental: una pared de
cueva con espesor puede ser una superficie cerrada aunque el paso esté abierto.

Una diferencia de cantidad de aristas de borde, por sí sola, no demuestra un
cambio topológico: comparar loops/correspondencias y conectividad. Los checks del
probe privado son conservadores y no reemplazan este validador de producción.

Si cumplir el presupuesto exige destruir la forma, el resultado falla. Se ofrecen
partición, mayor presupuesto o cambio explícito de perfil; no se tapa la entrada
ni se elimina un pilar para lograr un porcentaje atractivo.

## 7. UV, materiales y bake high → low

Antes del bake, fijar escala, transforms aplicables, winding, UV y triangulación
de LOD0. Calcular sus normales/tangentes con el contrato del cooker/renderer.
Después del bake no puede cambiar la triangulación/tangent basis sin invalidarlo.

Políticas de UV existentes: `preserve-source`, `unwrap-and-pack`, `hybrid` y
`triplanar-only`. Triplanar no elimina automáticamente la necesidad de UV para
normal bake, lightmaps o decals. El material y el consumidor deciden qué datos
son obligatorios, no la comodidad del generador.

La ruta autoritativa de la directiva es Cycles CPU, HIGH seleccionado, low activo,
cage explícito y targets de imagen definidos. Se hornean normal tangente y los
mapas requeridos de AO/cavity/curvature/clasificación. Se validan misses, cruces a
otro chunk, texels inválidos, padding, seams y mip/compression safety.

Las máscaras discretas de material/región no se promedian como colores. Se usan
IDs/tablas o muestreo discreto con correspondencia de superficie; los campos
continuos declaran rango, espacio, domain y interpolación.

`MaterialDefinition` conserva el material visual Jerico. El shader preview de
Blender no se importa como un shader runtime universal. `PhysicalMaterial` es una
referencia GUID/hash separada; textura de roca o máscara de hielo no decide por
sí sola densidad, fricción, restitución, sonido o resistencia estructural.

**Un normal map recupera iluminación de detalle; no recupera silueta, volumen,
agujeros, colisión ni una entrada de cueva que el remesh eliminó.**

## 8. Familia LOD

Usar LOD0 como referencia de identidad/materiales y HIGH como referencia de
fidelidad. Preferir candidatos independientes desde LOD0 congelado para no
acumular errores; si una herramienta simplifica progresivamente, medir igualmente
cada salida contra HIGH/LOD0, no sólo contra el LOD anterior.

La directiva existente propone estos rangos orientativos, nunca aceptación automática:

| Representación | Presupuesto relativo orientativo | Función |
| --- | --- | --- |
| HIGH | Sin equivalencia obligatoria con LOD0 | Referencia de autoría/bake |
| LOD0 | Presupuesto de juego de máxima calidad | Vista próxima |
| LOD1 | 45–65 % de LOD0 | Vista intermedia |
| LOD2 | 20–35 % de LOD0 | Vista lejana |
| LOD3 | 8–18 % de LOD0 | Muy lejano/entrada HLOD cuando esté admitido |

Cada LOD debe:

1. Reducir coste medido, manteniendo límites de error y todas las restricciones.
2. Mantener GUID de producto, regiones, material slots, PhysicalMaterial y seams.
3. Transferir UV/atributos con correspondencia validada; si eso no funciona,
   generar UV propios y rebakear desde HIGH para ese LOD.
4. Validar uso compartido de texturas/normal maps; no asumir que la textura de
   LOD0 funciona con cualquier triangulación o suavizado de otro LOD.
5. Reprobar keep-clear, mouth silhouette y continuidad con chunks vecinos.
6. Registrar bounds, error geométrico, error de silueta, hashes y coste.

La selección runtime usa error proyectado, tamaño en pantalla, cámara y presupuesto
mediante el owner existente; no distancias fijas universales. En perspectiva la
proyección depende de profundidad/FOV y en ortográfica de escala: un escenario
isométrico necesita perfiles distintos de una cámara FPS. Histeresis o morph sólo
se usan si el runtime los admite; no se promete ocultar errores con un crossfade.

Ni el salto LOD ni streaming reevalúan PWAS/EPLS. Los placements siguen anclados a
la superficie canónica. El perfil visual de LOD debe limitar separación aparente
en zonas con props apoyados, decals o contactos visibles.

Collision es otra representación: puede compartir una fuente segura, pero nunca
se cambia automáticamente al collider de un LOD lejano por mover la cámara.

## 9. Punto exacto de aplicación de PWAS

PWAS participa desde la intención de autoría, pero el **placement dependiente de
superficie** consume el producto ya validado. No espera al frame del runtime y
no hace raycast contra el LOD visible elegido por la cámara.

Entradas: regiones, soporte estable, normales, pendiente, campos de cavidad/estrato/
humedad, máscaras, Weighted Asset Set/prefabs y sus bounds, reglas, seeds, locks,
occupied bounds, exclusions y presupuesto.

Salida: placements candidatos con claves estables y transformaciones, referencia
de prefab/contenido, soporte y dependencias. Conserva `PwasCellKey`,
`PwasSplineSampleKey` y `PwasHandPaintedSeedKey`; no sustituye esas claves por
índices de vértices o una hash nueva que pierda las ediciones manuales.

Ejemplos: fragmentos al pie de un cliff, piedras sobre piso de cueva, cristales
en regiones seleccionadas, vegetación junto a humedad, postes a lo largo de una
spline. Consultar soporte y validar **la envolvente completa del elemento**, no
sólo su pivote: un árbol cuyo origen está fuera del pasillo puede bloquearlo.

Reevaluación conserva Remove/tombstones, Replace, Lock y Promote-to-manual según
los contratos existentes. Un Lock conflictivo no se borra para cumplir la densidad;
se informa el conflicto y se mantiene la decisión manual hasta resolución explícita.

Si colocar un objeto exige excavar una base, PWAS solicita una nueva operación
geométrica identificada. No cambia el BSP a escondidas. El diseñador acepta la
nueva revisión y entonces se recalculan soporte, placements y derivados.

## 10. Punto exacto de aplicación de EPLS

HIGH aporta campos base, pero éstos se transportan a superficies/páginas canónicas
admitidas. EPLS conserva sus propias recetas, `RegionMask`, layers manuales y
strokes; Blender no aplana la pintura del usuario dentro de la receta geológica.

Secuencia por revisión:

```text
Geología: cavity/porosity/strata/fresh-break + soporte versionado
    → reglas EPLS y scope regional
    → muestras procedurales
    → Include/Exclude manuales en su orden explícito
    → páginas finales acotadas
    → bindings/materiales y otros consumidores admitidos
```

Contrato real actual de `composeEplsFinalMask`, con muestras uint16:

```text
combined = min(65535, procedural + manualInclude)
final    = max(0, combined - manualExclude)
```

No reemplazar esta fórmula por multiplicación, `max` o una mezcla shader distinta.
La preparación y orden de manual layers pertenecen al evaluador EPLS existente.
Sus páginas llevan `surfaceOwnerGuid`, `pageGuid`, bounds, dimensión y hashes;
el límite actual es 4096 por dimensión y 16.777.216 texels por página.

EPLS puede producir material blending, decals y propuestas de patches/instancias.
Si necesita objetos 3D, entrega una intención a PWAS/placement o al productor de
decal/patch admitido. Si necesita cambiar estructura o collision, pide una
operación de geometría/Fracture explícita. Nunca ejecuta ese cambio por sí solo.

Ejemplo de hielo: brillo, frost y humedad son superficie; estalactitas o bloques
son geometría colocada; estrechar el túnel es un cambio estructural. Las tres
cosas no comparten automáticamente ni representación ni permiso de Apply.

Una máscara de humedad puede influir en reglas de placement de musgo. Resolver
la dependencia de forma dirigida por revisión: soporte → máscara → placements.
No realimentar esos placements en la misma evaluación para repetir infinitamente
la máscara y el scatter. Cambiar la intención requiere otra revisión explícita.

Runtime puede consumir máscaras cocinadas y parámetros ambientales admitidos.
Eso no le autoriza a ejecutar el grafo de autoría, rehacer topología o pintar
fuentes. Una consecuencia física del hielo/mojado exige mapping y consumidor
PhysicalMaterial/gameplay explícitos, no sólo el shader.

## 11. Reproyección y cambios: conservar intención, no índices

Al modificar HIGH o LOD0, preparar nuevo soporte y reproyectar selecciones,
atributos y pintura por producto/región/surface domain y proximidad validada.
La distancia máxima, normal compatible y región/owner compatible son restricciones.
Un simple nearest point no basta en cuevas, paredes finas o fragmentos próximos.

Si desaparece soporte: mostrar strokes/attachments sin soporte y su diff. No
destruir la intención original ni pegarla en la pared opuesta. Un dato sin soporte
no entra al cook hasta que su eliminación/reasignación haya sido resuelta.

| Cambio del diseñador | Trabajo que se invalida | Lo que no debe rehacerse por defecto |
| --- | --- | --- |
| Shape, corte o spline estructural | HIGH, LOD, bake, soporte y derivados espaciales afectados | Regiones/chunks no dependientes |
| Remesh policy/LOD0 | UV/bake/LOD y soporte dependiente | Receta HIGH si no cambió |
| Sólo política de LOD lejano | Ese LOD, páginas y pruebas asociadas | PWAS, pintura EPLS y collision canónica |
| UV/atlas/tangent basis | Mapas afectados y bindings dependientes | HIGH y placements con soporte válido |
| Pintura EPLS | Páginas EPLS/materiales; scatter dependiente si se declaró | Topología, LOD y collision sin dependencia explícita |
| Densidad/seed de scatter | PWAS desbloqueado y sus dependencias | HIGH y atlas compartido |
| PhysicalMaterial | Collision/fracture/respuestas dependientes | Render mesh si ningún parámetro geométrico depende de él |
| Material visual | Binding/texturas/material cook | Shape si su schema declara independencia |

Compartir `DirtySpatialRegion` existente, no un invalidation manager privado.
Usar old/new bounds, dependencias y halo de cada consumidor; no recocinar todo el
mundo ni ignorar navegación, sombras/GI y streaming afectados por geometría nueva.

## 12. Concurrencia, preview y cache

JericoEditor publica snapshots inmutables; no ejecuta hashing, bake o espera
bloqueante de procesos en el hilo UI. Reutilizar `AuthoringWorkCoordinator`,
`CancellationSource/Token`, `SmartImportCookJob`/QProcess, `SmartImportCookCache`
y `ProjectTransactionCoordinator`. No introducir otro scheduler, broker o DDC.

Preview primero, 512 px por defecto en el paquete privado, con control real de
resolución geométrica, dos threads y prioridad baja. El modelo de UI mantiene el
último resultado válido marcado desactualizado mientras calcula el siguiente;
si nunca hubo resultado válido, muestra vacío/error, no uno inventado.

Cambios rápidos se agrupan; cancelar o invalidar una revisión impide su promoción.
No se paraleliza `bpy` entre threads Python. Trabajos costosos se separan en hijos
admitidos y limitados, nunca en siete procesos simultáneos sin presupuesto.

La cache existente incluye identidad de fuente/BSP, receta/layers y orden,
selección, semántica/keep-clear, generator/schema/adapter, toolchain, parámetros,
seeds, perfil de geometría, algoritmo remesh, UV/bake, LOD, campos, material/physical
y target cuando cada dato sea dependencia de esa etapa. Preview y final tienen
keys distintas. Pintura manual se identifica en EPLS, no contamina la key HIGH.

La cancelación termina el job sin Apply ni entradas válidas de cache parciales.
El gate nativo `VerifyBlenderJobLifecycleAndCancellation` y el containment de
descendientes siguen siendo prerrequisitos de integración, no algo demostrado
por los tests de un hijo inmediato del harness privado.

## 13. Transporte y cook

GLB es transporte intermedio, no formato del mundo runtime. Se acompañan mallas,
UV/atributos, material slots, campos/masks, LOD, provenance y collision candidates
con sidecars/identidades verificables; no depender sólo de nombres de objetos o
de `extras` para datos por cara/vértice que el exporter pudiera omitir.

Jerico resuelve GUID/hash y canoniza coordenadas, unidades, índices, materiales y
atributos. Valida productos completos, errores espaciales y dependencias. Cocina
las representaciones admitidas `.rmesh`, `.rcoll`, materiales/texturas, prefabs,
nav y páginas de mundo por sus rutas existentes. Jolt/Navigation/Destruction
conservan autoridad física y de simulación; Blender no los sustituye.

Shipping contiene datos cocinados, no HIGH, cages, `.blend`, Python, addons,
Instant Meshes ni fuentes de Geometry Nodes. La prueba final es replay desde un
package source-free, no abrir la escena en Blender.

## 14. Ejemplos completos de causalidad

### Cueva húmeda para FPS

BSP define entradas, ancho/altura mínimos y ruta requerida. Geología produce
pared y HIGH dentro de la envolvente. Remesh y LOD preservan la boca y no cruzan
keep-clear. Bake aporta microdetalle. PWAS coloca piedras fuera del paso usando
sus bounds completos. EPLS aplica humedad en cavidades y pintura manual; musgo
geométrico se solicita a placement. Collision/nav validan el agente real.

### Cliff junto a carretera

La spline vial y su volumen de paso son autoridad de diseño; no se cambian para
acomodar el cliff. PWAS puede poner escombros en la banquina permitida, no dentro
del barrido del vehículo. EPLS genera polvo/manchas de escorrentía. Los LOD se
evalúan a velocidad y distancia de cámara representativas; collision no cambia
al usar un LOD lejano.

### Escenario isométrico con items

Assets individuales se generan/cocinan una vez; PWAS coloca variantes con seeds
estables y mantiene los items manuales. EPLS añade variación ambiental por región.
Los LOD se eligen por tamaño proyectado ortográfico y coste, no por una regla FPS
copiada. La identidad del item y sus datos gameplay no dependen del mesh LOD.

## 15. Gates antes de declarar esta ruta terminada

1. Fuente/BSP/locks: regenerar no cambia fuentes ni toca selección ajena.
2. HIGH: estilos legibles, dimensiones físicas, regiones y keep-clear correctos.
3. Remesh: menos triángulos con forma/conectividad válidas, no sólo target alcanzado.
4. UV/bake: mapas completos, sin cruces, con base tangente y materiales correctos.
5. LOD: comparar cada salida con HIGH; vistas exteriores e interiores, bocas de
   cueva, seams vecinos, cambios de cámara y estabilidad de attachments.
6. PWAS: mismas claves bajo reevaluación admisible; locks/tombstones persistentes;
   no invasión por envolventes ni reubicación por cambio de LOD.
7. EPLS: misma composición nativa, pintura preservada/reproyectada con diff,
   sin topología alterada por pintar y sin salto ambiental al cambiar LOD.
8. Material/PhysicalMaterial: cambios invalidan sólo dependencias correctas.
9. Collision/nav/destruction: conectividad requerida y respuestas reales válidas.
10. Lifecycle: UI no bloqueada, cancelación/stale/recovery sin promoción parcial.
11. Determinismo: repetir jobs limpios y variar scheduling conserva bytes canónicos
    bajo el pin; no confundir matching de dos capturas con prueba completa.
12. Cook/runtime: replay source-free y Beauty real en Jerico, no sólo Workbench.

Antes de cada fixture, el perfil fija errores, presupuestos y cámaras. Ninguna
prueba puede aumentar tolerancias después de ver un resultado fallido para aprobarlo.

## 16. Qué existe y qué falta hoy

- Existen contratos nativos PWAS/EPLS, recipes, masks, queries, presets y cook.
  Esto no demuestra automáticamente su conexión al productor Blender nuevo.
- Existen rutas privadas de siete addons y previews repetidos de 512 px.
- La primera cadena roca → core OURS produjo menos caras, pero quedó rechazada
  por defectos geométricos y por el incidente de siete cachés `.pyc`.
- OURS no es el modo orgánico normal de este Windows con bin disponible.
  La prueba STABLE/Instant Meshes, cueva y remediación siguen pendientes de
  autorización. Este documento no la concede ni inicia procesos.
- No están admitidos por estas pruebas: ruta completa HIGH→UV/bake→LOD, campos
  espaciales y provenance completos, PWAS/EPLS conectados, collision/nav/cook,
  main-thread responsiveness ni Shipping.
- Qt/controles nuevos permanecen en M10.5. El owner decide los gates de integración
  posteriores a M9; este documento no altera el slice ni retira código por sí solo.

## 17. Fuentes concretas y reconciliación

- [Directiva Blender, §§27–33](../../docs/JERICOENGINE_M8_BLENDER_HEADLESS_GEOLOGY_AUTHORITY_DIRECTIVE.md): UV, bake, LOD, PWAS/EPLS, derivados y Shipping.
- [Master PWAS/EPLS, §§1–3, 11–12](../../docs/PROCEDURAL_PWAS_EPLS_MASTER.md): intención manual, orquestación, ambiente y distribución.
- [ADR-030](../../docs/decisions/ADR-030-PWAS-POISSON-SCATTER-PRESETS.md): claves, presets, locks y Shipping.
- [PwasRecipeGraph.hpp](../ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/PwasRecipeGraph.hpp): tipos y claves existentes.
- [PwasWorldQuery.hpp](../ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/PwasWorldQuery.hpp): soporte y validación de envolvente.
- [EplsRecipe.hpp](../Epls/Include/Jerico/Epls/EplsRecipe.hpp): recipes, scope y layers manuales.
- [EplsMask.cpp](../Epls/Source/EplsMask.cpp): composición uint16 exacta y límites de página.
- [EplsCooker.hpp](../Epls/Include/Jerico/Epls/EplsCooker.hpp): páginas, hashes y dependencias.
- [Controles M10.5](M10_5_GENERATOR_CONTROLS.md), [suite privada](PRIVATE_ADDON_SUITE_M10_5.md) y [pruebas/limitaciones actuales](PRIVATE_PREVIEW_EXECUTION.md).

La directiva histórica usa términos de planificación como broker y enumera
integraciones futuras. Aquí no se crea ese módulo: se reutilizan los owners
nativos existentes y se marca lo no conectado como integración requerida.
