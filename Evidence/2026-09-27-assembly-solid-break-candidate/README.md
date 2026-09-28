# Rotura compartida de miembros sólidos

Este slice aplica el cortador volumétrico compartido a pilotes, riostras, postes, rieles de baranda, balaustres, largueros, torres, colgantes y anclajes. Las barras inclinadas se cortan en sus propios ejes. La selección de piezas de truss nuevas usa IDs únicos; los archivos de daño v1–v5 conservan los IDs compartidos históricos, mientras que las recetas nuevas usan v6.

## Evidencia

- Compilación Release completa: salida 0.
- CTest Release: **25/25 PASS**. El fixture prueba volumen retirado, dos fragmentos cerrados y seleccionables, interior fresco, UV, replay, preservación de piezas no seleccionadas y una riostra diagonal de truss. Un caso RED previo reprodujo la colisión de IDs y el cambio de selección de recetas antiguas.
- Vulkan, NVIDIA GeForce RTX 3090, 1280×960: 41 escenas capturadas dos veces; **41/41 PNG idénticos** y **41/41 pares de hashes de malla y selección idénticos**. El JSON conserva las 41 mediciones. Entre las 36 escenas previas, 35 PNG no cambiaron; sólo cambió `broken-railing.png`. Se conservan esa captura y cinco nuevas; las imágenes duplicadas y la segunda ronda se retiraron tras compararlas.
- Revisión independiente de Luna: sin regresión concreta en la rotura ni en la compatibilidad v1–v6. Revisión estática; la compilación y las pruebas indicadas arriba las ejecutó Sol.

## Límite visual

Los cortes de poste, pilote, tirante, riel superior y hanger crean pérdida de volumen real, pero sus interiores y separación todavía parecen mecánicos. No prueban envejecimiento de material, chips, escombros ni activación física de fragmentos. La Beauty y el objetivo transversal de daños siguen **PARCIALES / ACTIVE**.
