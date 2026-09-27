# Formación de capas: perímetro extremo sin tapas plegadas (2026-09-27)

## Alcance y causa reproducida

Este cambio sólo afecta la creación de capas coherentes en el Playground. El contacto entre estratos ya comparte triángulos, pero una amplitud grande del contorno angular podía invertir triángulos interiores de las tapas aunque los bordes siguieran coincidiendo. El test RED añadió los casos históricos del preset 11: amplitud 0,28/seed 1 y 0,22/seed 12 a resolución final 24, más 0,30/seed 1. `2026-09-27-layer-perimeter-red-ctest.log` falló en el primer caso: triángulo de contacto 3696, estrato 0, orientación +0,159822 para una cara que debía mirar hacia abajo. El fallo no dependía del bake high→low.

La documentación oficial de [Leadwerks Vertex Panel](https://www.leadwerks.com/learn/vertexpanel) describe una máscara que desactiva displacement en vértices compartidos por caras visibles para evitar grietas de teselación. Es una inspiración para conservar interfaces; no documenta un soldado universal de mallas. Aquí el par de tapas conserva posiciones y triángulos coincidentes, y el nuevo límite aborda además las inversiones interiores que un simple soldado de aristas no corrige.

## Corrección

`coherentLayerFormation` conserva los resultados normales cuando todas las caras de contacto etiquetadas mantienen su orientación. Si la amplitud pedida pliega una tapa, construye una referencia con amplitud de perímetro cero y reduce **sólo esa deformación** con un único factor común para todos los estratos y para el warp propagado. El área proyectada firmada de cada triángulo durante esa interpolación es cuadrática; la búsqueda comprueba el mínimo en todo el intervalo y deja margen del 1 % antes de aplicar el factor. Si incluso la referencia está plegada, la operación falla explícitamente. La receta conserva el valor pedido; la geometría puede usar menos amplitud para mantener el contacto.

## Verificación

- Los tres casos RED ahora producen sólidos cerrados, contactos por triángulo idénticos, asentamiento repetido menor que 1e-6 y replay exacto de receta. Un test directo con `sourceWarp` no nulo comprueba que el warp propagado mantiene emparejados los horizontes.
- Amplitud máxima 0,30: 32 seeds a resolución 24 pasaron cierre, triángulos de contacto y asentamiento sin desplazamiento extra. `2026-09-27-layer-perimeter-sweep-ctest.log` pasó el test focal; `2026-09-27-layer-perimeter-final-ctest.log` pasó **17/17** tests Release. Build final: `2026-09-27-layer-perimeter-close-vulkan-build.log`.
- `2026-09-27-layer-perimeter-close-vulkan.log` registra salida 0 con Diligent/Vulkan en NVIDIA GeForce RTX 3090. `2026-09-27-layer-perimeter-close/` guarda recetas, hashes y capturas originales **1280×960** en dos ángulos de los casos 0,16/seed 1, 0,28/seed 1, 0,22/seed 12 y 0,30/seed 1. En la vista neutral, 0,28 conserva un contorno distinto de 0,16 sin dejar estratos flotantes. Una ejecución midió 38,36 ms de CPU para 0,16/seed 1 y 41,20 ms para 0,28/seed 1; son tiempos individuales, no un presupuesto de rendimiento.
- Luna revisó en lectura el área cuadrática, la búsqueda, continuidad de contactos y cobertura; no encontró un defecto crítico restante en ese guard. La primera revisión detectó una búsqueda no monótona, corregida antes de la verificación final.

## Límite

La protección comprueba las tapas de contacto etiquetadas. No demuestra ausencia de inversiones en caras laterales ni autointersecciones globales, y la silueta aún parece una losa estratificada. Tampoco reproduce exactamente la imagen original, cuya receta no se conoce. La calidad Beauty geológica permanece **PARTIAL**; no se ha retomado high→low ni modificado `F:\JericoEngine`.
