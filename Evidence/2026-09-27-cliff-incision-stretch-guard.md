# Incisión extrema del cliff: aristas acotadas (2026-09-27)

## Reproducción y decisión

En el Playground, `CliffDrainage` desplaza la malla abierta en profundidad. La receta válida con amplitud máxima **0,50** y ancho mínimo **0,10** sobre el prefijo del preset 8 alargaba una arista **2,62490×** respecto a la malla previa. El test RED `2026-09-27-cliff-drainage-extreme-red-run.log` falló en ese límite medido. Una amplitud de 5 fue descartada por la validación de receta antes de esta prueba; no se amplió el rango autorizado.

La [documentación de Leadwerks](https://www.leadwerks.com/learn/vertexpanel) desactiva el desplazamiento en vértices de borde compartidos para evitar grietas; su [explicación de teselación](https://www.leadwerks.com/community/blogs/entry/2779-smart-tessellation/) aclara que esto mitiga costuras, pero no arregla toda discontinuidad de forma. Aquí las aristas de la malla abierta ya comparten índices y los contactos etiquetados de estratos se emparejan por posición. El defecto medido es el alargamiento entre vecinos, que una soldadura adicional no corrige.

El primer ensayo con límite 1,5× evitó el estiramiento, pero redujo la incisión finita v21 ya verificada hasta hacer fallar su profundidad mínima. La medición sin ese límite mostró **1,66107×** para la receta finita normal. Por eso el límite definitivo es **1,75×** de longitud de cada arista respecto a la entrada de esta operación. No se cambió el criterio de profundidad existente. El factor máximo se resuelve de la ecuación cuadrática de longitud de arista para un desplazamiento sólo en Z, se aplica globalmente a todos los vértices y deja 1 % de margen cuando actúa. El campo visualizado informa el retroceso realmente aplicado. Si la petición ya cabe, la geometría queda igual.

## Evidencia verificada

- `PlaygroundSurfaceTests` pasó con **1,74005×** para la incisión continua extrema y **1,73823×** para la finita extrema. Ambas conservan retroceso visible, cierran tras Solidify y repiten exactamente sus recetas. El campo coincide con el desplazamiento aplicado.
- La incisión ordinaria anterior conserva el hash `11640060100807022962`. La variante finita normal mantiene su prueba de profundidad y sus **1,66107×** medidos. `2026-09-27-cliff-drainage-final-surface-run.log` registra el test focal; `2026-09-27-cliff-drainage-full-build.log` salió 0 y `2026-09-27-cliff-drainage-full-ctest.log` pasó **17/17** pruebas Release.
- `2026-09-27-cliff-drainage-vulkan/` guarda las recetas, hashes y cuatro capturas Diligent/Vulkan originales **1280×960** de control y extremo con cámaras frontal y oblicua. `2026-09-27-cliff-drainage-vulkan-run.log` salió 0 en NVIDIA GeForce RTX 3090. La última corrida midió 14,25 ms de generación del control y 14,02 ms del extremo; son muestras aisladas.
- La reejecución de `--cliff-strata-creation-test` con el código revisado produjo los mismos **7 hashes de malla** y las mismas **28 imágenes PNG SHA-256** que `2026-09-26-cliff-finite-wall-candidate/`. `2026-09-27-cliff-drainage-v21-comparison.json` registra cada par; `2026-09-27-cliff-drainage-v21-regression-vulkan.log` salió 0. Esta comparación verifica que el guard no cambió las variantes previas del cliff bajo esas recetas.

## Límite visual y diagnóstico lateral

La captura extrema muestra una incisión estrecha sin aristas explosivas, pero el parche central del corte aún parece una placa separada. La Beauty del cliff sigue **PARTIAL**; este límite no construye juntas ni salientes arraigados y no prueba ausencia global de autointersección.

Antes de este cambio, un diagnóstico de **32 seeds** del preset estratificado 11 comparó caras laterales con perímetro cero: a amplitud normal 0,16 observó coseno mínimo −0,418 y arista máxima 3,503×; a amplitud máxima 0,30, −0,516 y 6,419×. Las capturas originales del seed 24 con ambas amplitudes mostraron una masa continua de aspecto parecido, sin la explosión triangular de la imagen del owner. Esas dos métricas, por sí solas, no distinguen un pliegue real de una ladera inclinada; no se introdujo un límite lateral arbitrario. Registros: `2026-09-27-layer-side-ordinary-run.log` y `2026-09-27-layer-side-diagnostic-run.log`.

No se retomó high→low, no se modificó `F:\JericoEngine` ni se ejecutó Blender. La receta de la imagen original sigue desconocida, así que no se afirma reproducirla exactamente.
