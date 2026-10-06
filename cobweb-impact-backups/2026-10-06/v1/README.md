# Whole-web impact lab · revisión cosmética r3

Prototipo C++17 determinista y offline de una telaraña portadora y una piedra. No utiliza simulación de hilos: un impacto suficiente produce una breve deformación analítica y retira el paño completo una sola vez, desactivando contacto y resistencia. Cuatro restos cosméticos quedan fijos en coordenadas locales de la piedra.

Si `breakable=false` o la puntuación no alcanza el umbral, el paño permanece y la deformación se recupera. El umbral usa unidades artísticas de impacto, no fuerza, energía ni resistencia de seda calibrada. El vuelo de la piedra es una trayectoria programada y los restos no simulan envoltura física.

## Construir y comprobar

Requiere compilador C++17 y Python 3. Blender instalado se usa solamente para imágenes; no se instalan paquetes.

```sh
./scripts/build.sh
./scripts/test.sh
```

CMake se incluye, pero la ruta comprobada es el script de compilación. Las pruebas cubren núcleo, contratos del renderer, composición e integración CLI, incluida la reserva concurrente de salidas.

## Generar y renderizar

```sh
python3 scripts/run_recipe.py recipes/breakable.json --out outputs/my_breakable_run
python3 scripts/run_recipe.py recipes/unbreakable.json --out outputs/my_unbreakable_run
blender -b --threads 3 --python scripts/render_impact.py -- --input outputs/my_breakable_run/02_indent --output renders/my_indent.png
```

Usar siempre carpetas y nombres nuevos. El exportador y el renderer rechazan reemplazar resultados existentes.

## Contrato

El host debe detectar el impacto real antes de llamar `Panel::receiveImpact`. Los IDs de impacto son crecientes; los antiguos o duplicados se ignoran. El paso es fijo a 1/120 s. Consultar `collisionEnabled()` y `resistanceEnabled()` antes de interactuar con el paño; en estado `Detached`, retirar sus caras y su respuesta hasta reset o restauración explícitos.

Un impacto admitido no puede cancelarse ni posponerse por impactos posteriores. La restauración valida el checkpoint de forma atómica. El viento opcional es una deformación analítica acotada, conserva bordes/anclajes y no genera impactos.

## Límites

No hay Jolt, detección general de colisiones, física por fibra, interfaz de motor ni simulación de envoltura. El benchmark incluido mide componentes de CPU aislados y no establece rendimiento de un juego. El trabajo previo por fibras y cohesión permanece en un respaldo WIP separado, sin vincularse a este prototipo.

Este checkpoint conserva fuentes, pruebas, recetas, scripts y NOTICE. Excluye datos de entrega, imágenes, binarios, informes y documentos de investigación. NOTICE conserva la procedencia y no añade condiciones de licencia nuevas.

## Restos cosméticos r3

Los cuatro restos usan bordes irregulares y recortes dispersos del mismo atlas. Conservan el presupuesto de 100 vértices y 96 triángulos; no se añaden fibras físicas ni envoltura simulada. El cambio se limita a los restos exportados y mantiene movimiento de piedra, paño, checkpoints, anclajes y atlas. La suite de integración incluye 17 pruebas.
