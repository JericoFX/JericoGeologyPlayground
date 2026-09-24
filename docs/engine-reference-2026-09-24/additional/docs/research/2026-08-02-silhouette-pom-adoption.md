# Silhouette POM / Prism POM: evaluación de adopción para JericoEngine

Fecha de revisión: 2026-08-03
Estado: investigación técnica; no cambia el slice M6F activo ni constituye una
prueba de runtime.

## Conclusión ejecutiva

**Decisión recomendada: Adapt, después de cerrar M6F Height.**

Jerico debe conservar el POM acotado y el MicroDisplacement ya diseñados como
las dos rutas de Height ordinarias. Una variante de silueta sólo merece entrar
como **perfil raster alto, opt-in y medido**, para assets cercanos donde el
borde contra el cielo/fondo sea parte de la lectura visual. No debe sustituir
el POM actual, ni presentarse como “displacement real” de colisión, navegación,
PWAS o EPLS.

El nombre “SilPOM/SPOM” se usa de forma imprecisa en la comunidad. Las fuentes
primarias revisadas demuestran tres familias distintas:

1. **POM normal:** busca la intersección del rayo con un height field en el
   píxel y desplaza UV; no modifica el borde geométrico.
2. **Silhouette POM de Crysis 3:** la presentación oficial de Crytek en GDC 2013
   documenta VS que extruye, GS que genera prismas/clip planes y PS que
   ray-marcha, calcula la UV final y descarta píxeles; también registra presión
   sobre PS y GS como bottleneck de generación de prismas. La documentación
   pública de CryEngine posterior no publica esos stages.
3. **Prism POM / shell mapping académico:** extruye cada triángulo a un volumen
   prismático y encuentra el rayo contra el height field dentro de ese volumen.
   Esta es una familia con silueta de height field mucho más fundamentada, pero
   requiere geometría envolvente/shell y un contrato de assets claramente mayor.

Por tanto, no se debe llamar “Prism POM” a un simple `clip()` de UV fuera de
rango, ni asumir que la opción **Displacement Scale** de Crimson Desert sea
SilPOM. Pearl Abyss confirma públicamente una opción y correcciones de
*displacement mapping*, pero no identifica su algoritmo.

## Evidencia primaria y qué confirma cada fuente

