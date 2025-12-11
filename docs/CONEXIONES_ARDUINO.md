````markdown


# 📋 GUÍA DE CONEXIONES - Sistema de Restaurante

## Arduino Mega 2560

---

## 📌 RESUMEN DE COMPONENTES

| Componente | Cantidad | Pines Usados |
|------------|----------|--------------|
| LCD 16x2 (4-bit) | 1 | 6 pines |
| Teclado 4x4 | 1 | 8 pines |
| RFID RC522 | 1 | 6 pines |
| Display 7 Segmentos | 1 | 7 pines |
| Botones Cocina | 2 | 2 pines |
| LED Alarma | 1 | 1 pin |
| Buzzer (opcional) | 1 | 1 pin |

---

## 🔌 CONEXIONES DETALLADAS

### 1️⃣ LCD 16x2 (Modo 4-bits)

```text
LCD Pin    → Arduino Mega    Descripción
────────────────────────────────────────
VSS (1)    → GND             Tierra
VDD (2)    → 5V              Alimentación
V0  (3)    → Potenciómetro   Contraste
RS  (4)    → Pin 24          Register Select
RW  (5)    → GND             Read/Write (GND = siempre escribir)
E   (6)    → Pin 22          Enable
D0-D3      → NC              No conectado (modo 4-bit)
D4 (11)    → Pin 34          Data bit 4
D5 (12)    → Pin 35          Data bit 5
D6 (13)    → Pin 36          Data bit 6
D7 (14)    → Pin 37          Data bit 7
A   (15)   → 5V (con R 220Ω) Backlight ánodo (+)
K   (16)   → GND             Backlight cátodo (-)
```text

---

### 2️⃣ TECLADO MATRICIAL 4x4

```text
Teclado    → Arduino Mega    Descripción
────────────────────────────────────────
Row 1      → Pin 10          Fila 1
Row 2      → Pin 11          Fila 2
Row 3      → Pin 12          Fila 3
Row 4      → Pin 13          Fila 4
Col 1      → Pin 26          Columna 1
Col 2      → Pin 27          Columna 2
Col 3      → Pin 28          Columna 3
Col 4      → Pin 29          Columna 4
```text

---

### 3️⃣ MÓDULO RFID RC522

```text
RC522 Pin  → Arduino Mega    Descripción
────────────────────────────────────────
SDA (SS)   → Pin 53          Slave Select
SCK        → Pin 52          Serial Clock (SPI)
MOSI       → Pin 51          Master Out Slave In
MISO       → Pin 50          Master In Slave Out
RST        → Pin 5           Reset
GND        → GND             Tierra
3.3V       → 3.3V            ⚠️ IMPORTANTE: Solo 3.3V
```text

---

### 4️⃣ DISPLAY 7 SEGMENTOS (Cátodo Común)

```text
Segmento   → Arduino Mega    Descripción
────────────────────────────────────────
a          → Pin 14          Segmento superior
b          → Pin 15          Segmento sup-derecho
c          → Pin 16          Segmento inf-derecho
d          → Pin 17          Segmento inferior
e          → Pin 18          Segmento inf-izquierdo
f          → Pin 19          Segmento sup-izquierdo
g          → Pin 20          Segmento central
GND/COM    → GND             Cátodo común
```text

---

### 5️⃣ BOTONES DE COCINA

```text
Botón          → Arduino Mega    Descripción
────────────────────────────────────────────
BTN_SELECCIONAR → Pin 2         Seleccionar pedido
BTN_LISTO       → Pin 3         Marcar como listo
```text

---

### 6️⃣ LED ALARMA CLIENTE

```
LED Ánodo  → Pin 38 (con R)  Señal de pedido listo
LED Cátodo → GND             Tierra
```

---

### 7️⃣ BUZZER (Opcional)

```
Buzzer (+) → Pin 40          Señal de alerta
Buzzer (-) → GND             Tierra
```

---

## 📊 TABLA RESUMEN DE PINES

| Pin Arduino | Función | Componente |
|-------------|---------|------------|
| **2** | Seleccionar | Botón Seleccionar |
| **3** | Listo | Botón Listo |
| **5** | RST | RFID |
| **10** | Row 1 | Teclado |
| **11** | Row 2 | Teclado |
| **12** | Row 3 | Teclado |
| **13** | Row 4 | Teclado |
| **14** | Seg a | 7 Segmentos |
| **15** | Seg b | 7 Segmentos |
| **16** | Seg c | 7 Segmentos |
| **17** | Seg d | 7 Segmentos |
| **18** | Seg e | 7 Segmentos |
| **19** | Seg f | 7 Segmentos |
| **20** | Seg g | 7 Segmentos |
| **22** | EN | LCD |
| **24** | RS | LCD |
| **26** | Col 1 | Teclado |
| **27** | Col 2 | Teclado |
| **28** | Col 3 | Teclado |
| **29** | Col 4 | Teclado |
| **34** | D4 | LCD |
| **35** | D5 | LCD |
| **36** | D6 | LCD |
| **37** | D7 | LCD |
| **38** | LED | Alarma Cliente |
| **40** | Buzzer | Alerta sonora |
| **50** | MISO | RFID (SPI) |
| **51** | MOSI | RFID (SPI) |
| **52** | SCK | RFID (SPI) |
| **53** | SS/SDA | RFID (SPI) |

---

## 🔋 ALIMENTACIÓN

```
Fuente         → Arduino Mega
────────────────────────────
5V             → VIN o 5V pin
GND            → GND
3.3V           → RFID RC522
```

---

## ✅ CHECKLIST DE CONEXIONES

- [ ] LCD: 6 cables de datos + alimentación
- [ ] Teclado: 8 cables (4 filas + 4 columnas)
- [ ] RFID: 6 cables (SPI + control)
- [ ] 7 Segmentos: 7 cables + GND
- [ ] Botones: 2 cables cada uno (señal + GND)
- [ ] LED: 1 cable + GND (con resistencia)
- [ ] Alimentación: 5V, 3.3V, GND distribuidos

---


---

**Documento actualizado para Proyecto Final - Microprocesadores 2025**

````
