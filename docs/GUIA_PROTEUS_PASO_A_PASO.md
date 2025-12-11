````markdown
# 🔧 GUÍA PASO A PASO - SIMULACIÓN EN PROTEUS
## Sistema de Restaurante - Arduino Mega 2560

---

## 📋 COMPONENTES NECESARIOS EN PROTEUS

Buscar en la librería de Proteus (tecla `P`):

| Componente | Nombre en Proteus | Cantidad |
|------------|-------------------|----------|
| Arduino Mega | ARDUINO MEGA 2560 | 1 |
| LCD 16x2 | LM016L | 1 |
| Teclado 4x4 | KEYPAD-PHONE o KEYPAD-SMALLCALC | 1 |
| Display 7 Seg | 7SEG-COM-CATHODE | 1 |
| LED Rojo | LED-RED | 1 |
| Pulsadores | BUTTON | 2 |
| Resistencias | RES | 8 (220Ω) |
| Potenciómetro | POT-HG | 1 (10KΩ) |
| Terminal Virtual | VIRTUAL TERMINAL | 1 |

> ⚠️ **NOTA:** El RFID RC522 no tiene modelo en Proteus. Se simulará con botones adicionales.

---

## 🚀 PASO 1: CREAR NUEVO PROYECTO

1. Abrir **Proteus Design Suite**
2. **File → New Project**
3. Nombre: `Sistema_Restaurante`
4. Seleccionar carpeta de destino
5. **Next → Create schematic from selected template → DEFAULT**
6. **Next → Do not create PCB layout**
7. **Next → No Firmware Project**
8. **Finish**

---

## 🔍 PASO 2: AGREGAR COMPONENTES

### 2.1 Agregar Arduino Mega
1. Presionar tecla `P` (Pick Devices)
2. Buscar: `ARDUINO MEGA 2560`
3. Doble clic para agregar
4. Colocarlo en el centro del esquema

### 2.2 Agregar LCD 16x2
1. Presionar `P`
2. Buscar: `LM016L`
3. Agregar al esquema
4. Colocar a la izquierda del Arduino

### 2.3 Agregar Teclado 4x4
1. Presionar `P`
2. Buscar: `KEYPAD-SMALLCALC` (tiene distribución 7,8,9...)
3. Agregar al esquema
4. Colocar arriba del Arduino

### 2.4 Agregar Display 7 Segmentos
1. Presionar `P`
2. Buscar: `7SEG-COM-CATHODE` (cátodo común)
3. Agregar al esquema
4. Colocar a la derecha del Arduino

### 2.5 Agregar LED
1. Presionar `P`
2. Buscar: `LED-RED`
3. Agregar al esquema

### 2.6 Agregar Botones
1. Presionar `P`
2. Buscar: `BUTTON`
3. Agregar **2 botones** al esquema
4. Nombrarlos: `BTN_SEL` y `BTN_LISTO`

### 2.7 Agregar Resistencias
1. Presionar `P`
2. Buscar: `RES`
3. Agregar **8 resistencias**
4. Doble clic en cada una → Valor: `220`

### 2.8 Agregar Terminal Virtual (Monitor Serial)
1. Presionar `P`
2. Buscar: `VIRTUAL TERMINAL`
3. Agregar al esquema

### 2.9 Agregar Tierra y VCC
1. Clic en el icono de terminales (lado izquierdo)
2. Seleccionar `GROUND` y `POWER`
3. Agregar varias instancias donde sea necesario

---

## 🔌 PASO 3: CONEXIONES DEL LCD

```
LCD Pin    →  Arduino Mega Pin
─────────────────────────────────
VSS (1)    →  GND
VDD (2)    →  5V (VCC)
V0  (3)    →  Potenciómetro (wiper) o GND
RS  (4)    →  Pin 8
RW  (5)    →  GND
E   (6)    →  Pin 9
D4  (11)   →  Pin 4
D5  (12)   →  Pin 5
D6  (13)   →  Pin 6
D7  (14)   →  Pin 7
A   (15)   →  5V (con resistencia 220Ω)
K   (16)   →  GND
```

**Pasos en Proteus:**
1. Clic en el pin VSS del LCD
2. Arrastrar cable hasta el símbolo GND
3. Repetir para cada conexión
4. Para V0: conectar a GND para máximo contraste

---

## ⌨️ PASO 4: CONEXIONES DEL TECLADO 4x4

El teclado KEYPAD-SMALLCALC tiene esta distribución:
```
┌─────┬─────┬─────┬─────┐
│  7  │  8  │  9  │  /  │  ← ROW A (Fila 1)
├─────┼─────┼─────┼─────┤
│  4  │  5  │  6  │  *  │  ← ROW B (Fila 2)
├─────┼─────┼─────┼─────┤
│  1  │  2  │  3  │  -  │  ← ROW C (Fila 3)
├─────┼─────┼─────┼─────┤
│  C  │  0  │  =  │  +  │  ← ROW D (Fila 4)
└─────┴─────┴─────┴─────┘
  COL1  COL2  COL3  COL4
```

