# Evidencia seleccionada para el repositorio privado

El repositorio conserva el código, las referencias documentales M8 copiadas con SHA-256, el checkpoint vigente, las fotos USGS y texturas Poly Haven CC0 con sus manifiestos, y la prueba visual más reciente de las seis familias. Los builds y los aproximadamente 7 GB de paquetes intermedios de `Evidence/` permanecen sólo en el playground local y se pueden regenerar; su ausencia en Git no significa que los bakes estén aceptados para M8.

La corrida reproducible más reciente está identificada por `specialist-mips-input-artifact-hashes.json`. Incluye el resumen `2026-09-24-specialist-mips-six-family-lods/rock-lod-bakes-proof.json`, la auditoría de los 17 bakes/mips, el comparador visual de 17 LOD, los logs RED/GREEN/Release y capturas originales `high.png`, `low.png`, `low-normal.png`, AO, contact, cavity y convex-edge para cada LOD válido. Las bases/mips PNG de textura y OBJ completos se regeneran localmente.

Desde el directorio raíz, con el build Release configurado y sus dependencias M8 de lectura disponibles:

```text
build/Release/JericoGeologyPlayground.exe --auto-uv-rock-lod-bakes-test Evidence/2026-09-24-specialist-mips-six-family-lods
python Tests/RockLodBakeAudit.py Evidence/2026-09-24-specialist-mips-six-family-lods Evidence/specialist-mips-six-family-lods-audit.json --require-auto-uv --require-specialist-mips
python Tests/RockLodVisualAudit.py Evidence/2026-09-24-specialist-mips-six-family-lods Evidence/specialist-mips-six-family-lods-visual-audit.json
```

El comparador de imágenes conserva los SHA-256 de High y Low+normal, de modo que permite verificar que el nuevo filtrado de mapas no cambió la macroforma. Las imágenes actuales aún muestran el parche liso del cliff y los estratos repetitivos. No se ejecutó Blender ni se modificó JericoEngine.
