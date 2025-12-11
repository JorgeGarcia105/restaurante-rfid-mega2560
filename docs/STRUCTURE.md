```markdown
# Estructura del repositorio

Breve descripción de las carpetas y archivos importantes.

- `Proyecto_cocina.ino` — Sketch principal (firmware). Punto de partida para compilación.
- `docs/` — Documentación organizada (guías, wiring, Proteus, RFID, CONTRIBUTING, STRUCTURE).
- `CHANGELOG.md` — Historial de cambios.
- `upload.sh` — Script de subida (si aplica).
- `build/` — Artefactos de compilación (no subir al repo; añadir a `.gitignore`).

Prácticas recomendadas
- Mantener la documentación actualizada en `docs/`.
- No incluir binarios ni archivos de Proteus generados en el repo (añadir a `.gitignore`).
- Los cambios del firmware deben acompañarse de notas en `CHANGELOG.md`.

Cómo generar el `.hex` para Proteus
1. Abrir `Proyecto_cocina.ino` en Arduino IDE.
2. `Sketch → Export compiled Binary` (se genera `.hex`).
3. Cargar el `.hex` en Proteus (o en la placa mediante el IDE / `arduino-cli`).

```