**Conexiones:**
```
Teclado Pin  →  Arduino Mega Pin
────────────────────────────────
ROW A        →  Pin 10
ROW B        →  Pin 11
ROW C        →  Pin 12
ROW D        →  Pin 13
COL 1        →  Pin 22
COL 2        →  Pin 23
COL 3        →  Pin 24
COL 4        →  Pin 25
```

**Pasos en Proteus:**
1. Hacer clic derecho en el teclado → **Edit Properties**
2. Verificar que tenga los pines nombrados como ROW y COL
3. Conectar cada ROW a los pines 10-13
4. Conectar cada COL a los pines 22-25

---

## 🔢 PASO 5: CONEXIONES DEL DISPLAY 7 SEGMENTOS

```
7-Seg Pin  →  Resistencia (220Ω)  →  Arduino Pin
─────────────────────────────────────────────────
a          →       R1             →  Pin 30
b          →       R2             →  Pin 31
c          →       R3             →  Pin 32
d          →       R4             →  Pin 33
e          →       R5             →  Pin 34
f          →       R6             →  Pin 35
g          →       R7             →  Pin 36
COM        →       (directo)      →  GND
```

**Diagrama del display:**
```
       ┌───a───┐
       │       │
       f       b
       │       │
       ├───g───┤
       │       │
       e       c
       │       │
       └───d───┘
```

**Pasos en Proteus:**
1. Colocar 7 resistencias en línea
2. Conectar un lado de cada resistencia a los pines del Arduino
3. Conectar el otro lado a los segmentos a-g del display
4. Conectar el pin COM (común) a GND

---

## 💡 PASO 6: CONEXIÓN DEL LED ALARMA

```
Arduino Pin 38 ──[220Ω R8]──┤>├── GND
                            LED
```

**Pasos en Proteus:**
1. Colocar la resistencia R8
2. Conectar Pin 38 → Resistencia → Ánodo LED (+)
3. Conectar Cátodo LED (-) → GND

---

## 🔘 PASO 7: CONEXIÓN DE BOTONES COCINA

```
BTN_SELECCIONAR:
Pin 2 ────┤ BUTTON ├──── GND

BTN_LISTO:
Pin 3 ────┤ BUTTON ├──── GND
```

**Pasos en Proteus:**
1. Conectar un terminal del botón al pin del Arduino
2. Conectar el otro terminal a GND
3. Los pull-ups internos se activan por software

> **Nota:** En Proteus los botones tipo `BUTTON` funcionan con clic.

---

## 📡 PASO 8: CONEXIÓN DEL TERMINAL VIRTUAL (SERIAL)

```
Terminal Virtual  →  Arduino Mega
─────────────────────────────────
RXD               →  Pin 1 (TX0)
TXD               →  Pin 0 (RX0)
```

**Configurar Terminal Virtual:**
1. Doble clic en VIRTUAL TERMINAL
2. **Baud Rate:** 9600
3. **Data Bits:** 8
4. **Parity:** None
5. **Stop Bits:** 1
6. **OK**

---

## 🎛️ PASO 9: SIMULACIÓN DEL RFID (Alternativa)

Como el RC522 no existe en Proteus, usar **botones adicionales**:

```
BTN_SIMULAR_TARJETA:
Pin 18 ────┤ BUTTON ├──── GND
(Simula detección de tarjeta)

BTN_PAGO_OK:
Pin 19 ────┤ BUTTON ├──── GND
(Simula pago exitoso)
```

> **Modificación en código:** Puedes agregar lógica para que al presionar estos botones se simule la lectura de tarjeta.

---

## 📁 PASO 10: CARGAR EL PROGRAMA (.HEX)

### 10.1 Compilar en Arduino IDE
1. Abrir `Proyecto_cocina.ino` en Arduino IDE
2. **Herramientas → Placa → Arduino Mega 2560**
3. **Programa → Exportar binario compilado** (Ctrl+Alt+S)
4. Se genera archivo `.hex` en la misma carpeta

### 10.2 Cargar en Proteus
1. Doble clic en el Arduino Mega en Proteus
2. En **Program File:** clic en el ícono de carpeta
3. Navegar hasta el archivo `.hex` generado
4. Seleccionar y **OK**

---

## ▶️ PASO 11: EJECUTAR SIMULACIÓN

1. Clic en el botón **Play** (esquina inferior izquierda)
2. El LCD debería mostrar "RESTAURANTE v2.0"
3. Después mostrará el menú principal

