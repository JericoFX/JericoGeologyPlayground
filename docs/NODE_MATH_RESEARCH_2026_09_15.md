# Investigación acotada: nodos -> matemáticas para BSP -> geología

Fecha: 2026-09-15 UTC. Ventana iniciada 02:59:47; búsqueda cerrada antes de 03:09:47 (máximo pedido: diez minutos). Investigación amplia y acotada, no una afirmación de haber agotado todos los ejemplos de Internet. Fuentes: manuales oficiales, tutorial de sus autores e inventario local de grafos previamente auditados. No se ejecutó Blender ni se copió código de addons.

## Evidencia de grafos reales

El inventario `BLENDER_ADDON_NODE_INVENTORY.md` de la review previa registra: Granite usa Extrude Mesh, Subdivide Mesh, Subdivision Surface, Noise y Set Position; Igneous añade Edge Angle y Voronoi; Sedimentary usa Separate/Combine XYZ, Math, Voronoi, varios Set Position y subdivisión; Cracker usa Proximity, Raycast, Boolean, Capture/Store Named Attribute y extrusión; Stalactite/Stalagmite usan Cone, extrusión, ruido y subdivisión. Que un nodo aparezca no demuestra que cualquier fórmula propuesta aquí reproduzca el addon. Las siguientes fórmulas son diseños propios apoyados en la semántica pública.

## Catálogo y traducción

