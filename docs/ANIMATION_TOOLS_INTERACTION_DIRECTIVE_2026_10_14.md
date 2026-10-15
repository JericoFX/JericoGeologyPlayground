# JE::AnimationTools — Playground interactivo de animación e IK/FK

**Objetivo principal**

El playground no debe ser solamente un visor de esqueletos ni un editor tradicional de animación.

Debe ser un entorno interactivo donde podamos **crear animaciones, reproducirlas, modificarlas en tiempo real y comprobar cómo interactúan los personajes con objetos físicos mediante IK/FK y GrabPoints**.

Queremos validar estas capacidades antes de integrarlas como herramientas nativas de JericoEngine.

## 1. Animación interactiva

Implementar y probar:

- Crear, editar y borrar keyframes.
- Animar posiciones, rotaciones y controles del rig.
- Timeline con play, pause, scrub y loop.
- Editar una pose mientras la animación continúa.
- Mezclar animación base con correcciones procedurales.
- Activar y desactivar IK por extremidad.
- Cambiar IK→FK y FK→IK durante la reproducción, sin saltos de posición ni rotación.
- Registrar o bakear los movimientos resultantes en nuevas claves.

**Importante:** una animación puede continuar mientras el usuario toma el control de una mano mediante IK.

## 2. Sistema de agarre con IK

Cada objeto interactivo puede contener uno o varios `GrabPoints`.

Cada GrabPoint debe definir:

- ID persistente.
- Transform local al objeto.
- Posición y orientación objetivo de la mano.
- Mano permitida: izquierda, derecha o ambas.
- Tipo de agarre.
- Restricciones de movimiento y rotación.
- Estado libre, ocupado o liberado.

Cuando un personaje agarra un objeto:

1. La mano se aproxima al GrabPoint.
2. El IK resuelve hombro, codo y muñeca respetando límites articulares.
3. La mano se alinea con la orientación del agarre.
4. Se establece una relación de agarre con el objeto.
5. Si el objeto se mueve, el objetivo IK sigue su transform.
6. Al soltar, se conserva la pose actual y se mezcla suavemente con la animación base.

No teletransportar la mano ni modificar directamente el objeto sin una política explícita.

## 3. Cambio dinámico de IK/FK

El IK/FK debe poder cambiar en ejecución, incluso durante un agarre.

Implementar:

- FK puro.
- IK puro.
- Mezcla FK/IK con peso continuo de 0 a 1.
- IK→FK conservando la pose resuelta.
- FK→IK colocando inicialmente el objetivo en el efector actual.
- Límites de articulaciones.
- Pole vectors para orientar codos y rodillas.
- Diagnósticos para objetivos imposibles y singularidades.

El cambio de autoridad debe preservar continuidad de posición y orientación, sin introducir saltos visibles.

## 4. Interacción física

Preparar pruebas con Jolt o con el backend físico disponible en el playground receptor.

Distinguir claramente:

- Animación que mueve al personaje.
- IK que posiciona una extremidad.
- Constraint físico que conecta mano y objeto.
- Física que resuelve el movimiento del objeto.

No crear dos sistemas independientes que intenten imponer simultáneamente transforms incompatibles.

Implementar modos de agarre cinemático y físico, documentando quién tiene autoridad en cada uno.

## 5. Agarre bimanual

Permitir que dos manos agarren un mismo objeto, usando dos GrabPoints.

Pruebas:

- Sostener un tubo con ambas manos.
- Girar una válvula circular.
- Empujar una puerta pesada.
- Mover un objeto manteniendo las dos manos en contacto.

No resolver las dos manos como dos cadenas completamente independientes si comparten pelvis, torso u otros controles.

Detectar restricciones incompatibles y mostrar sus errores.

## 6. Escenas obligatorias del playground

**Escena A — Brazo y objeto**

Personaje simple con un cubo que puede agarrarse, trasladarse, rotarse y soltarse.

**Escena B — Puerta**

Puerta con bisagras y manija. El personaje toma la manija y abre/cierra la puerta. El objetivo IK sigue el movimiento circular de la manija.

**Escena C — Válvula**

Dos manos agarran una rueda y la giran mediante restricciones de articulación.

**Escena D — Animación con intervención**

El personaje reproduce una animación de caminar. Durante la reproducción, una mano entra en IK, agarra un objeto y luego vuelve a FK sin detener la animación.

**Escena E — Criatura no humana**

Usar el fixture de ocho patas existente para probar IK de múltiples extremidades, apoyos y cambios de objetivos durante una animación.

## 7. Interfaz de pruebas

Integrar en el playground existente:

- Viewport 3D y gizmos de huesos/GrabPoints.
- Jerarquía del rig y objetos.
- Timeline y editor de keyframes.
- Inspector IK/FK por extremidad.
- Selector de GrabPoints.
- Controles Grab, Release, IK, FK y Blend.
- Visualización de objetivos, restricciones, errores y contactos.
- Estado de autoridad de cada transform.
- Undo/Redo y almacenamiento de escenas de prueba.

La UI debe permitir probar las interacciones directamente con el ratón.

## 8. Arquitectura y requisitos

- Reutilizar primero los componentes originales de `JericoFX/cascadeur-engine-research`.
- Adaptar los contratos al playground real.
- Namespace propio `JE::`.
- Trabajar únicamente en el playground unificado autorizado.
- No modificar todavía JericoEngine.
- No copiar código, pesos ni recursos propietarios de Cascadeur.
- Mantener la solución independiente de los modelos neuronales investigados.
- Probar los cambios con escenas y registros reproducibles.

## 9. Criterios de aceptación

La primera versión sólo se considera funcional cuando sea posible:

1. Reproducir una animación.
2. Agarrar un objeto durante esa reproducción.
3. Moverlo y comprobar que la mano lo sigue.
4. Cambiar entre IK y FK sin saltos.
5. Soltarlo y recuperar la animación base.
6. Grabar o bakear el resultado.
7. Repetir la prueba con ambas manos.

Medir discontinuidades en posición/orientación, residuos IK, errores de contacto y estabilidad física.

**Prioridad absoluta: interacción real y verificable, no únicamente demostraciones matemáticas o una UI decorativa.**

Entregar código, pruebas, capturas de ejecución y documentación de las funciones comprobadas.