| Fuente | Hecho confirmado | Límite de la afirmación |
|---|---|---|
| [CryEngine: POM](https://www.cryengine.com/docs/static/engines/cryengine-5/categories/23756816/pages/29450125) | POM es menos costoso que tessellation/displacement, no cambia la silueta, y Crytek lo limita a perfiles PC High/Very High; recomienda POM para superficies planas y una alternativa de silueta/tessellation cuando el borde sea visible. | No especifica el código de Jerico ni demuestra un presupuesto transferible entre engines. |
| [Crytek, GDC 2013 — *The Rendering Technologies of Crysis 3*](https://www.gdcvault.com/play/1017626/Advanced-Visual-Effects-with-DirectX) | El slide de Silhouette POM declara: VS extruye; GS genera prismas y clip planes; PS ray-marcha, calcula UV final y descarta el píxel. El mismo slide anota “Lots of pressure on PS” y “GS is the bottleneck (prism gen)”. | Es una ruta de Crysis 3/D3D11: confirma el algoritmo/stages de esa presentación, no el rendimiento de Jerico/Vulkan ni la implementación de una versión posterior de CryEngine. |
| [CryEngine: Silhouette POM](https://www.cryengine.com/docs/static/engines/cryengine-5/categories/23756816/pages/29450143) | Su SPOM usa el mismo setup de textura que POM, añade pasos y una distribución dependiente de la vista; es más caro, susceptible a artefactos y sólo para PC Very High. | La página de documentación no detalla stages; no se debe reemplazar la evidencia de GDC por una inferencia sobre versiones posteriores. |
| [Dachsbacher y Tatarchuk, *Prism Parallax Occlusion Mapping with Accurate Silhouette Generation*](https://inria.hal.science/inria-00606806) | Prism POM representa la superficie desplazada dentro de prismas extruidos desde triángulos; la intersección rayo-superficie dentro del volumen permite siluetas correctas de la altura. | Es un poster de investigación de 2007; no es una integración Vulkan/Diligent ni una promesa de rendimiento moderno. |
| [Jeschke, Mantler y Wimmer, *Interactive Smooth and Curved Shell Mapping*](https://www.cg.tuwien.ac.at/research/publications/2007/JESCHKE-2007-ISC/JESCHKE-2007-ISC-Preprint.pdf) | Los shells extruyen triángulos y pueden ofrecer parallax, self-occlusion y siluetas; corregir la continuidad del shell evita artefactos de aproximaciones tetraédricas. El propio trabajo informa mayor tiempo de render. | Es una técnica de shell completa, no una receta reducida a un material POM normal. |
| [Tatarchuk, *Dynamic Parallax Occlusion Mapping with Approximate Soft Shadows*](https://doi.org/10.1145/1111411.1111423) | POM admite muestreo adaptativo y LOD para balancear precisión/coste, y puede integrar auto-oclusión/sombras aproximadas. | No soluciona por sí solo la silueta geométrica. |
| [DiligentCore oficial](https://github.com/DiligentGraphics/DiligentCore) y [Tutorial 08 oficial](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial08_Tessellation) | Diligent usa HLSL con Vulkan/SPIR-V y tiene ruta de hardware tessellation; la muestra 08 ilustra tessellation adaptativa. | Disponibilidad de API no convierte tessellation/GS en baseline de Jerico. |
| Donor local DiligentFX `F:/penumbram/src/DiligentEngine/DiligentFX/Shaders/PBR/public/PBR_Parallax.fxh` y `F:/penumbram/src/DiligentEngine/DiligentFX/Shaders/PBR/private/RenderPBR.psh` | Hay un POM maduro con base tangente derivada, `SampleGrad`, pasos 4..64, refinamiento, fade por distancia y por ángulo rasante. | Es POM sin una ruta de prism/shell; sirve de oráculo del POM actual, no de implementación SilPOM. |

## Crimson Desert: qué está confirmado y qué no

Pearl Abyss publicó literalmente que el parche 1.03 añadió la opción
**`Displacement Scale`** bajo `Settings > Graphics` para PC y que el 1.04
corrigió errores/flickering de
*displacement mapping* ([1.03](https://crimsondesert.pearlabyss.com/en-US/News/Notice/Detail?_boardNo=81&pubDate=20260411),
[1.04](https://crimsondesert.pearlabyss.com/en-us/News/Notice/Detail?_boardNo=84&hl=el-GR)). Eso confirma que el juego expone una opción de displacement y que
la ruta tuvo problemas visuales corregidos.

No he encontrado, entre esas comunicaciones oficiales de Pearl Abyss, una
identificación de **Silhouette POM**, **SPOM**, **Prism POM**, *shell mapping*,
geometry shader, tessellation o el layout de sus buffers. Atribuirle SilPOM a
Crimson Desert sería por tanto una **inferencia de comunidad, no un hecho
confirmado por Pearl Abyss**. Esa distinción importa: no podemos copiar una
implementación desconocida a partir de una captura ni usarla como prueba de
coste o calidad.

## Comparación con el diseño M6F actual

| Ruta | Stages Jerico planificados | ¿Cambia la silueta? | Coste/datos | Uso recomendado |
|---|---|---:|---|---|
| `Off` | VS + PS PBR normal | No | Ningún Height activo | Default. |
| `Parallax` M6F | VS normal; POM en PS | No | 1 scalar Height opcional; 4..64 muestras PS con fade | Detalle interior: roca, metal, madera, hielo, suelo cercano. |
| `MicroDisplacement` M6F | VS muestrea Height LOD 0; PS reconstruye macro normal | Sí, sólo hasta la densidad de vértices existente | Misma Height; vértices/malla suficientemente densos | Borde real de objetos donde el mesh ya soporta el relieve. |
| Silhouette POM de Crysis 3 | VS extruye; GS prismas/clip planes; PS ray-march/UV/discard | Sí | Presión PS; GS bottleneck de prism generation; más caro que POM | Referencia concreta de alto coste; no baseline ni supuesto de rendimiento Vulkan. |
| Prism POM / shell mapping | Shell pre-cocinado o GS + PS; potencial `SV_Depth` | Sí, para el height field dentro del shell | Height + shell/conectividad + mayor cobertura/pixel cost | Sólo una clase de material de alta calidad y assets cercanos validados. |
| Tessellation | VS + HS + DS + PS, patch list | Sí | Requiere feature y más geometría dinámica | Alternativa futura, no baseline M6F. |

La decisión M6F de no hacer hardware tessellation baseline sigue siendo correcta:
POM cubre el detalle visual rápido; MicroDisplacement proporciona un borde
real cuando hay geometría. Silhouette/Prism POM no es un reemplazo gratis para
ninguna de esas rutas.

## Qué exige una silueta verdadera

POM ordinario sólo rasteriza la superficie base y desplaza el punto de shading;
por ello no puede aparecer un nuevo contorno fuera de la cobertura rasterizada.
Prism POM resuelve esto construyendo un volumen alrededor de cada triángulo:
el contorno potencial existe antes de la intersección de píxel. El trabajo de
Dachsbacher/Tatarchuk habla explícitamente de prismas extruidos y el de Jeschke
et al. señala que la continuidad entre shells es necesaria para evitar huecos y
artefactos.

La ruta concreta de Crysis 3 publicada por Crytek es **VS extrude → GS
prism/clip planes → PS ray-march/UV/discard**. Es una demostración de que la
silueta se puede construir en raster sin HS/DS, pero sus propios slides llaman
al GS el bottleneck y señalan presión elevada en PS. Para Jerico hay tres formas
conceptuales de explorar la propiedad, sin asumir que la de Crysis 3 sea la
ganadora:

1. **Shell/prism pre-cocinado (recomendado para un prototipo):** el cooker
   genera una envolvente conservadora y la runtime sólo usa VS+PS. Evita usar
   GS como requisito permanente y hace el coste de geometría, memoria y LOD
   observable en el asset.
2. **Geometry shader que emite prismas:** es la ruta publicada de Crysis 3 y
   es factible con Vulkan/Diligent, pero parte con un riesgo conocido de
   bottleneck. Debe demostrar throughput en una GPU objetivo; no se asume que
   sea performance-friendly.
3. **HS/DS tessellation:** compatible como feature de Diligent, pero requiere
   patch topology, feature/fallback y una familia de pipeline adicional; queda
   fuera del primer candidato SilPOM.

Un simple PS que descarta UVs parallax fuera de `[0,1]` puede mejorar algunos
bordes **dentro de una cobertura ya existente**, pero no equivale al algoritmo
de prismas ni garantiza la silueta de un height field. Se puede evaluar como
modo barato de *edge clipping*, pero debe llevar otro nombre y no reclamar
“Prism POM” ni silueta correcta.

## Vulkan/Diligent y datos de GPU

El shader source oficial de Diligent puede compilar HLSL a SPIR-V para Vulkan y
Diligent expone VS, PS, GS, HS y DS. Eso permite un experimento raster puro;
no exige ray tracing. El resultado correcto, sin embargo, depende del contrato
de pipeline, no de que los stages existan.

Para una ruta shell se necesitan, como mínimo:

- posición, normal, tangente y UV válidos por triángulo; conectividad/edge
  policy si la envolvente se cocina;
- Height Linear `ScalarMask`, con escala en metros y midpoint como M6F;
- normal map coherente con la misma UV, y un límite máximo de altura por mesh o
  por material para construir bounds conservadores;
- variante de material/pipeline explícita: la ruta normal no debe pagar recursos
  ni divergent branches de shell;
- `hit position`, normal y profundidad de la intersección, si se quiere que el
  depth buffer represente el shell y no la malla base.

La última viñeta es una inferencia de integración: si el PS no escribe la
profundidad del hit y los datos derivados, z-occlusion, postprocesos,
motion-vectors y sombras seguirán describiendo la malla base. Es aceptable para
un efecto puramente decorativo, pero no para prometer desplazamiento estructural.
Es necesario probarlo, no asumirlo.

## Restricciones de assets

Silhouette/Prism POM no es adecuado como switch universal sobre cualquier
`.rmesh`:

- **Meshes cerrados/manifold:** son el candidato más seguro para una envolvente;
  los bordes y UV seams necesitan una política explícita. Un shell de triángulos
  adyacentes sin continuidad puede abrir grietas.
- **Meshes abiertos, thin sheets, foliage y ropa:** por defecto se rechazan.
  La extrusión por normal puede auto-intersectar, exponer tapas falsas o romper
  el orden de profundidad. Sólo entran con un modo de authoring que declare
  grosor, bordes o una envolvente específica.
- **UV:** sin UV no hay height field; UVs degeneradas, solapadas sin intención o
  escalas texel/metro incoherentes deben bloquear la ruta, no producir NaN,
  parpadeo o una silueta impredecible.
- **Altura:** la escala máxima debe ser pequeña respecto del tamaño local y
  grosor del mesh. No se usa para crear cuevas, overhangs arbitrarios, rocas
  jugables o colisión: esos casos pertenecen a SDF/mesh authoring y cook.
- **Mips y compresión:** Height continúa siendo Linear, rojo normalizado y
  `BC4`/equivalente de máscara según perfil; los mips y las muestras a distancia
  deben validarse visualmente contra desaparición de detalle y popping.

## LOD, artefactos y rendimiento

Crytek es directo: SPOM cuesta más que POM, es más propenso a artefactos y debe
usarse sólo cuando la silueta se vea. Sus controles oficiales respaldan dos
reglas Jerico: limitar pasos por calidad del height map y aumentar muestras a
ángulos rasantes sólo bajo un máximo. El donor de DiligentFX ya ofrece el patrón
de pasos 4..64 y fade de distancia/ángulo para POM; se conserva para M6F.

Política propuesta para una futura ruta de silueta:

- `Off` por defecto; POM/MicroDisplacement son los LODs inferiores.
- Sólo aparece dentro de una distancia y tamaño de pantalla declarados por el
  material/asset; fuera de ellos cae de forma estable a POM u Off, nunca a una
  geometría de colisión diferente.
- No habilitarla en instancing masivo, vegetación, transparencia, skinned meshes
  o meshes con UV inválidos hasta que un benchmark específico lo apruebe.
- Cortar el modo antes de ángulos numéricamente inestables y registrar el motivo
  en diagnostics; no ocultar el fallback.
- Mantener sombras de luces, shadow maps, depth prepass y motion vectors como
  **no compatibles por defecto**. Cada uno requiere una prueba que compare el
  hit del shell con el pass correspondiente. Sin esa prueba, el material usa la
  profundidad/sombra de la malla base y se etiqueta decorativo.

### Gates de benchmark obligatorios

Antes de adoptar una variante productiva se necesita una escena fuente-cero con
los tres modos (`Off`, POM, MicroDisplacement, candidate shell) y matrices
equivalentes de 1/8/64 instancias, cámara frontal/rasante, 1080p/1440p/4K y
altura baja/media/alta. Debe registrar por GPU objetivo:

1. ms GPU de base, depth, shadow y color por separado;
2. fragment invocations, overdraw, draws, triángulos/prismas, VRAM y bandwidth;
3. diferencia visual contra un mesh densamente desplazado de referencia,
   incluyendo borde, auto-oclusión, shadow, SSR/AO, TAA/FXAA y motion;
4. estabilidad al mover cámara, cambiar mip, LOD, luz y FOV; y
5. determinismo de cook, `.epak` sin fuentes, reabrir y fallback correcto.

La aprobación no usa una cifra inventada hoy: el candidato debe cumplir un
presupuesto que se fije tras medir el baseline M6F en la misma GPU, ganar una
evaluación A/B perceptible sobre POM justo en los assets donde el borde importa,
y no introducir gaps, depth fighting, shimmering ni errores Diligent. Si no
gana esas tres cosas, se rechaza y se usa POM/MicroDisplacement/SDF según el
caso.

## Decisión recomendada

**Adapt, no adoptar todavía.**

Adoptamos ahora el conocimiento: Silhouette POM es una categoría válida pero
alta, y Prism/shell mapping demuestra por qué la silueta exige una envolvente
geométrica. No adoptamos una implementación concreta durante M6F porque el
slice actual ya tiene una ruta verificable con Height opcional, POM acotado y
displacement real por vértices; mezclar un shell pipeline ahora multiplicaría
riesgo, shader variants, pruebas de depth y tiempo de cierre.

La futura capacidad debe llamarse provisionalmente **`SilhouetteShell`**, no
`SilPOM`, hasta que el prototipo declare exactamente cuál de las variantes
implementa. Será una clase de material separada, alta y explícita, no un checkbox
que se active para todos los Height maps.

### Slice posterior concreto: `M6F-SilhouetteShellCandidate`

Se inicia **sólo después de que M6F Height tenga su prueba Vulkan y `.epak`
fuente-cero**. El slice realiza una única vertical:

1. un cooker no destructivo toma un mesh estático cerrado con UV válidas y
   genera un shell/prism proxy determinista, con diff, bounds, seed/version y
   undo/redo de la operación de authoring;
2. una variante Vulkan raster `VS + PS` consume ese proxy y el mismo `.rtext`
   Height Linear/ScalarMask; no usa GS ni HS/DS en el primer experimento;
3. el PS resuelve el hit del height field, expone modo diagnostics y compara
   profundidad base frente a `SV_Depth` de hit como dos políticas explícitas;
4. tests rechazan mesh abierto, UV degenerada, altura que excede el shell,
   textura sRGB/no-ScalarMask y la activación fuera de la distancia/LOD;
5. la escena de benchmark anterior produce capturas, métricas y un `.epak`
   fuente-cero; y
6. una revisión A/B decide **Adopt** o **Reject** antes de añadirlo al Material
   Workbench general.

Nada de este slice cambia colisión, navegación, SDF, PWAS ni EPLS. Si el nivel
necesita una roca, cueva, overhang o hielo realmente jugable, se modela/genera
como geometría o SDF y se cocina: SilhouetteShell sólo añade detalle visual.

## Fuentes primarias consultadas

- [CryEngine — Parallax Occlusion Mapping](https://www.cryengine.com/docs/static/engines/cryengine-5/categories/23756816/pages/29450125)
- [CryEngine — Silhouette POM](https://www.cryengine.com/docs/static/engines/cryengine-5/categories/23756816/pages/29450143)
- [Crytek / GDC Vault — The Rendering Technologies of Crysis 3](https://www.gdcvault.com/play/1017626/Advanced-Visual-Effects-with-DirectX)
- [Dachsbacher & Tatarchuk — Prism Parallax Occlusion Mapping with Accurate Silhouette Generation](https://inria.hal.science/inria-00606806)
- [Jeschke, Mantler & Wimmer — Interactive Smooth and Curved Shell Mapping](https://www.cg.tuwien.ac.at/research/publications/2007/JESCHKE-2007-ISC/JESCHKE-2007-ISC-Preprint.pdf)
- [Tatarchuk — Dynamic Parallax Occlusion Mapping with Approximate Soft Shadows](https://doi.org/10.1145/1111411.1111423)
- [DiligentCore oficial](https://github.com/DiligentGraphics/DiligentCore)
- [DiligentSamples Tutorial 08 — Tessellation](https://github.com/DiligentGraphics/DiligentSamples/tree/master/Tutorials/Tutorial08_Tessellation)
- [Pearl Abyss — Crimson Desert patch 1.03](https://crimsondesert.pearlabyss.com/en-US/News/Notice/Detail?_boardNo=81&pubDate=20260411)
- [Pearl Abyss — Crimson Desert patch 1.04](https://crimsondesert.pearlabyss.com/en-us/News/Notice/Detail?_boardNo=84&hl=el-GR)
- Donor local oficial auditado: `F:/penumbram/src/DiligentEngine/DiligentFX/Shaders/PBR/public/PBR_Parallax.fxh` y `F:/penumbram/src/DiligentEngine/DiligentFX/Shaders/PBR/private/RenderPBR.psh`.
