# Revisión de entrada — dossier de geología del 2026-09-26

**Estado:** EN REVISIÓN. Esta entrada incorpora investigación documental al Playground; no aprueba una dependencia, una arquitectura nueva, un preset visual ni una capacidad M8. El objetivo activo sigue priorizando bakes high→low, mapas de superficie y calidad visual de rocas, cliffs y estratos. `F:\JericoEngine` se mantuvo en solo lectura; no se ejecutó Blender ni se usó OpenVDB.

## Procedencia de la versión revisada

El `main` local del Playground estaba en `103f7adcf99894a3d13c85a6b710859039cec030` y sin cambios. El `main` remoto se adelantó con tres commits del 2026-09-26; se hizo `git fetch origin main` y avance rápido hasta `1d7859b939287baee817e582d6cd2e38c69d8b8a`. Los tres archivos nuevos quedaron en `docs/`, sin reemplazar la copia documental congelada de `docs/engine-reference-2026-09-24/`:

| Archivo | Commit de incorporación | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `docs/JERICO_GEOLOGY_RESEARCH_2026_09_26.md` | `dd88346` | 91.507 | `951B81079CB62BE1CFFA14BE5F2436CACC0DFBC9F121C76A06F5F393A1B752D0` |
| `docs/JERICO_GEOLOGY_RESEARCH_2026_09_26.html` | `cf3a7b4` | 110.422 | `54C758BBA70E0B50811BAE629F198D967920C96C45B4C29CF0C4DF4D58A02300` |
| `docs/JERICO_GEOLOGY_SOURCES_2026_09_26.json` | `1d7859b` | 34.999 | `F44E674F7A0447258E5FDF262944F240E6AE445E132FBDA8F8A37B1E16D3DA55` |

La autoridad M8 antigua que inicialmente parecía candidata, `JERICOENGINE_M8_BLENDER_HEADLESS_GEOLOGY_AUTHORITY_DIRECTIVE.md`, ya estaba copiada en el Playground: origen y copia tienen 178.661 bytes y SHA-256 `AA633BC6C02BECCF7D3B0D15085AF14EDBD6ADC9DF96DFA274773EC95C387920`. El documento nuevo es el dossier del propio Git del Playground, acompañado de HTML y catálogo; no hay que sobrescribir aquella referencia.

## Qué aporta el dossier y qué puede probarse aquí

| Propuesta documental | Relación con el C++ y la evidencia actual | Revisión pendiente |
| --- | --- | --- |
| Separar límite sólido, coordenada de estrato, fase/material, juntas, durabilidad, daño y espacio protegido (§7.1). | `LayerFormation.cpp` ya comparte horizontes de contacto; `Model.cpp` conserva operaciones ordenadas y regiones. La propuesta puede servir como contrato para interiores y materiales sin duplicar ruido. | Una clasificación idéntica en el corte, el high y cada LOD aún necesita una prueba focal con identidades estables; no inferirla de colores de `region-id.png`. |
| Deformación común, espesores positivos, gradiente por la regla de la cadena y fallas cronológicas (§§7.2–7.4). | Hay estratos coherentes y `VectorNoise` local; los tests y capturas previos prueban contactos de presets concretos. | El exponencial de espesores, la inyectividad del warp y la cronología de fallas son modelos propuestos, no resultados ejecutados en este checkout. Primero fijar tolerancias y casos RED. |
| Juntas finitas, superficies de fractura pareadas y separación explícita entre grieta cerrada/abierta (§§7.5–7.8). | Existen cortes/experimentos de fractura, pero la evidencia local no admite piezas con soporte/colisión transaccional. | Probar finitud de la junta, superficies compartidas, topología e interiores antes de llamarlo fractura física. La fórmula `max(φ_roca, −φ_corte)` presupone la convención interior negativo y un volumen de corte definido. |
| Desgaste guiado por material, concavidad y exposición (§§7.9–7.10). | `SurfaceFeatures.cpp` ya calcula curvatura firmada, concavidad, convexidad, contacto y AO. La Beauty actual del cliff y estratos sigue PARTIAL. | Un experimento acotado sobre un mismo high/low podría comparar pérdida de volumen, silueta, contactos, regiones y capturas neutrales. El dossier no aporta un filtro listo para copiar ni un resultado visual aceptado. |
| Mallado Hermite, invalidación local y grafo de soporte (§§7.12–7.14). | Son criterios útiles para futuras pruebas de geometría y publicación; no sustituyen los bakes ni el mallador/cook existente. | Exigen casos de topología subvoxel, fronteras y publicación transaccional. Son trabajo M8/producto o investigación posterior según owner y autoridad del slice. |
| Presupuestos y pruebas por etapa (§§8–11). | El dossier distingue compilación, runtime, Beauty, rendimiento y fidelidad física, en línea con los límites ya registrados en `GOAL.md`. | Los 8/64 MiB de 128³/256³ por canal son aritmética, no memoria medida en Jerico. No extrapolar velocidades ni porcentajes. |

