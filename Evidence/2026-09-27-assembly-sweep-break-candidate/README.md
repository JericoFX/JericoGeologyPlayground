# Corte compartido de tubo, fascia y cables

El estado Broken de PipeBody, Fascia, Cable y HandRope ya genera la pieza completa y después aplica el mismo cortador de trayectoria que los demás miembros. El cuerpo del tubo conserva la geometría del barrido normal antes del corte. El hueco fijo de `SplineConstruction` sigue disponible para el laboratorio de splines, pero deja de ser la ruta de estos cuatro roles de assemblies.

## Verificación

- Antes del cambio, el fixture falló en `PipeCouplings/PipeBody_000`: había hueco, pero no fractura del backend común.
- Release completo y **25/25 CTest PASS**. Para PipeBody, Fascia, Cable de suspensión, Cable de puente de cuerda y HandRope, el fixture comprueba volumen retirado, dos fragmentos cerrados y seleccionables, interior fresco, UV, replay y conservación de las demás piezas. Las pruebas antiguas de huecos ahora exigen geometría separada real y no una reducción arbitraria del número de triángulos.
- Vulkan en NVIDIA GeForce RTX 3090, 1280×960: **46/46 PNG idénticos** entre dos ejecuciones y **46/46 pares de hashes de malla/selección idénticos**. Las 41 escenas anteriores quedaron byte a byte iguales. Se retienen sólo las cinco capturas nuevas; el JSON conserva los 46 casos medidos.
- Revisión estática independiente de Luna: sin regresión concreta de Bent/Detached, selección o corte.

## Límite visual

El espacio y las tapas de la rotura son visibles, pero las superficies siguen siendo lisas y de color uniforme. Aún faltan material interior propio, deshilachado de cuerdas, chips/escombros y activación física. **Beauty PARCIAL; objetivo global ACTIVE.**
