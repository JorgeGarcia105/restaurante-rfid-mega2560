````markdown
# 💳 GUÍA RFID - Sistema de Pago y Recarga

## 📋 Descripción General

El sistema RFID permite:
- **Recargar saldo** en tarjetas MIFARE
- **Pagar pedidos** descontando del saldo
- **Consultar saldo** al acercar la tarjeta

---

## 🔧 Hardware RFID RC522

### Conexiones
```
RC522 Pin  →  Arduino Mega
─────────────────────────────
SDA/SS     →  Pin 53
SCK        →  Pin 52
MOSI       →  Pin 51
MISO       →  Pin 50
RST        →  Pin 49
IRQ        →  Pin 18 (opcional)
GND        →  GND
VCC        →  3.3V ⚠️ IMPORTANTE
```

### ⚠️ Advertencia
El módulo RC522 funciona a **3.3V**. Conectar a 5V puede dañarlo.

---

## 📊 Estructura de Datos en Tarjeta

El saldo se guarda en el **Bloque 4** (Sector 1) de la tarjeta MIFARE:

```
Bloque 4 (16 bytes):
┌────────┬────────┬────────┬────────┬────────────────────┐
│ Byte 0 │ Byte 1 │ Byte 2 │ Byte 3 │ Bytes 4-15 (libre) │
└────────┴────────┴────────┴────────┴────────────────────┘
    └──────────────────┬──────────────────┘
                 Saldo (uint32_t)
                 Little-endian
                 Máximo: $4,294,967,295
```

---

## 💰 FUNCIÓN: RECARGAR TARJETA

### Flujo de Operación

```
┌─────────────────────────────────────────────────────────┐
│                    MODO RECARGA                         │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. Usuario presiona [*] en menú principal              │
│                    ↓                                    │
│  2. LCD muestra: "MODO RECARGA"                         │
│                  "Acerque tarjeta"                      │
│                    ↓                                    │
│  3. Usuario acerca tarjeta RFID                         │
│                    ↓                                    │
│  4. Sistema lee saldo actual                            │
│     LCD: "Saldo: $XXXXX"                                │
│           "Ingrese monto..."                            │
│                    ↓                                    │
│  5. Usuario ingresa monto con teclado (0-9)             │
│     LCD: "Monto: $XXXXX"                                │
│           "==OK  C=Cancelar"                            │
│                    ↓                                    │
│  6. Usuario presiona [=] para confirmar                 │
│                    ↓                                    │
│  7. LCD: "Acerque tarjeta"                              │
│          "para grabar..."                               │
│                    ↓                                    │
│  8. Sistema escribe nuevo saldo                         │
│     nuevoSaldo = saldoActual + montoIngresado           │
│                    ↓                                    │
│  9. LCD: "RECARGA OK!"                                  │
│          "Saldo: $XXXXX"                                │
│                    ↓                                    │
│ 10. Vuelve al menú principal                            │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Código de la Función

```cpp
// ==================== MODO RECARGA ====================

// Paso 1: Entrar en modo recarga (tecla *)
void procesarMenuPrincipal(char tecla) {
  switch (tecla) {
    case '*':  // Modo recarga
      estadoActual = MODO_RECARGA;
      mostrarModoRecarga();
      break;
    // ... otras teclas
  }
}

// Paso 2: Mostrar pantalla de recarga
void mostrarModoRecarga() {
  lcd_clear();
  lcd_setCursor(0, 0);
  lcd_print("MODO RECARGA");
  lcd_setCursor(0, 1);
  lcd_print("Acerque tarjeta");
}

// Paso 3-4: Cuando se detecta tarjeta en modo recarga
void procesarRFID() {
  if (!leerSaldoRFID()) {
    return;  // No hay tarjeta o error
  }
  
  switch (estadoActual) {
    case MODO_RECARGA:
      // Mostrar saldo actual
      lcd_clear();
      lcd_print("Saldo: $");
      lcd_printNum(saldoTarjeta);
      lcd_setCursor(0, 1);
      lcd_print("Ingrese monto...");
      delay(1500);
      
      // Preparar para ingreso de monto
      montoIngresado = 0;
      digitosIngresados = 0;
      estadoActual = INGRESANDO_MONTO;
      mostrarIngresoMonto();
      break;
    // ... otros casos
  }
}

