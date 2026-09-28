# Spline Construction: ruta de daño compartida (2026-09-28)

Estado: **incremento técnico verificado; Beauty e interacción manual pendientes**.

El barrido cerrado de Spline Construction admite reglas `PathDamageRule` con ID estable y `DamagePathCutter` común. La receta conserva la fuente: habilitar, deshabilitar, mover o quitar el proyector regenera la pieza. El corte Boolean sólo añade topología alrededor de la grieta, registra `DamageInterior`, transfiere las UV de superficie y genera islas UV únicas para bake. El laboratorio permite elegir el área, ajustar centro, radio, profundidad, cantidad, irregularidad, separación y seed, además de moverla con Shift+drag. La extracción de transferencia UV a `DamageUvTransfer` mantiene el mismo camino para assemblies.

## Evidencia

- RED: [test previo](../2026-09-28-spline-shared-path-red-test.log) rechazó la ausencia de corte compartido.
- Release: [build final](../2026-09-28-spline-shared-path-build-final.log), [prueba focal](../2026-09-28-spline-shared-path-test-final.log) y [CTest 26/26](../2026-09-28-spline-shared-path-ctest.log). Tras el último cambio de valores predeterminados y cámara se recompiló el ejecutable y pasó la [captura final](../2026-09-28-spline-shared-path-capture-final.log); el modelo y la prueba focal no cambiaron.
- [Prueba Vulkan 1280×960](../spline-construction/spline-construction-proof.json): 930 vértices, 1.856 triángulos, 24 islas bake; las tres vistas del mismo daño conservan el hash de malla `5462317554085830082`. Se revisaron en resolución original [Lit](../spline-construction/shared-crack-lit.png), [Wireframe](../spline-construction/shared-crack-wire.png) y [Bake UV](../spline-construction/shared-crack-bake-uv.png). Las 13 imágenes existentes no cambiaron.
- La prueba comprueba sólido cerrado, interior fresco, replay, desplazamiento fuera de la pieza, desactivación, UV de bake y marcadores de costura supervivientes. El revisor detectó que los marcadores se borraban tras el Boolean; la corrección quedó incluida antes del build final.

## Límites

La grieta se distingue en wireframe y checker UV, pero la vista iluminada aún parece casi intacta: **Beauty no aceptada**. El primer cutter del preset de 40 mm atravesaba demasiado la pieza y falló el cierre; se redujeron sus parámetros predeterminados y la captura final pasó. No se probó manualmente el Shift+drag en ventana. La prueba cubre el barrido circular recto; quedan perfiles, curvaturas y casos extremos. El viejo `breakEnabled` aún coexiste con la ruta compartida. Este incremento no acredita paridad de Edge Damage, Infinite Fracture, aging ni todos los demás destructores entre consumidores.
