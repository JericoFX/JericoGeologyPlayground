# Candidate8 · snapshot de código experimental

Se conserva aparte de candidate6 y candidate7, sin reemplazarlos. Incluye fuentes, CLI, visor, recetas, herramientas, tests y licencias del snapshot cerrado el 2026-10-06. No se presenta como una variante promovida o aprobada visualmente.

Compilar: `bash build_direct.sh`.

Ejemplo independiente: `build/rocks_cli generate --recipe recipes/alveolar-48-comparison.json --out out/prueba`.

Los recolectores comparativos usan las carpetas hermanas `lab-families-expansion` y `lab-slate-candidate7`; primero hay que compilar y regenerar sus controles. Este respaldo conserva código; los informes históricos, manifiestos, imágenes y grandes salidas quedan fuera. No se ha ejecutado una nueva batería de tests durante el respaldo.
