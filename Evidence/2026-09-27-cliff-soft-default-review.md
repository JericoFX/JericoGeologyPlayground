# Corte suave predeterminado del cliff — 2026-09-27

## Alcance y decisión

La captura del owner muestra una placa ovalada que sobresale de la pared. `CliffCutLoop` con perfil 0 duplicaba el borde y creaba triángulos `CutFace` casi perpendiculares al frente. En el preset 8, el perfil 2 ya existente desplaza los vértices de la misma cuadrícula y usa una caída gradual. Se eligió ancho 1,30 m tras comparar 0,35/0,70/1,30 m con los perfiles 2 y 5. No se alteró la matemática de los perfiles, el Engine ni el trabajo high→low. Las recetas serializadas con perfil 0 siguen reproduciendo el corte clásico.

La [documentación de Leadwerks sobre teselación](https://www.leadwerks.com/community/blogs/entry/2779-smart-tessellation/) describe el control de desplazamiento en bordes para evitar grietas entre parches. Eso apoya preservar continuidad en bordes compartidos, pero no justifica soldar todos los vértices de material o región. En este caso el defecto específico era la pared física añadida por el perfil 0.

## Comprobación

- RED: `Evidence/2026-09-27-cliff-soft-default-red-test.log` falló porque el preset todavía generaba pared lateral. El mismo test pasó tras cambiar el preset (`Evidence/2026-09-27-cliff-soft-default-green-test.log`).
- El corte nuevo conserva índices y número de vértices de la superficie previa, mantiene la región `CutLoop` y elimina `CutFace`. A resolución 32 el retroceso máximo es 0,3591 m y la máxima diferencia de retroceso entre vértices vecinos es 0,1648 m. La malla cerrada valida y la receta serializada reproduce su hash.
- En el perfil métrico 5, ampliar el ancho de 0,35 a 1,30 m redujo el salto junto al contorno de 0,3918 a 0,0438 m, sin mover la pared distante ni perder el relieve central. Se conserva como opción; la captura del perfil 2 se integró mejor al preset sin cambiar la geometría existente.
- Build Release: `Evidence/2026-09-27-cliff-soft-final-build.log`, exit 0. Suite completa: `Evidence/2026-09-27-cliff-soft-final-ctest.log`, **17/17**. El test de reducción de pared física fija ahora explícitamente el perfil 0 para conservar su contrato original.
- Vulkan RTX 3090, imágenes originales 1280×960: `Evidence/2026-09-27-cliff-soft-default-vulkan/`; log `Evidence/2026-09-27-cliff-soft-default-vulkan.log`. A resolución 64, el corte clásico tiene 8746 vértices / 17488 triángulos y el nuevo 8450 / 16896. Los tiempos de generación CPU de una captura fueron 54,29 y 54,31 ms; no demuestran una mejora de rendimiento.
- Los controles v19–v21 conservaron los siete hashes de malla y 28 PNG idénticos (`Evidence/2026-09-27-cliff-soft-v21-comparison.json`). El smoke extremo conservó nueve hashes y cuatro PNG de cliff (`Evidence/2026-09-27-cliff-soft-extreme-comparison.json`).

## Límite visual

En `classic-64-view-1.png` la placa tiene una pared lateral visible. En `soft-default-64-view-1.png` esa pared desaparece y las bandas de la pared entran en la zona modificada. Aún se lee una zona ovalada y hay transiciones locales marcadas, especialmente cerca del centro y con material neutro. **Beauty: PARTIAL**; faltan macroforma, juntas y material para que el resultado parezca una formación geológica natural. El experimento de extender el empuje también hacia fuera del contorno produjo saltos peores que el perfil métrico existente, por lo que se descartó.
