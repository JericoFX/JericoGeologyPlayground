# PlaneCut: A/B acotado de constraints muestreadas y analíticas

Fecha: 2026-09-05. Base: `3c77f8ab1e21e8cebefbfdea957a6243c1833afe`.
Estado: **CONFIRMADO LOCALMENTE; diagnóstico cerrado, M8 PARTIAL**.
Autorización: el usuario pidió ejecutar una prueba aislada, sin tocar producción,
y cerrar la investigación después de un resultado medido. No autoriza provenance
general, otro mesher, cambios de schema, ImplicitBlock ni modificaciones de LOD.

## Dictamen solicitado

1. **Confirmado:** al sustituir sólo root/normal muestreados por una constraint
   analítica validada, el mismo QEF y Surface Nets recuperan planitud. No queda
   demostrado que éste sea el defecto principal de la roca F4 ni de todo M8.
2. **Defecto explicado:** pérdida de precisión geométrica al reunir constraints
   desde un campo de corte muestreado, especialmente cerca de cambios de operando
   dominante. Existe además suavizado en las normales de salida, que no son las
   normales analíticas entregadas al QEF. No explica la ausencia de fracturas,
   bloques independientes o el rechazo de LOD del miembro real.
3. **Reparación mínima compatible:** para un corte exacto terminal que realmente
   use extracción sampled, validar el cruce contra la fuente previa y entregar
   root/normal exactos al gather existente. Un único plano no necesita provenance
   volumétrica. Las rutas productivas de corte nativo/Hermite exacto ya preservan
   esos cortes y no deben reemplazarse por este diagnóstico. En F4 con desgaste
   posterior, el plano anterior NO es automáticamente la frontera final: este
   resultado no autoriza reimponerlo ni deshacer el desgaste. Ninguna reparación
   de producción se implementó aquí.
4. **Siguiente RED real ejecutable de F4:** el LOD del miembro con desgaste sigue
   rechazando 112.727115° frente a 35°. Inspeccionar el par y su vecindad antes y
   después de la finalización, distinguiendo geometría, normales y correspondencia.
   No cambiar LOD, presupuesto o umbrales para aceptar el fallo. La Beauty
   estructural y los cuatro productos siguen pendientes; este ensayo no los cierra.

## Aislamiento y controles

- Nuevo ejecutable `JericoGeologyPlaneCutAbProbe`, Test y EXCLUDE_FROM_ALL. Enlaza
  Diligent/Vulkan, no la sesión productiva de geología ni el WIP de caché.
- Consume `geologySurfaceNetsShader` del repositorio. En memoria sustituye un único
  punto de gather, comprobando que el texto aparece exactamente una vez. No cambia
  ningún archivo de shader, función de QEF ni implementación de conectividad.
- A/B comparten el mismo shader compilado. Un selector privado del test decide
  si aplicar la constraint; `nativeConstraintCount` permanece igual, para impedir
  un cambio oculto de solver. Ambas ramas llaman al solve bounded existente.
- Caja convexa: centro `(0.03,-0.02,0.05)`, semiejes `(0.83,0.71,0.67)` metros.
  Corte: normal `normalize(1,0.63,0.31)`, offset 0.19 m. Sin máscara, roughness,
  transformación ni operador posterior. No hubo búsqueda de presets/semillas.
- El host construye y sube una única fixture escalar `max(boxScalar, planeScalar)`
  con saturación ±4h. No ejecuta la cadena de generación productiva. La extracción
  sí ejecuta en Vulkan el shader real. Es un recorrido diagnóstico sampled-QEF;
  no se etiqueta A como la ruta productiva de corte nativo de F4.
- h=0.12 m, 64 hojas de 512 muestras, layout lineal, stride1 en ambas ramas.
  Sólo malla densa; sin LOD adicional, remesh posterior ni cambio de resolución.
- Se detectan los mismos cambios de signo. El oracle verifica el cruce del plano
  y que la caja previa tenga scalar < -1.2e-5 m en el root. Fallback para costura,
  exterior o ausencia de cruce. Una caja convexa evita ambigüedad de cruces múltiples.
- No cambia el campo entre A/B. La posición entregada a Hermite se convierte a
  coordenadas locales de celda; la normal está normalizada. Los buffers nuevos
  existen sólo en el ejecutable diagnóstico; no hay estado runtime permanente.

## Resultado medido

RTX 3090, Vulkan/Diligent. Ambos resultados: **828 vértices, 1652 triángulos**.
Mismas celdas activas y misma conectividad comprobada mediante IDs de celda,
independientemente del orden de asignación GPU. Cero triángulos degenerados según
el umbral diagnóstico de área 1e-12 m². Esto no demuestra ausencia de solapamientos.

| Métrica | A sampled | B analítico validado |
| --- | ---: | ---: |
| Distancia RMS al plano, 290 vértices de celdas con cruce de corte | 22.1965 mm | 1.1844 mm |
| Distancia máxima en ese conjunto | 93.1978 mm | 7.9946 mm |
| Distancia RMS, 222 celdas cuyos cruces son todos del corte | 11.1439 mm | 0.0000168 mm |
| Normal geométrica RMS, 382 triángulos del conjunto anterior | 12.0773° | 0.0000194° |
| Normal de shading RMS en esos 222 vértices | 21.1557° | 13.7629° |
| Distancia RMS al borde analítico más cercano, 68 celdas mixtas | 52.8848 mm | 31.5480 mm |
| Tiempo GPU mediano de extracción, instrumentado | 853.264 µs | 855.376 µs |

