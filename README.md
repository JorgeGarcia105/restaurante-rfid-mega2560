# restaurante-rfid-mega2560

Sistema de toma de pedidos para restaurante implementado sobre plataforma Arduino Mega 2560.

Contenido
- Descripción
- Requisitos de hardware
- Firmware
- Documentación adicional
- Compilación y carga
- Contribuciones

Descripción
Este repositorio contiene el firmware y la documentación para un sistema de toma de pedidos destinado a un entorno de restaurante. El sistema incluye interfaces de usuario (LCD y teclado), pago por RFID, y comunicación con un dispositivo de cocina que gestiona las comandas.

Requisitos de hardware
- Arduino Mega 2560
- Lector RFID MFRC522 (3.3V)
- LCD 16x2
- Teclado matricial 4x4
- Displays de 7 segmentos y botones para la cocina

Firmware
- Archivo principal: `Proyecto_cocina.ino`

Documentación adicional
Las guías extensas y los diagramas de conexión se han centralizado en la carpeta `docs/`:
- Documentación completa: [docs/README_COMPLETO.md](docs/README_COMPLETO.md)
- Conexiones y pines: [docs/CONEXIONES_ARDUINO.md](docs/CONEXIONES_ARDUINO.md)
- Simulación (Proteus): [docs/GUIA_PROTEUS_PASO_A_PASO.md](docs/GUIA_PROTEUS_PASO_A_PASO.md)
- RFID (pago/recarga): [docs/GUIA_RFID_PAGO_RECARGA.md](docs/GUIA_RFID_PAGO_RECARGA.md)
- Instrucciones originales (cátedra): [docs/INSTRUCCIONES_CATEDRA.md](docs/INSTRUCCIONES_CATEDRA.md)
- Registro de cambios: [CHANGELOG.md](CHANGELOG.md)

Compilación y carga
Se recomienda usar `arduino-cli` o el IDE de Arduino para compilar y cargar el firmware. Ejemplo con `arduino-cli`:

```bash
arduino-cli compile --fqbn arduino:avr:mega Proyecto_cocina
arduino-cli compile --fqbn arduino:avr:mega Proyecto_cocina --output-dir build
arduino-cli upload -p COMx --fqbn arduino:avr:mega Proyecto_cocina
```

Notas sobre conexiones rápidas
- MFRC522: utilice 3.3 V para alimentación y asegúrese de no aplicar 5 V al módulo.
- Conexión SPI (ejemplo para Mega): SDA->53 (SS), SCK->52, MOSI->51, MISO->50, RST->5.

Contribuciones
- Abra un `issue` para discutir cambios importantes.
- Envíe `pull requests` pequeños y autocontenidos; incluya descripción y pruebas cuando sea posible.
- Consulte [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) para pautas.

Licencia
Consulte `LICENSE` si existe; si no, contacte al autor para condiciones de uso.

Historial de versiones
Versión actual: `v1.0.0` (etiqueta aplicada al commit que reorganiza la documentación).

