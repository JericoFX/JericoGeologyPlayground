# Assets de Fab para probar los algoritmos (2026-11-07)

El pedido del propietario: usar mallas de Fab/Megascans como banco de pruebas de los algoritmos
internos (arquetipos, cajones, sockets, splines). Este documento dice **qué falta**, **por qué** y
**cómo entra al repositorio**.

## Regla de oro: el repositorio es público

`JericoFX/JericoGeologyPlayground` es público y la licencia de Fab **no permite redistribuir** el
asset crudo. Por eso:

- Los binarios van a `Assets/Fab/<vendor>/<asset>/`, que ya está **fuera de git** (`Assets/` no se
  commitea).
- Se versiona sólo un manifiesto: id, ruta, bytes, SHA-256, licencia, URL de Fab y fecha de descarga.
- Las capturas de evidencia pueden mostrar el asset transformado; el asset fuente no viaja.

## Cómo traerlos (dos caminos)

**A. Manual (5 minutos, funciona hoy).** Descargar de Fab con la cuenta de Epic, descomprimir y dejar
la carpeta en `Assets/Fab/...`. Yo escribo el manifiesto y el ingest.

**B. Automático (requiere tu login una vez).** Epic no publica API abierta de Fab, pero existen dos
clientes que usan el OAuth de Epic Games:
- `egs-api-py` (Python) — sirve en Windows.
- `StarksLabs/epic-fab` (CLI nativa de Linux) — referencia de cómo se autentica y descarga.
En ambos casos hace falta **tu** authorization code / device auth; no puedo autenticarme por vos.
Si te sirve, armo el ingest con `egs-api-py` y vos corrés el login una sola vez.

## Lista de lo que me gustaría probar (por capability, no por nombre)

| # | Qué necesito | Para qué | Tamaño objetivo |
|---|---|---|---|
| 1 | **Humanoide** sin rig (escaneo o estilizado) | distinguir bípedo de cuadrúpedo frontal y proponer esqueleto | < 200k tri, GLB/FBX |
| 2 | **Cuadrúpedo** (perro/caballo) | contraste contra el bípedo con el mismo algoritmo | < 200k tri |
| 3 | **Mueble con cajones** (kit modular de cocina/armario) | validar la firma de escalón y la reconstrucción de profundidad/carrera contra geometría real, no sintética | 1 unidad, GLB |
| 4 | **Puerta con bisagra visible** (industrial/metálica) | barrido de bisagra con holgura realista | 1-2 unidades |
| 5 | **Kit de muros modulares** (paneles/pilares) | muros por spline con juntas y daño seleccionable | 3-5 piezas |
| 6 | **Pasarela / catwalk modular** | consumidor de línea B (barandas, tablas, uniones) | 3-5 piezas |
| 7 | **Rocas/riscos escaneados** (Megascans) | `ScanPrior`: estratos, cavidades, silueta para los generadores | 3-4 rocas 1K |
| 8 | **Tubería/válvula con extremos limpios** | inferencia de sockets contra ground truth (el elbow actual tiene extremos sucios) | 2-3 piezas |

Prioridad si hay que elegir: **3 → 1 → 2 → 8**. El 3 desbloquea justo lo que hoy está probado sólo
contra geometría sintética; el 1 y el 2 desbloquean la línea de huesos, que es la única sin ninguna
malla real.

## Qué NO necesito

- Riggeado: el playground crea los rigs. Un humanoide **sin** rig es mejor banco de pruebas.
- 8K: 1K/2K alcanza; el repositorio no guarda texturas.
- Packs gigantes: una unidad representativa por familia sirve más que 50 variantes.

## Qué haré con cada uno (trazabilidad)

Cada asset entra con un gate: se clasifica, se guarda la evidencia en `Evidence/` y el resultado
queda en `GOAL.md`. Si un asset nuevo **contradice** un umbral, se ajusta el umbral con la medición,
no se descarta el asset.