Reducción del error RMS al plano: **94.66%**. El interior recupera planitud, pero
**la costura no queda resuelta**: en el conjunto completo de cara aún hay normales
geométricas con error máximo 139.52°. No ocultar esa cola mediante promedios o una
captura. Las constraints de fuente/costura que hacen fallback siguen muestreadas.

Los grupos se determinan desde las celdas de entrada y el oracle de la caja, nunca
seleccionando sólo los vértices que salieron planos. «Todos los cruces del corte»
no significa que los ocho corners estén dominados por el plano; ese grupo puede
estar cerca de la costura y tener constraints trilineales contaminadas en A.

Cobertura: **1020 sustituciones de 3304 constraints de edge** (30.87%). Los 1020
candidatos válidos se sustituyeron; 2284 fallbacks: 76 cercanos a costura, 24 fuera
de la caja y 2184 sin cruce del plano. Son constraints por celda/edge, no edges
globales únicos. Memoria volumétrica extra: **0 bytes**. Extensión del contador
del test: 32 bytes en A y B; selector reutiliza constantes no usadas en el probe.

El tiempo suma las dos pasadas GPU, excluye compilación, upload y readback. Se
calientan ambas ramas y se alterna orden A/B durante ocho pares. La diferencia
de 2.112 µs incluye instrumentación y no demuestra un coste productivo significativo
ni un presupuesto completo de M8. Los tiempos no son una medición cold-start.

## Geometría y shading separados

[Comparación 1400×1280](images/m8-f4-planecut-ab-01/comparison.png): izquierda A,
derecha B. Arriba normales geométricas por triángulo; abajo normales de salida del
shader interpoladas. Cámara ortográfica, luz y material neutro idénticos.
Es una **visualización offline de las mallas extraídas en Vulkan**, no una captura
del renderer Jerico ni aceptación Beauty/source-free. No usa texturas ni assets.

El código de salida conserva `surfaceFieldNormal(voxelPosition)`: reconstruye un
gradiente filtrado del campo. Sustituir la constraint del QEF no sustituye esa
normal; esto explica por qué B puede ser plano y seguir pareciendo suavizado.
La finalización productiva de otras rutas recalcula normales por región: tampoco
se puede extrapolar esta salida de shader a todos los materiales/runtime de M8.

Datos conservados: [A.ply](images/m8-f4-planecut-ab-01/A.ply),
[B.ply](images/m8-f4-planecut-ab-01/B.ply),
[métricas completas](images/m8-f4-planecut-ab-01/metrics.json),
[resumen de ejecución](images/m8-f4-planecut-ab-01/run-summary.txt).

La revisión READ_ONLY no encontró defectos bloqueantes. Señaló que la distancia
al borde busca el segmento analítico más cercano globalmente, sin demostrar
correspondencia con la costura de cada celda. Se conserva como indicador auxiliar,
no como prueba de preservación del borde ni criterio del dictamen. La selección
de grupos usa cálculo float64 frente a float32 en GPU: limita la exactitud de
clasificación junto a empates; no justifica extrapolar a otros casos.

## Verificación y condición de parada

- Configuración del build existente y compilación del target diagnóstico: exit0.
- Control nulo `--null-control`: **exit1 esperado**, cero sustituciones y cambio
  geométrico cero. Demuestra que el criterio no acepta una intervención ausente.
- A/B real: exit0, sin errores Diligent en el log final.
- Analizador independiente de métricas/conectividad y vistas: exit0.
- Criterio fijado antes de leer resultados: error A >0.1 mm, reducción RMS ≥50%
  y al menos 32 celdas mixtas. Cumplido; no se cambiaron parámetros ni umbrales.
- Fallos de montaje corregidos antes del control válido: colisión del nombre
  BIND_FLAGS con Windows y rebind requerido después de WaitForIdle. No son RED
  de geología ni se cuentan como falsación del experimento.

Reproducción desde el checkout con el build configurado:

```powershell
cmake --build build/windows-vulkan-editor --config Debug --target JericoGeologyPlaneCutAbProbe --parallel 4
& ./build/windows-vulkan-editor/Tests/ProceduralAuthoring/Debug/JericoGeologyPlaneCutAbProbe.exe ./build/windows-vulkan-editor/planecut-ab
python Tests/ProceduralAuthoring/AnalyzePlaneCutAb.py build/windows-vulkan-editor/planecut-ab
```

El analizador requiere NumPy/Pillow ya disponibles en el Python empaquetado de
Codex; no se añade una dependencia del engine. Para repetir el control nulo, usar
otro directorio de salida y `--null-control`; su exit1 es intencional.

Auditoría de integración: el único producto nuevo es evidencia diagnóstica.
No cambia fuente de assets, cooker, colisión, renderer, runtime, LOD, streaming,
editor/UI, scripting ni formatos. No se pierde ni cambia semántica productiva.
Se conservan el WIP de caché y los cambios ajenos. **No generalizar el probe ni
iniciar otra ronda de tuning. Hipótesis acotada cerrada. M8 sigue incompleto.**
