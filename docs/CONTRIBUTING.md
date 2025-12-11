```markdown
# Contribuir al proyecto

Gracias por contribuir. Estas son las reglas mínimas para facilitar revisiones y mantener la calidad.

1. Reportar issues
- Usa la sección de 'Issues' del repositorio para bugs, mejoras o preguntas.
- Incluye: pasos para reproducir, plataforma (Windows/Linux), versión del firmware si aplica, y logs relevantes (Serial).

2. Flujo de trabajo para pull requests
- Crea una rama con nombre: `feat/<breve-descripción>` o `fix/<breve-descripción>`.
- Haz commits pequeños y atómicos. Usa mensajes descriptivos en inglés o español.
- Abre un PR contra `main` y describe: objetivo, archivos modificados y pruebas realizadas.

3. Estilo de código
- El firmware es Arduino/C++. Mantén estilo consistente: indentación de 2 o 4 espacios según el archivo existente.
- Evita cambios de formato masivos en archivos donde no toques lógica.

4. Compilación y pruebas
- Antes de abrir PRs, asegúrate de que `Proyecto_cocina.ino` compila localmente en Arduino IDE o con `arduino-cli`.

5. Seguridad y hardware
- No incluyas claves, credenciales ni binarios (por ejemplo, `.hex`) en PRs.
- Si añades dependencias externas, documenta la versión y la fuente.

6. Licencia
- Respeta la licencia del repositorio (si no hay, consulta al autor antes de añadir código con licencia externa).

7. Preguntas
- Si no estás seguro sobre cambios grandes, abre un Issue describiendo la propuesta antes de implementar.

```