// Paso 5: Mostrar pantalla de ingreso
void mostrarIngresoMonto() {
  lcd_clear();
  lcd_setCursor(0, 0);
  lcd_print("Monto: $");
  lcd_printNum(montoIngresado);
  lcd_setCursor(0, 1);
  lcd_print("==OK  C=Cancelar");
}

// Paso 5-6: Procesar teclas durante ingreso
void procesarIngresoMonto(char tecla) {
  // Ingresar dígitos (0-9)
  if (tecla >= '0' && tecla <= '9') {
    if (digitosIngresados < 6) {  // Máximo $999,999
      montoIngresado = montoIngresado * 10 + (tecla - '0');
      digitosIngresados++;
      mostrarIngresoMonto();
    }
  }
  // Confirmar con tecla =
  else if (tecla == '=') {
    if (montoIngresado > 0) {
      uint32_t nuevoSaldo = saldoTarjeta + montoIngresado;
      
      // Paso 7: Pedir tarjeta para grabar
      lcd_clear();
      lcd_print("Acerque tarjeta");
      lcd_setCursor(0, 1);
      lcd_print("para grabar...");
      
      // Paso 8: Esperar tarjeta y escribir
      unsigned long timeout = millis() + 10000;  // 10 segundos
      while (millis() < timeout) {
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
          if (escribirSaldoRFID(nuevoSaldo)) {
            // Paso 9: Éxito
            lcd_clear();
            lcd_print("RECARGA OK!");
            lcd_setCursor(0, 1);
            lcd_print("Saldo: $");
            lcd_printNum(nuevoSaldo);
            delay(2500);
          } else {
            lcd_clear();
            lcd_print("Error escritura!");
            delay(2000);
          }
          break;
        }
        delay(100);
      }
      
      // Paso 10: Volver al menú
      montoIngresado = 0;
      digitosIngresados = 0;
      estadoActual = MENU_PRINCIPAL;
      mostrarMenuPrincipal();
    }
  }
  // Cancelar con tecla C
  else if (tecla == 'C') {
    montoIngresado = 0;
    digitosIngresados = 0;
    estadoActual = MENU_PRINCIPAL;
    mostrarMenuPrincipal();
  }
  // Borrar último dígito con tecla -
  else if (tecla == '-') {
    if (digitosIngresados > 0) {
      montoIngresado /= 10;
      digitosIngresados--;
      mostrarIngresoMonto();
    }
  }
}
```

### Teclas en Modo Recarga

| Tecla | Función |
|-------|---------|
| `0-9` | Ingresar dígitos del monto |
| `=` | Confirmar recarga |
| `C` | Cancelar y volver al menú |
| `-` | Borrar último dígito |

---

## 💵 FUNCIÓN: PAGAR PEDIDO

### Flujo de Operación

```
┌─────────────────────────────────────────────────────────┐
│                      MODO PAGO                          │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. Usuario presiona [-] en menú principal              │
│                    ↓                                    │
│  2. ¿Hay pedido activo?                                 │
│     NO → LCD: "No hay pedido!" → Volver al menú         │
│     SI ↓                                                │
│  3. LCD muestra: "Total: $XXXXX"                        │
│                  "Acerque tarjeta"                      │
│                    ↓                                    │
│  4. Usuario acerca tarjeta RFID                         │
│                    ↓                                    │
│  5. Sistema lee saldo de tarjeta                        │
│                    ↓                                    │
│  6. ¿Saldo suficiente?                                  │
│     │                                                   │
│     ├─── SI ───────────────────────────┐                │
│     │                                  ↓                │
│     │    7a. nuevoSaldo = saldo - total                 │
│     │    8a. Escribir nuevo saldo en tarjeta            │
│     │    9a. LCD: "PAGO EXITOSO!"                       │
│     │             "Saldo: $XXXXX"                       │
│     │   10a. LED parpadea                               │
│     │   11a. Limpiar pedido del cliente                 │
│     │   12a. Volver al menú                             │
│     │                                                   │
│     └─── NO ───────────────────────────┐                │
│                                        ↓                │
│          7b. LCD: "SALDO INSUF!"                        │
│                   "Faltan: $XXXXX"                      │
│          8b. LCD: "C=Cancel *=Recar"                    │
│                        ↓                                │
│          9b. ¿Qué elige usuario?                        │
│              C → Cancelar, volver al menú               │
│              * → Ir a modo recarga                      │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Código de la Función