**Prioridad de este Goal:** el dossier estudia sobre todo estructura, erosión y fractura. No trae un algoritmo high→low nuevo que justifique modificar `NormalBake.cpp` o `AutoUv.cpp` ahora. El Playground ya tiene diez mapas, UV automática y cuatro niveles de mips con pruebas focales; sigue faltando aceptación Beauty/material/filtrado y presupuesto representativo. La interoperabilidad RBrush tiene PASS matemático local y PARTIAL de editor/cook/runtime según `Evidence/2026-09-25-rbrush-current-engine-compatibility.md`; el dossier no cambia ese resultado. Antes de adoptar una idea estructural, elegir una hipótesis visual concreta y mantener idénticos high/low, cámara, materiales y gates afectados para atribuir la mejora.

## Hallazgos de revisión documental

1. **Procedencia interna consistente.** El catálogo declara 80 fuentes y contiene 80 IDs/anclas únicos; las 101 referencias de tipo `[P01](#p01)` en Markdown resuelven. El HTML contiene las 80 anclas y las 80 URL del JSON. Esto verifica enlaces internos y sincronía básica, no las 80 afirmaciones externas.
2. **Muestra externa contrastada.** La [ficha de PhaseTree de los autores](https://perso.liris.cnrs.fr/apeytavi/website/publication/hal-05713515/) describe fases múltiples y ray tracing; no certifica mallas/colisión para Jerico. La [documentación oficial de Unreal Mesh Terrain](https://dev.epicgames.com/documentation/unreal-engine/mesh-terrain-in-unreal-engine) confirma su estado experimental, túneles/voladizos y la separación preview/runtime. El [repositorio de pySimFrac](https://github.com/lanl/pySimFrac) confirma superficies de fractura sintéticas y licencia BSD-3-Clause. El resto del catálogo no se revalidó fuente por fuente en esta revisión.
3. **No elevar propuestas a autoridad.** El dossier se autodeclara investigación; su referencia J01 consultó solo el primer tramo de un índice del Engine. La prioridad del usuario para este Goal y las autoridades vigentes del Playground/Engine gobiernan cualquier cambio. La sección de addons propone comparaciones con herramientas Blender, pero la instrucción actual prohíbe ejecutarlo o descargar addons para esta tarea.
4. **Frase histórica potencialmente confusa.** La última sección dice «No se modificó el repositorio». Era el alcance de la investigación original, pero el dossier sí fue publicado después en tres commits. La frase debe leerse como registro del proceso de investigación, no como estado actual de Git. El dossier tampoco auditó el C++ vigente; `docs/M8_GEOLOGY_BACKEND.md` y la autoridad M8 copiada describen estados anteriores y no deben sustituir la inspección de código y evidencia actuales.

## Dictamen y siguiente gate

**APTO COMO REFERENCIA DE INVESTIGACIÓN; IMPLEMENTACIÓN Y ACEPTACIÓN PENDIENTES.** No se encontró en este intake una corrección C++ obligatoria ni una razón para alterar los bakes existentes. La siguiente prueba útil para el objetivo visual sería seleccionar una sola relación del dossier —por ejemplo durabilidad/estrato frente al desgaste de un cliff—, fijar un control del mismo high/low y verificar geometría, diez mapas, mips, regiones, repetibilidad y Beauty original. Esa propuesta es un candidato, no un slice ni un preset aprobado. La integración M8, el renderer y los consumidores de Engine siguen fuera de esta revisión.
