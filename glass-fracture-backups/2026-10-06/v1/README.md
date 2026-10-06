# Glass Fracture Study · checkpoint v1

Generador independiente de hueco y grietas para una ventana industrial de paño único. Conserva el marco, el rebaje y los sockets; la geometría del vidrio sí cambia. No es una simulación física de fractura ni una declaración de calidad AAA.

El ejemplo aprobado mantiene el hash de asset `4f88e227270cfe881b35d823e45591b51d34e9847652ec49c69087504923646f` tras las correcciones de valores por defecto y semillas. El hueco es geométrico y los fragmentos tienen volumen y apoyo en el marco.

## Uso

Núcleo Python estándar; Blender 4.3.2 para adaptador, exportación y render. Pillow se usa sólo para la comprobación independiente de píxeles GLB.

```sh
bash scripts/check_all.sh
blender -b --python-exit-code 1 --python render_glass_fracture.py
blender -b --python-exit-code 1 --python render_glass_fracture.py -- --clay
blender -b --python-exit-code 1 --python export_glass_fracture.py
```

API: `glass_fracture.build_fracture(recipe)`. Parte de las recetas industriales incluidas y admite `glazing` intact/broken/empty, semilla, punto de impacto normalizado, intensidad artística y cantidad de radios.

El constructor acorta grietas o activa un fallback radial conservador cuando es necesario evitar solapes, dejando esa decisión en los metadatos. Rechaza configuraciones que no puede construir con seguridad.

## Verificación y fidelidad

El checkpoint registra 176 casos. La copia aislada omite un diagnóstico que necesita un GLB histórico no incluido; Python normal omite además las pruebas de Blender. Esas omisiones no deben contarse como pruebas ejecutadas.

Se conserva el fix del exportador que mantiene los índices de material por cara en las copias temporales de bake. Los tests verifican materiales de frente y canto; el verificador adicional inspecciona píxeles del GLB real.

La auditoría registrada de 120 configuraciones no es una prueba exhaustiva. El GLB usa transmisión uniforme 0,96, mientras que el shader de autoría puede responder de forma distinta en el canto. No se promete equivalencia óptica entre motores. OBJ requiere JSON para recuperar jerarquía, sockets y pivotes.

Sólo se respaldan fuentes, pruebas, recetas y este README. Los renders, modelos exportados, referencias, addons originales, manifiestos y datos Library quedan fuera. Los checkpoints anteriores no se sustituyen.
