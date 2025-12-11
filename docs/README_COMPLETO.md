````markdown
# Proyecto Cocina — Documentación completa

Última actualización: 2025-12-10

Resumen
-------
Este documento complementa las instrucciones mínimas de la profesora y centraliza: diseño hardware, mapeo de pines, guía de compilación y subida, pasos para simulación en Proteus y notas de diseño del software (máquina de estados, ISR, estructura de datos).

Contenido
---------
- Introducción y objetivo
- Requisitos hardware y software
- Mapeo de pines (referencia a `CONEXIONES_ARDUINO.md`)
- Compilar y subir (Arduino IDE / `arduino-cli`)
- Simulación en Proteus (referencia a `GUIA_PROTEUS_PASO_A_PASO.md`)
- Funcionamiento del firmware (estados, eventos, RFID, cola de comandas)
- Pruebas y depuración
- Estructura del repositorio y consejos antes de push

Introducción y objetivo
-----------------------
El sistema implementa la toma de pedidos para una mesa de restaurante con:
- Interfaz cliente: LCD 16x2 y teclado matricial 4x4
- Gestión de múltiples clientes por mesa
- Envío de pedidos a cocina por Serial (cola FIFO)
- Control en cocina con dos botones: seleccionar y marcar listo
- Notificación al cliente con display 7‑segmentos y LED alarma
- Pago y recarga mediante tarjeta RFID (MFRC522)

Requisitos
----------
- Arduino Mega 2560 (hardware objetivo)
- Librería MFRC522 instalada
- Arduino IDE o `arduino-cli` para compilación
- Proteus (opcional) para simulación; ver `GUIA_PROTEUS_PASO_A_PASO.md`

Mapeo de pines
--------------
Usar `CONEXIONES_ARDUINO.md` como fuente única. Breve resumen:
- LCD 4-bit: pines `LCD_RS`, `LCD_EN`, `LCD_D4..D7`
- Teclado 4x4: filas `10..13`, columnas `26..29`
- SPI (MFRC522): MOSI=51, MISO=50, SCK=52, SS=`RFID_SS`(53), RST=`RFID_RST`(5)
- Botones cocina: `BTN_SELECCIONAR`=2 (INT0), `BTN_LISTO`=3 (INT1)
- Display 7‑seg: pines `14..20`
- LED alarma y buzzer: `LED_ALARMA`, `BUZZER_PIN`

Compilar y subir
-----------------
Con Arduino IDE:
1. Abrir `Proyecto_cocina.ino` en la IDE.
2. Seleccionar placa: *Arduino Mega or Mega 2560*.
3. Seleccionar puerto COM. Subir.

Con `arduino-cli`:
```bash
arduino-cli compile --fqbn arduino:avr:mega Proyecto_cocina
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:mega Proyecto_cocina
```
(En Windows, reemplazar `/dev/ttyACM0` por `COMx`.)

Simulación en Proteus
---------------------
Seguir la guía `GUIA_PROTEUS_PASO_A_PASO.md`. Notas clave:
- Use el mapeo exacto de pines en `CONEXIONES_ARDUINO.md`.
- Evite usar PCINT para filas del teclado en Proteus; el firmware está preparado para escaneo periódico (tick) que funciona mejor en simulación.

Diseño del firmware
-------------------
- Timer1 CTC configurado para tick de 1 ms — ISR mínima que empuja eventos `EVT_TICK_MS`.
- INT0/INT1 para botones cocina; ISRs muy pequeñas que solo marcan flags y empujan eventos.
- Teclado: escaneo periódico desde contexto principal cada 50 ms para debounce y compatibilidad Proteus.
- Estructuras: cola circular para comandas (`colaCocina`), lista de pedidos para facturación (`listaPedidos`).
- RFID: máquina de estados para lectura/escritura segura (bloque 4). Evitar bloqueos largos dentro de ISRs.

Pruebas y depuración
---------------------
- Habilitar trazas Serial en el sketch si necesitas ver eventos y valores en Proteus.
- Verifica `g_msTicks` y que el `EVT_TICK_MS` esté llegando (puedes imprimir cada 1000 ms).
- Si el teclado no responde en Proteus: revisar pull-ups y columnas; usar el escaneo periódico.

Estructura del repositorio
--------------------------
- `Proyecto_cocina/Proyecto_cocina.ino` — código fuente principal
- `CONEXIONES_ARDUINO.md` — wiring
- `GUIA_PROTEUS_PASO_A_PASO.md` — cómo simular
- `GUIA_RFID_PAGO_RECARGA.md` — instrucciones RFID
- `build/` — artefactos (ignorar)
- `readme.md` — instrucciones dadas por la cátedra (no editar sin permiso)

Checklist antes de subir (GitHub)
--------------------------------
- [ ] Confirmar que `Proyecto_cocina.ino` compila sin errores.
- [ ] Ejecutar una simulación Proteus y confirmar flujos críticos: teclado, envío a cocina, pago RFID.
- [ ] Eliminar binarios y resultados de build (carpeta `build/`).
- [ ] Añadir `.gitignore` (ya incluido en este repo).

Notas finales
------------
Mantener `readme.md` (instrucciones de la profesora) exactamente como está; cualquier ampliación o guía propia debe ir en `README_COMPLETO.md` o en `README.md` en la raíz. Si quieres, puedo crear `CHANGELOG.md` y la plantilla `ISSUE_TEMPLATE` ahora.

````