| Nodos / concepto | Matemática utilizable | Aplicación / límite |
|---|---|---|
| Grid; Position | `p(u,v)=origin+width*(u-.5)*U+height*(v-.5)*V` | Plano de cliff; superficie abierta. [Grid](https://docs.blender.org/manual/ka/4.5/modeling/geometry_nodes/mesh/primitives/grid.html). |
| Vector Math Dot, Normalize; Separate XYZ | `s=dot(p-origin,normalize(axis))` | Coordenadas de estratificación independientes del género o mundo. [Vector Math](https://docs.blender.org/manual/en/latest/modeling/geometry_nodes/utilities/vector/vector_math.html). |
| Set Position; Vector Math Scale/Add | `p'=p+mask*amplitude*field*direction` | Deformación real; requiere densidad de vértices. [Set Position](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/geometry/write/set_position.html). |
| Noise / fBM | `sum(gain^i*N(lacunarity^i*p))/sum(gain^i)` | Detalle multiescala. Reutilizar FBM de Jerico. [Noise](https://docs.blender.org/manual/nl/4.3/modeling/geometry_nodes/texture/noise.html). |
| Absolute, Subtract, Power; ridged noise | `r_i=(1-abs(N_i))^2`, suma ponderada; opcional modulación por octava previa | Aristas y canales. Variante propia, no equivalencia exacta con Musgrave. [Ejemplos fBM/turbulencia/ridged](https://thebookofshaders.com/13/). |
| Turbulence / Absolute | `sum(gain^i*abs(N_i))/sum(gain^i)` | Señal no negativa con valles; no física de fluidos. [Musgrave](https://docs.blender.org/manual/en/2.80/render/shader_nodes/textures/musgrave.html). |
| Wave Bands, Phase, Distortion | `s=dot(axis,p)/period+distortion*N(p); wave=.5+.5*sin(2*pi*s)` | Estratos ondulados. Distorsión mueve coordenadas, no transforma una máscara en sólido. [Wave](https://docs.blender.org/UATEST/manual/en/4.5/modeling/geometry_nodes/texture/wave.html). |
| Floor, Fraction, Ping-Pong | `k=floor(s); u=s-k; triangle=1-abs(2*u-1)` | Banda categórica y perfil periódico. `floor` debe funcionar también con coordenadas negativas. [Math](https://docs.blender.org/manual/en/latest/modeling/geometry_nodes/utilities/math/math.html). |
| Map Range; Smooth/Smoother Step | `t=clamp((x-a)/(b-a)); H3=t*t*(3-2*t); H5=t^3*(t*(6*t-15)+10)` | Remapeo de influencia y transiciones continuas. [Map Range y ejemplo](https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/utilities/math/map_range.html). |
| Float Curve / Vector Curves | `y=(1-factor)*x+factor*C(x)`; curva por componente para vectores | Perfil de terrazas o crestas; versión inicial puede usar perfiles analíticos acotados. [Float Curve](https://docs.blender.org/manual/en/4.3/modeling/geometry_nodes/utilities/math/float_curve.html), [Vector Curves](https://docs.blender.org/manual/id/4.2/render/shader_nodes/vector/curves.html). |
| Geometry Proximity + Map Range | `d=min distance(p,triangle); influence=1-H3(d/radius)` | Protección/transición cerca de bordes o guías. Distancia no firmada. El manual contiene ejemplos con planos. [Proximity](https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/geometry/sample/geometry_proximity.html). |
| Raycast | `p_hit=p+t*d`, mínimo `t>=0` sobre triángulos | Proyección/espesor aproximado. Un miss debe ser explícito. [Raycast](https://docs.blender.org/manual/nl/4.5/modeling/geometry_nodes/geometry/sample/raycast.html). |
| Sample Nearest Surface; Sample Index | Proyección a triángulo y pesos baricéntricos; valores categóricos por selección, no promedio | Ya usado para transferencia en los cortes; no da campos interiores exactos. [Sample Nearest](https://docs.blender.org/UATEST/manual/en/4.5/modeling/geometry_nodes/geometry/sample/sample_nearest.html). |
| Capture / Store Named Attribute | Guardar campo en su dominio antes de deformar | Mantener coordenadas fuente, región, estrato y máscara. La reevaluación de Position después de cada operación cambia el campo. [Fields](https://docs.blender.org/manual/en/latest/modeling/geometry_nodes/fields.html). |
| Blur Attribute | `x_i'=(x_i+w*sum(x_j))/(1+w*degree_i)` usando una copia de la iteración anterior | Suavizar máscara; distinto de Relax de geometría. [Blur y ejemplo de plano](https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/attribute/blur_attribute.html). |
| Edge Angle | `acos(clamp(dot(n1,n2),-1,1))` | Selección de crestas; depende de densidad de malla, no equivale directamente a curvatura física. [Edge Angle](https://docs.blender.org/manual/en/5.1/modeling/geometry_nodes/mesh/read/edge_angle.html). |
| Extrude / Solidify | Duplicar `p` con offsets `a*t*d` y `(a-1)*t*d`; invertir caras traseras y unir aristas de borde | Superficie -> sólido; validar orientación, bordes y cierre. [Solidify](https://docs.blender.org/manual/nl/4.5/modeling/modifiers/generate/solidify.html), [Extrude](https://docs.blender.org/manual/id/3.6/modeling/geometry_nodes/mesh/operations/extrude_mesh.html). |
| Mesh Boolean | `A\B`, `A union B`, `A intersect B` | Cueva/grieta geométrica. Usar Manifold existente, conservar caras interiores. [Boolean](https://docs.blender.org/manual/en/4.4/modeling/geometry_nodes/mesh/operations/mesh_boolean.html). |
| Normal + Dot + ramp | `coverage=H3(dot(n,depositDirection))*noiseMask` | Acumulación de hielo/nieve/costras: operación reutilizable, sin simular congelación. |
| Distribute Points on Faces; Random Value | Elegir triángulo por área; baricéntricas con `sqrt(u)`; semilla estable | Crecimientos distribuidos; separación mínima sería un contrato adicional. [Distribución y ejemplos](https://docs.blender.org/manual/en/3.3/modeling/geometry_nodes/point/distribute_points_on_faces.html). |
| Curve Line; Spline Parameter; Set Curve Radius; Curve to Mesh | `c(t)=p0+t*L*d`, `r(t)=r0*(1-t)^gamma`; barrido circular | Icicles y estalactitas artísticas. Instancias/componentes separados hasta una unión solicitada. [Curve to Mesh](https://docs.blender.org/manual/en/4.5/modeling/geometry_nodes/curve/operations/curve_to_mesh.html). |

## Decisión de implementación

Prioridad vertical: entrada de malla/selección BSP real con GUID -> plano/pared de prueba -> relieve estratificado y ridged -> solidificación validada -> cortes -> crecimiento/acumulación reutilizable. Mantener selección/input inmutables, preservar zonas no seleccionadas y definir Surface/Solid explícitamente. No introducir un segundo motor procedural desconectado: todas las entradas alimentan el mismo evaluador.

El usuario seleccionó esta dirección como backend M8. Eso registra la decisión; las integraciones de cook, materiales, colisión, transacciones de editor y replay siguen necesitando pruebas antes de declarar M8 terminado.
