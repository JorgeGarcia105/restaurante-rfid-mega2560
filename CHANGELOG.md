# CHANGELOG

Todos los cambios se documentan siguiendo la convención "Keep a Changelog".

## [Unreleased]

### Añadido
- Estados no bloqueantes para entrada de cantidad y scroll de categorías.
- Función `removerPedidoPorNumero()` para mantener `listaPedidos` sincronizada con la cola de cocina.
- `README_COMPLETO.md` con guía completa de wiring, compilación y Proteus.

### Corregido
- Bug en selección de categoría (`catActual` usado correctamente en lugar de valor fijo).

### Pendiente / Por probar
- Validación completa en Proteus (teclado, RFID y botones de cocina).
- Habilitar trazas `DEBUG` opcionales para facilitar tests.

## [v0.1.0] - 2025-12-10

### Añadido
- Versión inicial refactorizada para Arduino Mega 2560.
- Soporte básico: teclado 4x4 (escaneo periódico), LCD 16x2, cola de comandas, RFID (MFRC522), 7‑seg y alarmas.
- Documentación: `readme.md` (instrucciones de la cátedra), `README_COMPLETO.md`, `CONEXIONES_ARDUINO.md`, guías Proteus/RFID.

### Notas de release
- Este release marca el punto de partida; se recomienda ejecutar pruebas en Proteus antes de desplegar a hardware real.

## Cómo versionar / publicar

1. Actualizar la sección `[Unreleased]` con los cambios que se incluirán en la nueva versión.
2. Cambiar el encabezado `## [Unreleased]` por `## [vX.Y.Z] - YYYY-MM-DD` con la nueva versión y fecha.
3. Añadir una nueva sección `## [Unreleased]` vacía para siguientes cambios.

Ejemplo de comando para tag y push:

```bash
git tag -a v0.1.1 -m "Pequeñas correcciones y pruebas Proteus"
git push origin --tags
```