```cpp
// ==================== MODO PAGO ====================

// Paso 1-3: Iniciar pago (tecla -)
void procesarMenuPrincipal(char tecla) {
  switch (tecla) {
    case '-':  // Pagar
      if (clientesMesa[clienteActual].total > 0) {
        estadoActual = ESPERANDO_PAGO;
        lcd_clear();
        lcd_setCursor(0, 0);
        lcd_print("Total: $");
        lcd_printNum(clientesMesa[clienteActual].total);
        lcd_setCursor(0, 1);
        lcd_print("Acerque tarjeta");
      } else {
        lcd_clear();
        lcd_print("No hay pedido!");
        delay(1500);
        mostrarMenuPrincipal();
      }
      break;
    // ... otras teclas
  }
}

// Paso 4-5: Cuando se detecta tarjeta esperando pago
void procesarRFID() {
  if (!leerSaldoRFID()) {
    return;
  }
  
  switch (estadoActual) {
    case ESPERANDO_PAGO:
      realizarPago();
      break;
    // ... otros casos
  }
}

// Paso 6-12: Procesar el pago
void realizarPago() {
  uint32_t totalAPagar = clientesMesa[clienteActual].total;
  
  // Paso 6: Verificar saldo
  if (saldoTarjeta >= totalAPagar) {
    // ===== SALDO SUFICIENTE =====
    
    // Paso 7a: Calcular nuevo saldo
    uint32_t nuevoSaldo = saldoTarjeta - totalAPagar;
    
    // Paso 8a: Escribir en tarjeta
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      if (escribirSaldoRFID(nuevoSaldo)) {
        
        // Paso 9a: Mostrar éxito
        lcd_clear();
        lcd_print("PAGO EXITOSO!");
        lcd_setCursor(0, 1);
        lcd_print("Saldo: $");
        lcd_printNum(nuevoSaldo);
        
        // Paso 10a: Efecto visual LED
        digitalWrite(LED_ALARMA, HIGH);
        delay(500);
        digitalWrite(LED_ALARMA, LOW);
        
        // Enviar confirmación por serial
        Serial.print("PAGO OK Cliente ");
        Serial.print(clienteActual + 1);
        Serial.print(" $");
        Serial.println(totalAPagar);
        
        delay(2500);
        
        // Paso 11a: Limpiar pedido del cliente
        clientesMesa[clienteActual].total = 0;
        clientesMesa[clienteActual].items = "";
        
        // Paso 12a: Volver al menú
        estadoActual = MENU_PRINCIPAL;
        mostrarMenuPrincipal();
        
      } else {
        lcd_clear();
        lcd_print("Error tarjeta!");
        delay(2000);
      }
    }
  } else {
    // ===== SALDO INSUFICIENTE =====
    
    // Paso 7b: Mostrar error
    lcd_clear();
    lcd_print("SALDO INSUF!");
    lcd_setCursor(0, 1);
    lcd_print("Faltan: $");
    lcd_printNum(totalAPagar - saldoTarjeta);
    delay(2500);
    
    // Paso 8b: Ofrecer opciones
    lcd_clear();
    lcd_print("C=Cancel *=Recar");
    
    // Paso 9b: Esperar decisión
    char resp = 0;
    while (!resp) {
      resp = leerTecla();
      if (resp == 'C') {
        // Cancelar pago
        estadoActual = MENU_PRINCIPAL;
        mostrarMenuPrincipal();
        return;
      } else if (resp == '*') {
        // Ir a recargar
        estadoActual = MODO_RECARGA;
        mostrarModoRecarga();
        return;
      }
      resp = 0;
      delay(50);
    }
  }
}
```

````