### Pruebas a realizar:
| Acción | Tecla | Resultado esperado |
|--------|-------|-------------------|
| Nuevo pedido | `/` | Muestra categorías |
| Modo recarga | `*` | "MODO RECARGA" |
| Siguiente categoría | `=` | Avanza categoría |
| Ver total | `C` | Muestra total |
| Agregar cliente | `+` | "Cliente X agregado" |
| Cancelar | `C` | Vuelve al menú |

---

## 📐 DIAGRAMA COMPLETO DE CONEXIONES

```
                         ┌─────────────────────────────────────┐
                         │         ARDUINO MEGA 2560          │
                         │                                     │
    ┌─────────┐          │  0 (RX) ◄──────── TXD (Terminal)   │
    │  LCD    │          │  1 (TX) ────────► RXD (Terminal)   │
    │ 16x2    │          │  2 ◄──────────── BTN_SELECCIONAR   │
    │         │          │  3 ◄──────────── BTN_LISTO         │
    │ D7 ─────┼──────────┤  7                                  │
    │ D6 ─────┼──────────┤  6                                  │
    │ D5 ─────┼──────────┤  5                                  │
    │ D4 ─────┼──────────┤  4                                  │
    │ E  ─────┼──────────┤  9                                  │
    │ RS ─────┼──────────┤  8                                  │
    └─────────┘          │                                     │
                         │ 10 ◄──────────── ROW A (Teclado)   │
    ┌─────────┐          │ 11 ◄──────────── ROW B             │
    │ TECLADO │          │ 12 ◄──────────── ROW C             │
    │  4x4    │          │ 13 ◄──────────── ROW D             │
    │         │          │                                     │
    │ C1 ─────┼──────────┤ 22                                  │
    │ C2 ─────┼──────────┤ 23                                  │
    │ C3 ─────┼──────────┤ 24                                  │
    │ C4 ─────┼──────────┤ 25                                  │
    └─────────┘          │                                     │
                         │ 30 ──[220Ω]──► a ┐                 │
    ┌─────────┐          │ 31 ──[220Ω]──► b │                 │
    │ 7-SEG   │          │ 32 ──[220Ω]──► c │ Display         │
    │         │◄─────────┤ 33 ──[220Ω]──► d │ 7-Seg           │
    │   COM ──┼── GND    │ 34 ──[220Ω]──► e │                 │
    └─────────┘          │ 35 ──[220Ω]──► f │                 │
                         │ 36 ──[220Ω]──► g ┘                 │
                         │                                     │
                         │ 38 ──[220Ω]──► LED ──► GND         │
                         │                                     │
                         │ 5V  ────────► VCC (LCD, etc.)      │
                         │ GND ────────► GND (común)          │
                         └─────────────────────────────────────┘
```

---

## ⚠️ SOLUCIÓN DE PROBLEMAS COMUNES

### El LCD no muestra nada
- Verificar conexiones V0 (contraste)
- Asegurarse que VDD está en 5V
- Verificar que el archivo .hex está cargado

### El teclado no responde
- Verificar que las filas van a pines 10-13
- Verificar que las columnas van a pines 22-25
- Revisar la orientación del componente

### El display 7-seg no enciende
- Verificar que es cátodo común conectado a GND
- Revisar resistencias en serie
- Verificar polaridad

### Error al cargar .hex
- Usar **Exportar binario compilado** en Arduino IDE
- Seleccionar el archivo con extensión `.ino.mega.hex`

---

## 🎮 FUNCIONES DEL TECLADO

```
┌─────────────────────────────────────────────────────────┐
│                    MENÚ PRINCIPAL                       │
├─────────────────────────────────────────────────────────┤
│  /  = Nuevo Pedido (iniciar toma de pedido)            │
│  *  = Modo Recarga (recargar tarjeta RFID)             │
│  -  = Pagar (procesar pago con tarjeta)                │
│  +  = Agregar Cliente (max 5 por mesa)                 │
│  0  = Cambiar Cliente (rotar entre clientes)           │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                 SELECCIÓN DE ITEMS                      │
├─────────────────────────────────────────────────────────┤
│ 1-3 = Seleccionar opción del menú                      │
│  =  = Siguiente categoría / Enviar pedido              │
│  C  = Ver total / Confirmar envío                      │
│  /  = Volver al menú principal                         │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                   INGRESO DE MONTO                      │
├─────────────────────────────────────────────────────────┤
│ 0-9 = Ingresar dígitos del monto                       │
│  =  = Confirmar recarga                                │
│  C  = Cancelar                                         │
│  -  = Borrar último dígito                             │
└─────────────────────────────────────────────────────────┘

---

## ✅ CHECKLIST FINAL

````
