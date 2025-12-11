/******************************************************************************************
   PROYECTO FINAL - SISTEMA DE RESTAURANTE CON RFID
   Arduino Mega 2560
   
   FUNCIONALIDADES:
   - Menú interactivo en LCD 16x2
   - Teclado matricial 4x4 con interrupciones
   - RFID RC522 para pago y recarga
   - Comunicación serial a cocina
   - Display 7 segmentos para número de pedido
   - LED alarma para cliente
   - Cola FIFO de comandas
   - Soporte multi-cliente por mesa
   
   Autor: Proyecto Microprocesadores
   Fecha: Diciembre 2025
*******************************************************************************************/

#include <SPI.h>
#include <MFRC522.h>
#include <avr/interrupt.h>

// Debug toggle: poner 1 para habilitar trazas DEBUG, 0 para desactivar
#define DEBUG 1

#if DEBUG
#define DBG_PRINT(x) Serial.print(x)
#define DBG_PRINTLN(x) Serial.println(x)
#else
#define DBG_PRINT(x)
#define DBG_PRINTLN(x)
#endif

// ==================== CONFIGURACIÓN DE PINES ====================

// --- LCD 16x2 (modo 4 bits) ---
// RW conectado a GND (solo escritura)
#define LCD_RS  24
#define LCD_EN  22
#define LCD_D4  34
#define LCD_D5  35
#define LCD_D6  36
#define LCD_D7  37

// --- RFID RC522 ---
#define RFID_SS   53    // SDA/SS
#define RFID_RST  5     // RST (igual que archivos funcionales)
// MOSI = 51, MISO = 50, SCK = 52 (SPI por defecto)

// --- RFID IRQ ---
#define RFID_IRQ_PIN 18  // INT5 (Pin 18 en Mega) - Conectar pin IRQ del RC522 aquí

// --- Teclado 4x4 ---
byte rowPins[4] = {10, 11, 12, 13};   // Filas (PCINT4-7)
byte colPins[4] = {26, 27, 28, 29};   // Columnas (movidas, evitan conflicto con LCD)

// --- Botones Cocina (interrupciones externas) ---
#define BTN_SELECCIONAR  2    // INT0 - Avanzar/Seleccionar pedido (Av)
#define BTN_LISTO        3    // INT1 - Marcar pedido listo (Con)

// --- Display 7 Segmentos (MOVIDO A PINES ANALÓGICOS A8-A14) ---
// Usamos los pines analógicos como digitales para liberar TX2/RX2 e Interrupciones
#define SEG_A   A8
#define SEG_B   A9
#define SEG_C   A10
#define SEG_D   A11
#define SEG_E   A12
#define SEG_F   A13
#define SEG_G   A14

const int PINES_7SEG[7] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G};

// --- LED Alarma Cliente ---
#define LED_ALARMA  38

// --- Buzzer (opcional) ---
#define BUZZER_PIN  40

// ==================== CONFIGURACIÓN TECLADO ====================
// Distribución del teclado físico 4x4 - AJUSTADA
// Fila 1: 1,2,3,A
// Fila 2: 4,5,6,B
// Fila 3: 7,8,9,C
// Fila 4: *,0,#,D
// Matriz igual que calculadora funcional
char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Variables y flags para manejo por interrupción (no hacer scanning en ISR)
volatile char ultimaTecla = 0;        // última tecla detectada (procesada en loop)
volatile bool teclaPendiente = false; // flag indicado por la ISR
volatile uint8_t filaInterrup = 0xFF; // fila que causó la interrupción (0..3)
volatile uint8_t lastRowState = 0;    // bitmap de estado anterior de filas

// Variable volátil para la interrupción
volatile bool tarjetaDetectadaIRQ = false;

// FUNCIONES DE TECLAS:
// A = Nuevo pedido (ordenar)
// B = Modo recarga
// C = Cancelar / Enviar pedido
// D = Pagar
// # = Confirmar (OK) / Agregar cliente
// * = Cambiar cliente
// 0 = Ver opciones
// 1,2,3 = Seleccionar opción en menú

// ==================== MENÚ DEL RESTAURANTE ====================
const char* categorias[4] = {"== ENTRADA ==", "=PTO FUERTE=", "== BEBIDA ==", "== POSTRE =="};
const char* nombresItems[4][3] = {
  {"Sopa", "Ensalada", "Nada"},
  {"Carne", "Pollo", "Pescado"},
  {"Agua", "Refresco", "Cerveza"},
  {"Helado", "Flan", "Nada"}
};
// Precios mostrados en forma corta
const char* preciosStr[4][3] = {
  {"$4000", "$5000", "$0"},
  {"$15000", "$13000", "$18000"},
  {"$1500", "$2500", "$4000"},
  {"$4000", "$3500", "$0"}
};
uint16_t precios[4][4] = {
  {4000, 5000, 0, 0},
  {15000, 13000, 18000, 0},
  {1500, 2500, 4000, 0},
  {4000, 3500, 0, 0}
};

// ==================== PATRONES 7 SEGMENTOS ====================
// Orden: a,b,c,d,e,f,g para dígitos 0-9
const byte patronesDigitos[10] = {
  0b0111111,  // 0: a,b,c,d,e,f
  0b0000110,  // 1: b,c
  0b1011011,  // 2: a,b,d,e,g
  0b1001111,  // 3: a,b,c,d,g
  0b1100110,  // 4: b,c,f,g
  0b1101101,  // 5: a,c,d,f,g
  0b1111101,  // 6: a,c,d,e,f,g
  0b0000111,  // 7: a,b,c
  0b1111111,  // 8: todos
  0b1101111   // 9: a,b,c,d,f,g
};

// ==================== ESTRUCTURAS DE DATOS ====================

// Cola de comandas para cocina
#define MAX_COMANDAS 10
struct Comanda {
  uint16_t numero;
  uint32_t total;
  bool entregada;
};
Comanda colaCocina[MAX_COMANDAS];
uint8_t colaInicio = 0, colaFin = 0, colaTam = 0;

// Lista de pedidos (solo se borran al pagar)
#define MAX_PEDIDOS 10
struct Pedido {
  uint16_t numero;
  uint32_t total;
  bool pagado;
  uint8_t cliente; // índice del cliente que generó el pedido
};
Pedido listaPedidos[MAX_PEDIDOS];
uint8_t numPedidosActivos = 0;

// Soporte multi-cliente por mesa
#define MAX_CLIENTES 5
struct Cliente {
  uint32_t total;
  String items;
};
Cliente clientesMesa[MAX_CLIENTES];
uint8_t clienteActual = 0;
uint8_t totalClientes = 1;

// ==================== VARIABLES GLOBALES ====================
MFRC522 mfrc522(RFID_SS, RFID_RST);
MFRC522::MIFARE_Key keyRFID;

// Contadores y estados
uint16_t numPedidoGlobal = 1;
uint8_t catActual = 0;
int8_t pedidoSeleccionado = -1;

// Debounce / bloqueo para boton LISTO
unsigned long lastListoTime = 0;

// Estados del sistema
enum EstadoSistema { 
  MENU_PRINCIPAL, 
  SELECCION_CATEGORIA, 
  MOSTRAR_TOTAL,
  ESPERANDO_PAGO,
  MODO_RECARGA,
  INGRESANDO_MONTO
};
EstadoSistema estadoActual = MENU_PRINCIPAL;

// Banderas de interrupción
volatile bool teclaPresionada = false;
volatile bool btnSeleccionarPresionado = false;
volatile bool btnListoPresionado = false;
volatile bool timerTick = false;

// Mostrar resumen de categorías solo la primera vez en el menú
bool firstMenuShown = false;

// Variables para entrada de monto
uint32_t montoIngresado = 0;
uint8_t digitosIngresados = 0;

// Saldo RFID
uint32_t saldoTarjeta = 0;

// ==================== PROTOTIPOS DE FUNCIONES ====================
// Agrega estos prototipos aquí para declarar todas las funciones antes de usarlas
void pulseEnable();
void lcd_write4bits(uint8_t value);
void lcd_command(uint8_t cmd);
void lcd_write(uint8_t c);
void lcd_init();
void lcd_clear();
void lcd_setCursor(uint8_t col, uint8_t row);
void lcd_print(const char* s);
void lcd_printNum(uint32_t num);
void init7Seg();
void mostrar7Seg(uint8_t digito);
void apagar7Seg();
void initRFID();
bool leerSaldoRFID();
bool escribirSaldoRFID(uint32_t nuevoSaldo);
bool inicializarTarjetaRFID();
bool agregarComanda(uint16_t num, uint32_t total);
Comanda obtenerComandaActual();
bool removerComandaSeleccionada();
bool removerComanda();
void initTeclado();
char leerTecla();
void mostrarMenuPrincipal();
void mostrarCategoria();
void mostrarOpcionesCategoria();
void scrollMostrarCategoria(uint8_t cat);
void mostrarCategoriasResumen();
uint16_t pedirCantidad();
void mostrarRollMenu();
void mostrarTotalCliente();
void mostrarTotalMesa();
void mostrarModoRecarga();
void mostrarIngresoMonto();
void procesarTecla(char tecla);
void procesarMenuPrincipal(char tecla);
void procesarSeleccionCategoria(char tecla);
void procesarIngresoMonto(char tecla);
void procesarRFID();
void realizarPago();
void enviarPedidoCocina();
void eliminarClienteAt(uint8_t idx);
void procesarBotonSeleccionar();
void procesarBotonListo();
void actualizarDisplay();

// Rutina de Interrupción para RFID
void isrRFID() {
  tarjetaDetectadaIRQ = true;
}

// ==================== FUNCIONES LCD (Manual 4-bits) ====================
void pulseEnable() {
  digitalWrite(LCD_EN, HIGH);
  delayMicroseconds(1);
  digitalWrite(LCD_EN, LOW);
  delayMicroseconds(100);
}

void lcd_write4bits(uint8_t value) {
  digitalWrite(LCD_D4, (value >> 0) & 1);
  digitalWrite(LCD_D5, (value >> 1) & 1);
  digitalWrite(LCD_D6, (value >> 2) & 1);
  digitalWrite(LCD_D7, (value >> 3) & 1);
  pulseEnable();
}

void lcd_command(uint8_t cmd) {
  digitalWrite(LCD_RS, LOW);
  lcd_write4bits(cmd >> 4);
  lcd_write4bits(cmd & 0x0F);
  if (cmd <= 3) delay(2);
}

void lcd_write(uint8_t c) {
  digitalWrite(LCD_RS, HIGH);
  lcd_write4bits(c >> 4);
  lcd_write4bits(c & 0x0F);
}

void lcd_init() {
  pinMode(LCD_RS, OUTPUT);
  pinMode(LCD_EN, OUTPUT);
  pinMode(LCD_D4, OUTPUT);
  pinMode(LCD_D5, OUTPUT);
  pinMode(LCD_D6, OUTPUT);
  pinMode(LCD_D7, OUTPUT);
  
  // Esperar que LCD se estabilice (mínimo 40ms después de encender)
  delay(100);
  
  // Inicialización especial para modo 4-bit
  digitalWrite(LCD_RS, LOW);
  digitalWrite(LCD_EN, LOW);
  
  // Secuencia de inicialización (3 veces 0x03, luego 0x02)
  lcd_write4bits(0x03); 
  delay(5);  // Esperar >4.1ms
  lcd_write4bits(0x03); 
  delay(5);  // Esperar >100us
  lcd_write4bits(0x03);
  delay(1);
  lcd_write4bits(0x02);  // Modo 4-bit
  delay(1);
  
  lcd_command(0x28);     // 2 líneas, 5x8 font
  delay(1);
  lcd_command(0x0C);     // Display ON, cursor OFF, blink OFF
  delay(1);
  lcd_command(0x06);     // Incrementar cursor, no shift
  delay(1);
  lcd_command(0x01);     // Clear display
  delay(3);              // Clear necesita más tiempo
}

void lcd_clear() { 
  lcd_command(0x01); 
  delay(2); 
}

void lcd_setCursor(uint8_t col, uint8_t row) {
  lcd_command(row == 0 ? 0x80 + col : 0xC0 + col);
}

void lcd_print(const char* s) { 
  while (*s) lcd_write(*s++); 
}

void lcd_printNum(uint32_t num) {
  char buf[12];
  ltoa(num, buf, 10);
  lcd_print(buf);
}

// ==================== FUNCIONES 7 SEGMENTOS ====================
void init7Seg() {
  for (int i = 0; i < 7; i++) {
    pinMode(PINES_7SEG[i], OUTPUT);
    digitalWrite(PINES_7SEG[i], LOW); // Apagar segmento (cátodo común)
  }
}

void mostrar7Seg(uint8_t digito) {
  if (digito > 9) digito = 0;
  for (int i = 0; i < 7; i++) {
    // Para cátodo común: segmento encendido = HIGH, apagado = LOW
    digitalWrite(PINES_7SEG[i], ((patronesDigitos[digito] >> i) & 1) ? HIGH : LOW);
  }
}

void apagar7Seg() {
  for (int i = 0; i < 7; i++) {
    digitalWrite(PINES_7SEG[i], LOW); // Apagar todos los segmentos (cátodo común)
  }
}

// ==================== FUNCIONES RFID ====================
void initRFID() {
  SPI.begin();
  mfrc522.PCD_Init();
  
  // --- CONFIGURACIÓN PARA ACTIVAR IRQ EN RC522 ---
  // 0xA0 = Habilitar IRQ en detección
  mfrc522.PCD_WriteRegister(MFRC522::ComIEnReg, 0xA0); 
  mfrc522.PCD_SetRegisterBitMask(MFRC522::DivIEnReg, 0x80); 
  
  // Configurar pin del Mega
  pinMode(RFID_IRQ_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RFID_IRQ_PIN), isrRFID, FALLING);

  // Clave por defecto
  for (byte i = 0; i < 6; i++) {
    keyRFID.keyByte[i] = 0xFF;
  }
  Serial.println("[INFO] RFID por Interrupcion listo.");
}

bool leerSaldoRFID() {
  // Igual que archivo "leer nombre tarjeta.txt"
  if (!mfrc522.PICC_IsNewCardPresent()) {
    Serial.println("[INFO] No se detectó tarjeta. Intente de nuevo.");
    return false;
  }
  Serial.println("[INFO] Tarjeta detectada, intentando leer UID...");
  if (!mfrc522.PICC_ReadCardSerial()) {
    Serial.println("[ERROR] No se pudo leer el UID de la tarjeta. Puede estar dañada o incompatible.");
    return false;
  }
  Serial.print("[INFO] UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();
  
  byte block = 4;
  
  // Autenticar bloque 4 (sector 1)
  MFRC522::StatusCode status;
  Serial.println("[INFO] Autenticando bloque 4 con clave por defecto...");
  status = mfrc522.PCD_Authenticate(
             MFRC522::PICC_CMD_MF_AUTH_KEY_A,
             block,
             &keyRFID,
             &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.print("[ERROR] Error de autenticación en bloque 4: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    Serial.println("[SUGERENCIA] Verifique que la tarjeta esté formateada y use la clave FF FF FF FF FF FF.");
    mfrc522.PICC_HaltA();
    return false;
  }
  
  // Leer bloque
  byte buffer[18];
  byte size = sizeof(buffer);
  
  Serial.println("[INFO] Leyendo bloque 4...");
  status = mfrc522.MIFARE_Read(block, buffer, &size);

  if (status != MFRC522::STATUS_OK) {
    Serial.print("[ERROR] Error leyendo saldo en bloque 4: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    Serial.println("[SUGERENCIA] Puede que el bloque no esté escrito o la tarjeta sea incompatible.");
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return false;
  }
  
  // Extraer saldo (4 bytes little-endian) - Corregido para evitar warnings
  saldoTarjeta = (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) | ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
  
  Serial.print("[OK] Saldo leído correctamente: $");
  Serial.println(saldoTarjeta);
  
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  return true;
}

bool escribirSaldoRFID(uint32_t nuevoSaldo) {
  // Igual que archivo "guardar nombre en tarjeta.txt"
  // Permitir recargar si la tarjeta ya está presente
  // Si el UID ya está cargado en la instancia (por una lectura previa), úsalo.
  if (mfrc522.uid.size == 0) {
    // Intentar leer el UID directamente
    if (!mfrc522.PICC_ReadCardSerial()) {
      // Si no hay UID, intentar detectar nueva tarjeta
      if (!mfrc522.PICC_IsNewCardPresent()) {
        Serial.println("[INFO] No se detectó tarjeta para recarga.");
        return false;
      }
      if (!mfrc522.PICC_ReadCardSerial()) {
        Serial.println("[ERROR] No se pudo leer el UID de la tarjeta para recarga.");
        return false;
      }
    }
  }
  
  byte block = 4;
  
  // Autenticar bloque 4 (sector 1)
  MFRC522::StatusCode status;
  status = mfrc522.PCD_Authenticate(
             MFRC522::PICC_CMD_MF_AUTH_KEY_A,
             block,
             &keyRFID,
             &(mfrc522.uid));
  
  if (status != MFRC522::STATUS_OK) {
    Serial.print("[ERROR] Error de autenticación al recargar: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    return false;
  }
  
  // Preparar datos (16 bytes requeridos)
  byte buffer[16] = {0};
  buffer[0] = nuevoSaldo & 0xFF;
  buffer[1] = (nuevoSaldo >> 8) & 0xFF;
  buffer[2] = (nuevoSaldo >> 16) & 0xFF;
  buffer[3] = (nuevoSaldo >> 24) & 0xFF;
  
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  
  // Mensajes de debug eliminados
  
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  if (status == MFRC522::STATUS_OK) {
    Serial.print("[OK] Recarga exitosa. Nuevo saldo: $");
    Serial.println(nuevoSaldo);
    // Dejar la tarjeta en halt tras la escritura
    return true;
  } else {
    Serial.println("[ERROR] No se pudo escribir el nuevo saldo en la tarjeta.");
    return false;
  }
}

// ==================== FUNCIONES COLA FIFO ====================
// ==================== INICIALIZAR / FORMATEAR TARJETA RFID ====================
bool inicializarTarjetaRFID() {
  Serial.println("[INFO] Iniciando formateo de tarjeta RFID (bloque 4)...");
  if (!mfrc522.PICC_IsNewCardPresent()) {
    Serial.println("[ERROR] No se detectó tarjeta para inicializar. Retire y acerque la tarjeta.");
    return false;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    Serial.println("[ERROR] No se pudo leer el UID de la tarjeta para inicializar.");
    return false;
  }
  byte block = 4;
  MFRC522::StatusCode status;
  status = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A,
    block,
    &keyRFID,
    &(mfrc522.uid)
  );
  if (status != MFRC522::STATUS_OK) {
    Serial.print("[ERROR] Error de autenticación al inicializar: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    return false;
  }
  byte buffer[16] = {0}; // Formatear con ceros
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  if (status == MFRC522::STATUS_OK) {
    Serial.println("[OK] Tarjeta inicializada correctamente. Bloque 4 ahora contiene ceros.");
    return true;
  } else {
    Serial.println("[ERROR] No se pudo escribir en la tarjeta durante la inicialización.");
    return false;
  }
}

bool agregarComanda(uint16_t num, uint32_t total) {
  if (colaTam >= MAX_COMANDAS) {
    lcd_clear();
    lcd_print("ERROR: Cola llena");
    delay(1500);
    return false;
  }
  
  colaCocina[colaFin].numero = num;
  colaCocina[colaFin].total = total;
  colaCocina[colaFin].entregada = false;
  colaFin = (colaFin + 1) % MAX_COMANDAS;
  colaTam++;
  
  // Mostrar en 7-seg el último pedido
  mostrar7Seg(num % 10);
  
  return true;
}

Comanda obtenerComandaActual() {
  if (colaTam == 0) {
    return {0, 0, true};
  }
  int idx = (colaInicio + (pedidoSeleccionado >= 0 ? pedidoSeleccionado : 0)) % MAX_COMANDAS;
  return colaCocina[idx];
}

// Remueve el pedido seleccionado actualmente (no siempre el primero)
bool removerComandaSeleccionada() {
  if (colaTam == 0) return false;
  int start = (pedidoSeleccionado >= 0) ? pedidoSeleccionado : 0;
  // Eliminada variable unused idxRemover
  
  // Mover todos los elementos después del removido hacia atrás
  for (int i = start; i < colaTam - 1; i++) {
    int idxActual = (colaInicio + i) % MAX_COMANDAS;
    int idxSiguiente = (colaInicio + i + 1) % MAX_COMANDAS;
    colaCocina[idxActual] = colaCocina[idxSiguiente];
  }
  
  // Actualizar cola
  colaFin = (colaFin - 1 + MAX_COMANDAS) % MAX_COMANDAS;
  colaTam--;
  
  // Ajustar selección
  if (colaTam == 0) {
    pedidoSeleccionado = -1;
    apagar7Seg();
  } else {
    // Ajustar pedidoSeleccionado si es mayor o igual al nuevo tamaño
    if (pedidoSeleccionado < 0) pedidoSeleccionado = 0;
    if (pedidoSeleccionado >= colaTam) pedidoSeleccionado = pedidoSeleccionado % colaTam;
    mostrar7Seg(obtenerComandaActual().numero % 10);
  }
  
  return true;
}

// Remueve siempre el primer pedido de la cola (FIFO)
bool removerComanda() {
  if (colaTam == 0) return false;
  
  colaInicio = (colaInicio + 1) % MAX_COMANDAS;
  colaTam--;
  pedidoSeleccionado = -1;
  
  if (colaTam > 0) {
    mostrar7Seg(colaCocina[colaInicio].numero % 10);
  } else {
    apagar7Seg();
  }
  
  return true;
}

// ==================== FUNCIONES TECLADO CORREGIDAS ====================

void initTeclado() {
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], INPUT_PULLUP);
    pinMode(colPins[i], OUTPUT);        
    digitalWrite(colPins[i], LOW); // Columnas en LOW para detectar interrupción
  }

  // Configurar interrupción PCINT0 (Pines 10-13)
  PCICR |= (1 << PCIE0); 
  PCMSK0 |= (1 << PCINT4) | (1 << PCINT5) | (1 << PCINT6) | (1 << PCINT7);
  PCIFR |= (1 << PCIF0); // Limpiar bandera inicial
}

// ISR ÚNICA para PCINT0
ISR(PCINT0_vect) {
  // Verificar si alguna fila está en LOW (tecla presionada)
  bool actividad = false;
  for(int i=0; i<4; i++) {
    if(digitalRead(rowPins[i]) == LOW) actividad = true;
  }
  
  if(actividad) {
    // CORRECCIÓN: Usar la variable que el loop() está escuchando
    teclaPresionada = true; 
  }
}

char leerTecla() {
  // Deshabilitar interrupciones para escanear sin interferencias
  PCICR &= ~(1 << PCIE0); 

  char teclaEncontrada = 0;
  
  // 1. Poner todas las columnas en HIGH (desactivadas)
  for (int i = 0; i < 4; i++) digitalWrite(colPins[i], HIGH);

  // 2. Escanear una por una
  for (byte c = 0; c < 4; c++) {
    digitalWrite(colPins[c], LOW); // Activar columna
    delayMicroseconds(10);         // Estabilizar
    
    for (byte r = 0; r < 4; r++) {
      if (digitalRead(rowPins[r]) == LOW) {
        teclaEncontrada = keys[r][c];
        // Esperar a que suelte (anti-rebote simple)
        unsigned long t0 = millis();
        while(digitalRead(rowPins[r]) == LOW && (millis() - t0 < 200));
      }
    }
    digitalWrite(colPins[c], HIGH); // Desactivar columna
    if (teclaEncontrada) break;
  }

  // 3. RESTAURAR: Todas las columnas a LOW para esperar la siguiente interrupción
  for (int i = 0; i < 4; i++) digitalWrite(colPins[i], LOW);
  
  // Limpiar bandera de interrupción que pudo saltar durante el escaneo y reactivar
  PCIFR |= (1 << PCIF0); 
  PCICR |= (1 << PCIE0); 
  
  return teclaEncontrada;
}

// ==================== FUNCIONES MENÚ ====================
void mostrarMenuPrincipal() {
  if (!firstMenuShown) {
    mostrarCategoriasResumen();
    firstMenuShown = true;
  }
  lcd_clear();
  lcd_setCursor(0, 0);
  // Línea 1: "Cli1 $15000 [2]"
  lcd_print("Cli");
  lcd_printNum(clienteActual + 1);
  lcd_print(" $");
  lcd_printNum(clientesMesa[clienteActual].total);
  // Mostrar total clientes a la derecha
  lcd_setCursor(12, 0);
  lcd_print("[");
  lcd_printNum(totalClientes);
  lcd_print("]");
  
  // Línea 2: Opciones con nuevas teclas
  lcd_setCursor(0, 1);
  lcd_print("A:Ord B:Rec D:Pag");
}

void mostrarCategoria() {
  // Al entrar a una categoría, mostrar un scroll con los productos disponibles
  scrollMostrarCategoria(catActual);

  lcd_clear();
  lcd_setCursor(0, 0);
  lcd_print(categorias[catActual]);
  lcd_setCursor(0, 1);
  lcd_print("1-3:Elige C:Enviar");
}

// Mostrar opciones de la categoría actual
void mostrarOpcionesCategoria() {
  // Mostrar opción 1 y 2 primero
  lcd_clear();
  lcd_setCursor(0, 0);
  lcd_print("1.");
  lcd_print(nombresItems[catActual][0]);
  lcd_print(" ");
  lcd_print(preciosStr[catActual][0]);
  lcd_setCursor(0, 1);
  lcd_print("2.");
  lcd_print(nombresItems[catActual][1]);
  lcd_print(" ");
  lcd_print(preciosStr[catActual][1]);
  delay(600);
  
  // Mostrar opción 3
  lcd_clear();
  lcd_setCursor(0, 0);
  lcd_print("3.");
  lcd_print(nombresItems[catActual][2]);
  lcd_print(" ");
  lcd_print(preciosStr[catActual][2]);
  lcd_setCursor(0, 1);
  lcd_print("#:Sig C:Enviar");
  delay(600);
  
  mostrarCategoria();
}

// Muestra los productos de una categoría con efecto "scroll/marquee".
// Se puede salir presionando cualquier tecla.
void scrollMostrarCategoria(uint8_t cat) {
  // Construir strings para cada item: "Nombre - $Precio"
  for (int i = 0; i < 3; i++) {
    String line = String(nombresItems[cat][i]);
    line += " - ";
    line += preciosStr[cat][i];

    // Si la línea cabe en 16 cols, mostrarla y esperar breve tiempo
    if (line.length() <= 16) {
      lcd_clear();
      lcd_setCursor(0,0);
      lcd_print(line.c_str());
      // esperar 1.5s o salir si tecla
      unsigned long start = millis();
      while (millis() - start < 600) {
        if (leerTecla()) return;
        delay(50);
      }
      continue;
    }

    // Marquee simple: desplazar la ventana de 16 caracteres
    int total = line.length();
    int window = 16;
    // Repetir el scroll dos veces
    for (int pass = 0; pass < 2; pass++) {
        for (int pos = 0; pos <= total - window; pos++) {
        lcd_clear();
        lcd_setCursor(0,0);
        String sub = line.substring(pos, pos + window);
        lcd_print(sub.c_str());
        unsigned long t0 = millis();
        while (millis() - t0 < 120) {
          if (leerTecla()) return;
          delay(10);
        }
      }
      // Pausa al final del scroll
      unsigned long t1 = millis();
      while (millis() - t1 < 200) {
        if (leerTecla()) return;
        delay(10);
      }
    }
  }
}

// Muestra un resumen con los nombres de las 4 categorías en pantalla
void mostrarCategoriasResumen() {
  lcd_clear();
  lcd_setCursor(0,0);
  // Línea 1: "1 Entrada 2 Fte"
  lcd_print("1 Entrad 2 Fte");
  lcd_setCursor(0,1);
  // Línea 2: "3 Bebida 4 Postre"
  lcd_print("3 Bebid 4 Postre");
  delay(600);
  // Mostrar variantes más legibles si hay tiempo
  lcd_clear();
  lcd_setCursor(0,0);
  lcd_print("Entrada - Plato");
  lcd_setCursor(0,1);
  lcd_print("Fuerte - Beb - Post");
  delay(600);
}

// Pedir cantidad al usuario (teclado) -- devuelve al menos 1
uint16_t pedirCantidad() {
  uint16_t cantidad = 0;
  uint8_t dig = 0;
  lcd_clear();
  lcd_setCursor(0,0);
  lcd_print("Cantidad:");
  lcd_setCursor(0,1);
  lcd_printNum(cantidad);
  lcd_print(" #OK *Borr C=Salir");

  unsigned long start = millis();
  while (true) {
    char t = leerTecla();
    if (t >= '0' && t <= '9' && dig < 3) { // hasta 3 dígitos
      cantidad = cantidad * 10 + (t - '0');
      dig++;
      lcd_setCursor(0,1);
      lcd_print("      ");
      lcd_setCursor(0,1);
      lcd_printNum(cantidad);
      lcd_print(" #OK *Borr C=Salir");
    } else if (t == '*') {
      if (dig > 0) {
        cantidad /= 10;
        dig--;
        lcd_setCursor(0,1);
        lcd_print("      ");
        lcd_setCursor(0,1);
        lcd_printNum(cantidad);
        lcd_print(" #OK *Borr C=Salir");
      }
    } else if (t == '#') {
      if (cantidad == 0) cantidad = 1;
      return cantidad;
    } else if (t == 'C') {
      return 1; // cancelar selección cantidad -> 1 por defecto
    }
    // Timeout opcional: si pasan 30s, devolver 1
    if (millis() - start > 30000) return (cantidad == 0) ? 1 : cantidad;
    delay(50);
  }
}

// Mostrar menú en modo "roll" para que el cliente vea opciones
void mostrarRollMenu() {
  unsigned long endTime = millis() + 12000; // 12 segundos de roll
  while (millis() < endTime) {
    for (int c = 0; c < 4; c++) {
      lcd_clear();
      lcd_setCursor(0,0);
      lcd_print(categorias[c]);
      lcd_setCursor(0,1);
      delay(1500);
      lcd_print(nombresItems[c][0]);
      lcd_print(" ");
      lcd_print(preciosStr[c][0]);
      delay(2500);

      lcd_clear();
      lcd_setCursor(0,0);
      lcd_print(nombresItems[c][1]);
      lcd_print(" ");
      lcd_print(preciosStr[c][1]);
      lcd_setCursor(0,1);
      lcd_print(nombresItems[c][2]);
      lcd_print(" ");
      lcd_print(preciosStr[c][2]);
      unsigned long innerStart = millis();
      while (millis() - innerStart < 2500) {
        if (leerTecla()) return; // salir si el cliente presiona algo
        delay(50);
      }
      if (leerTecla()) return;
    }
    if (leerTecla()) return;
  }
  mostrarMenuPrincipal();
}

void mostrarTotalCliente() {
  lcd_clear();
  lcd_setCursor(0, 0);
  lcd_print(">Cliente ");
  lcd_printNum(clienteActual + 1);
  lcd_print(" de ");
  lcd_printNum(totalClientes);
  lcd_setCursor(0, 1);
  lcd_print("Cuenta: $");
  lcd_printNum(clientesMesa[clienteActual].total);
}

void mostrarTotalMesa() {
  // Calcular total pendiente de pago (solo clientes que aún deben)
  uint32_t totalPendiente = 0;
  uint8_t clientesPendientes = 0;
  for (int i = 0; i < totalClientes; i++) {
    if (clientesMesa[i].total > 0) {
      totalPendiente += clientesMesa[i].total;
      clientesPendientes++;
    }
  }
  
  lcd_clear();
  lcd_setCursor(0, 0);
  lcd_print("CUENTA MESA:");
  lcd_setCursor(0, 1);
  lcd_print("$");
  lcd_printNum(totalPendiente);
  lcd_print(" (");
  lcd_printNum(clientesPendientes);
  lcd_print(" pend)");
}

void mostrarModoRecarga() {
  lcd_clear();
  lcd_setCursor(0, 0);
  lcd_print("=== RECARGA ===");
  lcd_setCursor(0, 1);
  lcd_print("Pase su tarjeta");
}

void mostrarIngresoMonto() {
  lcd_clear();
  lcd_setCursor(0, 0);
  lcd_print("Recargar: $");
  lcd_printNum(montoIngresado);
  lcd_setCursor(0, 1);
  lcd_print("#OK *Borr C=Salir");
}

// Nueva función para mostrar el título de la categoría por 500ms
void mostrarTituloCategoria(uint8_t cat) {
  lcd_clear();
  lcd_setCursor(0, 0);
  
  // Centramos el texto según la categoría
  switch(cat) {
    case 0: lcd_print("=== ENTRADAS ==="); break;
    case 1: lcd_print("= PLATO FUERTE ="); break;
    case 2: lcd_print("=== BEBIDAS ===="); break;
    case 3: lcd_print("=== POSTRES ===="); break;
  }
  
  // Línea decorativa abajo (opcional)
  lcd_setCursor(0, 1);
  lcd_print("================");
  
  delay(500); // El tiempo que pediste
}

// ==================== ISRs (Interrupciones) ====================
// Interrupción PCINT para teclado
// ISR(PCINT0_vect) {
//   teclaPresionada = true;
// }

// Interrupción botón seleccionar (INT0 - pin 2)
void isrBtnSeleccionar() {
  btnSeleccionarPresionado = true;
  // NO activar el otro
}

// Interrupción botón listo (INT1 - pin 3)
void isrBtnListo() {
  btnListoPresionado = true;
  // NO activar el otro
}

// Timer1 para escaneo periódico (opcional)
ISR(TIMER1_COMPA_vect) {
  timerTick = true;
}

// ==================== SETUP ====================
void setup() {
  // Inicializar Serial
  Serial.begin(9600);
  delay(100);
  Serial.println();
  Serial.println("================================");
  Serial.println("  Sistema Restaurante v2.0");
  Serial.println("================================");
  Serial.println("Botones: Pin 2 (Seleccionar), Pin 3 (Listo)");
  Serial.println("Presiona un boton para probar...");
  Serial.println();
  // Serial.println("[DEBUG] Entrando a setup()");
  
  // Inicializar Serial COCINA (Requerimiento TX2/RX2)
  Serial2.begin(9600); 

  // Inicializar LCD
  // Serial.println("[DEBUG] Inicializando LCD");
  lcd_init();
  lcd_clear();
  lcd_setCursor(2, 0);
  lcd_print("RESTAURANTE");
  lcd_setCursor(4, 1);
  lcd_print("v2.0");
  delay(2000);
  delay(2000);
  // Inicializar teclado
  // Serial.println("[DEBUG] Inicializando teclado");
  initTeclado();
  
  // Inicializar RFID
  // Serial.println("[DEBUG] Inicializando RFID");
  initRFID();
  
  // Inicializar 7 segmentos
  // Serial.println("[DEBUG] Inicializando display 7 segmentos");
  init7Seg();
  
  // Inicializar LED alarma
  // Serial.println("[DEBUG] Inicializando LED alarma");
  pinMode(LED_ALARMA, OUTPUT);
  digitalWrite(LED_ALARMA, LOW);
  
  // Inicializar buzzer (opcional)
  // Serial.println("[DEBUG] Inicializando buzzer");
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Configurar botones cocina con interrupciones
  // Serial.println("[DEBUG] Configurando botones con interrupciones");
  pinMode(BTN_SELECCIONAR, INPUT);
  pinMode(BTN_LISTO, INPUT);
  attachInterrupt(digitalPinToInterrupt(BTN_SELECCIONAR), isrBtnSeleccionar, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN_LISTO), isrBtnListo, FALLING);
  
  // Inicializar cliente
  // Serial.println("[DEBUG] Inicializando clientes de mesa");
  for (int i = 0; i < MAX_CLIENTES; i++) {
    clientesMesa[i].total = 0;
    clientesMesa[i].items = "";
  }
  // Asegurar que exista al menos un cliente al iniciar (Cliente 1)
  totalClientes = 1;
  clienteActual = 0;
  lcd_clear();
  lcd_setCursor(0,0);
  lcd_print("Cliente 1 agregado");
  delay(1000);
  
  // Habilitar interrupciones globales
  // Serial.println("[DEBUG] Habilitando interrupciones globales");
  sei();
  
  // Mostrar menú principal
  // Serial.println("[DEBUG] Mostrando menú principal");
  mostrarMenuPrincipal();
  estadoActual = MENU_PRINCIPAL;
  // Serial.println("[DEBUG] Fin de setup()");
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
  char tecla = 0;

  // --- Procesar teclado (SOLO SI HUBO INTERRUPCIÓN) ---
  if (teclaPresionada) {
    teclaPresionada = false;
    tecla = leerTecla();
  } 

  // --- Procesar RFID (SOLO SI HUBO INTERRUPCIÓN) ---
  // La función procesarRFID ahora verifica la bandera tarjetaDetectadaIRQ internamente
  procesarRFID(); 

  // --- Procesar botones cocina (con debounce) ---
  static unsigned long lastBtnTime = 0;
  unsigned long now = millis();

  // Solo procesar si pasaron 400ms desde última pulsación
  if (now - lastBtnTime > 400) {
    if (btnSeleccionarPresionado) {
      btnSeleccionarPresionado = false;
      btnListoPresionado = false;  // Limpiar el otro también
      lastBtnTime = now;
      Serial.println("-> Ejecutando SELECCIONAR");
      procesarBotonSeleccionar();
    }
    else if (btnListoPresionado) {
      btnListoPresionado = false;
      btnSeleccionarPresionado = false;  // Limpiar el otro también
      lastBtnTime = now;
      Serial.println("-> Ejecutando LISTO");
      procesarBotonListo();
    }
  } else {
    // Ignorar pulsaciones muy rápidas
    btnSeleccionarPresionado = false;
    btnListoPresionado = false;
  }

  // --- Procesar tecla según estado ---
  if (tecla) {
    procesarTecla(tecla);
  }

  // --- Actualizar display periódicamente ---
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 500) {
    lastUpdate = millis();
    actualizarDisplay();
  }
  // Serial.println("[DEBUG] loop() fin");
}

// ==================== PROCESAMIENTO DE TECLAS ====================
void procesarTecla(char tecla) {
  switch (estadoActual) {
    case MENU_PRINCIPAL:
      procesarMenuPrincipal(tecla);
      break;
      
    case SELECCION_CATEGORIA:
      procesarSeleccionCategoria(tecla);
      break;
      
    case MODO_RECARGA:
      if (tecla == 'C') {
        // Cancelar recarga
        estadoActual = MENU_PRINCIPAL;
        mostrarMenuPrincipal();
      }
      break;
      
    case INGRESANDO_MONTO:
      procesarIngresoMonto(tecla);
      break;
      
    case ESPERANDO_PAGO:
      if (tecla == 'C') {
        // Cancelar pago
        estadoActual = MENU_PRINCIPAL;
        mostrarMenuPrincipal();
      }
      break;
      
    default:
      break;
  }
}

void procesarMenuPrincipal(char tecla) {
  switch (tecla) {
    case 'A':  // Nuevo pedido
      
      catActual = 0;
      estadoActual = SELECCION_CATEGORIA;
      
      // AGREGAMOS: Mostrar el título de la primera categoría (Entradas)
      mostrarTituloCategoria(catActual);
      
      mostrarCategoria();
      break;
      
    case 'B':  // Modo recarga
      estadoActual = MODO_RECARGA;
      mostrarModoRecarga();
      break;
      
    case 'D':  // Pagar - Mostrar cuenta del cliente actual
      if (clientesMesa[clienteActual].total > 0) {
        estadoActual = ESPERANDO_PAGO;
        lcd_clear();
        lcd_setCursor(0, 0);
        lcd_print("Cli");
        lcd_printNum(clienteActual + 1);
        lcd_print(" Debe: $");
        lcd_printNum(clientesMesa[clienteActual].total);
        lcd_setCursor(0, 1);
        lcd_print("Acerque tarjeta");
      } else {
        lcd_clear();
        lcd_print("Cli");
        lcd_printNum(clienteActual + 1);
        lcd_print(" no debe!");
        delay(1500);
        mostrarMenuPrincipal();
      }
      break;
    case '0':
      // Mostrar menú en modo roll para que el cliente vea opciones
      mostrarRollMenu();
      break;
      
    case '#':  // Agregar cliente
      if (totalClientes < MAX_CLIENTES) {
        totalClientes++;
        clienteActual = totalClientes - 1;
        lcd_clear();
        lcd_print("Cliente ");
        lcd_printNum(totalClientes);
        lcd_print(" agregado");
        delay(1500);
        mostrarMenuPrincipal();
      } else {
        lcd_clear();
        lcd_print("Max 5 clientes!");
        delay(1500);
        mostrarMenuPrincipal();
      }
      break;
      
    case '*':  // Cambiar cliente
      clienteActual = (clienteActual + 1) % totalClientes;
      mostrarTotalCliente();
      delay(1500);
      mostrarMenuPrincipal();
      break;
  }
}

void procesarSeleccionCategoria(char tecla) {
  // Tecla 0 = mostrar opciones disponibles
  if (tecla == '0') {
    mostrarOpcionesCategoria();
    return;
  }
  
  if (tecla >= '1' && tecla <= '3') {
    int opcion = tecla - '1';
    uint16_t precio = precios[catActual][opcion];
    
    if (precio > 0 || opcion == 2) {  // Opción válida o "Ninguna"
      // Permitir seleccionar cantidad si el ítem tiene precio
      uint16_t cantidad = 1;
      if (precio > 0) {
        cantidad = pedirCantidad();
      }
      uint32_t subtotal = (uint32_t)precio * cantidad;
      clientesMesa[clienteActual].total += subtotal;
      // Registrar item y cantidad en la lista del cliente
      clientesMesa[clienteActual].items += nombresItems[catActual][opcion];
      if (cantidad > 1) {
        clientesMesa[clienteActual].items += " x";
        clientesMesa[clienteActual].items += String(cantidad);
      }
      
      // Enviar a serial
      Serial.print("[Cli");
      Serial.print(clienteActual + 1);
      Serial.print("] +");
      Serial.print(nombresItems[catActual][opcion]);
      Serial.print(" x");
      Serial.print(cantidad);
      Serial.print(" $");
      Serial.println(subtotal);
      
      // Mostrar confirmación clara
      lcd_clear();
      lcd_setCursor(0, 0);
      lcd_print("+");
      lcd_print(nombresItems[catActual][opcion]);
      if (cantidad > 1) {
        lcd_print(" x");
        lcd_printNum(cantidad);
      }
      lcd_print(" ");
      lcd_print(preciosStr[catActual][opcion]);
      lcd_setCursor(0, 1);
      lcd_print("Total: $");
      lcd_printNum(clientesMesa[clienteActual].total);
      delay(1500);
      
      // Avanzar automáticamente a siguiente categoría
      if (catActual < 3) {
        catActual++;
        // AGREGAMOS: Mostrar título de la siguiente categoría
        mostrarTituloCategoria(catActual);
      }
      mostrarCategoria();
    }
  }
  else if (tecla == '#') {
    // Siguiente categoría (tecla #)
    if (catActual < 3) {
      catActual++;
      // AGREGAMOS: Mostrar título de la siguiente categoría
      mostrarTituloCategoria(catActual);
      mostrarCategoria();
    } else {
      // Finalizar pedido y enviarlo a cocina
      if (clientesMesa[clienteActual].total > 0) {
        enviarPedidoCocina();
      }
      catActual = 0;
      estadoActual = MENU_PRINCIPAL;
      mostrarMenuPrincipal();
    }
  }
  else if (tecla == 'C') {
    // Enviar pedido a cocina si hay algo
    if (clientesMesa[clienteActual].total > 0) {
      lcd_clear();
      lcd_setCursor(0, 0);
      lcd_print("ENVIAR PEDIDO?");
      lcd_setCursor(0, 1);
      lcd_print("#:Si  C:Seguir");
      
      // Esperar confirmación
      char conf = 0;
      while (!conf) {
        conf = leerTecla();
        if (conf == '#') {
          enviarPedidoCocina();
          estadoActual = MENU_PRINCIPAL;
          mostrarMenuPrincipal();
          return;
        } else if (conf == 'C') {
          mostrarCategoria();
          return;
        }
        conf = 0;
        delay(50);
      }
    } else {
      lcd_clear();
      lcd_print("Pedido vacio!");
      delay(1500);
      mostrarCategoria();
    }
  }
  else if (tecla == 'A') {
    // Volver al menú principal
    estadoActual = MENU_PRINCIPAL;
    mostrarMenuPrincipal();
  }
}

void procesarIngresoMonto(char tecla) {
  if (tecla >= '0' && tecla <= '9') {
    if (digitosIngresados < 6) {  // Máximo 999999
      montoIngresado = montoIngresado * 10 + (tecla - '0');
      digitosIngresados++;
      mostrarIngresoMonto();
    }
  }
  else if (tecla == '#') {
    // Confirmar recarga (tecla #)
    if (montoIngresado > 0) {
      uint32_t nuevoSaldo = saldoTarjeta + montoIngresado;
      
      lcd_clear();
      lcd_setCursor(0, 0);
      lcd_print("Pase la tarjeta");
      lcd_setCursor(0, 1);
      lcd_print("para recargar...");
      
      // Esperar tarjeta y escribir
      unsigned long timeout = millis() + 10000;  // 10 segundos
      while (millis() < timeout) {
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
          if (escribirSaldoRFID(nuevoSaldo)) {
            // Pantalla 1 - Confirmación
            lcd_clear();
            lcd_setCursor(0, 0);
            lcd_print("** RECARGADO **");
            lcd_setCursor(0, 1);
            lcd_print("+$");
            lcd_printNum(montoIngresado);
            delay(2000);
            
            // Pantalla 2 - Nuevo saldo
            lcd_clear();
            lcd_setCursor(0, 0);
            lcd_print("Nuevo saldo:");
            lcd_setCursor(0, 1);
            lcd_print("$");
            lcd_printNum(nuevoSaldo);
            delay(2000);
          } else {
            lcd_clear();
            lcd_print("Error tarjeta!");
            delay(2000);
          }
          break;
        }
        delay(50); // Pequeña pausa para no saturar
      } // <--- ESTA LLAVE FALTABA PARA CERRAR EL WHILE
      
      digitosIngresados = 0;
      estadoActual = MENU_PRINCIPAL;
      mostrarMenuPrincipal();
    }
  }
  else if (tecla == 'C') {
    // Cancelar (tecla C)
    montoIngresado = 0;
    digitosIngresados = 0;
    estadoActual = MENU_PRINCIPAL;
    mostrarMenuPrincipal();
  }
  else if (tecla == '*') {
    // Borrar último dígito (tecla *)
    if (digitosIngresados > 0) {
      montoIngresado /= 10;
      digitosIngresados--;
      mostrarIngresoMonto();
    }
  }
}

// ==================== PROCESAMIENTO RFID ====================
void procesarRFID() {
  // 1. VERIFICACIÓN ESTRICTA: Si no saltó la interrupción, NO HACER NADA.
  if (!tarjetaDetectadaIRQ) {
    return; 
  }

  // 2. Limpiar la interrupción en el chip RC522 para permitir futuras lecturas
  mfrc522.PCD_WriteRegister(MFRC522::ComIrqReg, 0x7F);

  // 3. Intentar leer
  if (!mfrc522.PICC_IsNewCardPresent()) {
    tarjetaDetectadaIRQ = false; // Falsa alarma
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    tarjetaDetectadaIRQ = false; // Error lectura
    return;
  }

  Serial.println("[INFO] Tarjeta detectada por INTERRUPCION.");

  // Leer saldo de la tarjeta
  byte block = 4;

  MFRC522::StatusCode status;
  status = mfrc522.PCD_Authenticate(
             MFRC522::PICC_CMD_MF_AUTH_KEY_A,
             block,
             &keyRFID,
             &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.print("[ERROR] Error de autenticación: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    tarjetaDetectadaIRQ = false;
    return;
  }

  // Leer bloque
  byte buffer[18];
  byte size = sizeof(buffer);

  status = mfrc522.MIFARE_Read(block, buffer, &size);

  if (status != MFRC522::STATUS_OK) {
    Serial.print("[ERROR] Error leyendo saldo: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    tarjetaDetectadaIRQ = false;
    return;
  }

  // Extraer saldo (4 bytes little-endian) - Corregido para evitar warnings
  saldoTarjeta = (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) | ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);

  Serial.print("[OK] Saldo leído: $");
  Serial.println(saldoTarjeta);
  // Procesar según estado actual
  switch (estadoActual) {
    case MODO_RECARGA:
      // Mostrar saldo actual e ingresar monto
      lcd_clear();
      lcd_print("Saldo: $");
      lcd_printNum(saldoTarjeta);
      lcd_setCursor(0, 1);
      lcd_print("Ingrese monto...");
      delay(1500);

      montoIngresado = 0;
      digitosIngresados = 0;
      estadoActual = INGRESANDO_MONTO;
      mostrarIngresoMonto();
      break;

    case ESPERANDO_PAGO:
      realizarPago();
      break;

    default:
      // Solo mostrar saldo
      lcd_clear();
      lcd_print("Saldo: $");
      lcd_printNum(saldoTarjeta);
      delay(2000);
      mostrarMenuPrincipal();
      break;
  }

  tarjetaDetectadaIRQ = false; // Resetear bandera
}

void realizarPago() {
  uint32_t totalAPagar = clientesMesa[clienteActual].total;
  
  if (saldoTarjeta >= totalAPagar) {
    uint32_t nuevoSaldo = saldoTarjeta - totalAPagar;
    
    // Indicar al usuario que mantenga la tarjeta mientras se procesa
    lcd_clear();
    lcd_setCursor(0,0);
    lcd_print("Mantenga tarjeta");
    lcd_setCursor(0,1);
    lcd_print("Procesando pago...");
    delay(300);

    // Escribir nuevo saldo usando la tarjeta ya presente (UID disponible)
    if (escribirSaldoRFID(nuevoSaldo)) {
      // Pago exitoso - Pantalla 1
      lcd_clear();
      lcd_setCursor(0, 0);
      lcd_print("** PAGADO! **");
      lcd_setCursor(0, 1);
      lcd_print("Cobrado: $");
      lcd_printNum(totalAPagar);
      
      // Efecto visual
      digitalWrite(LED_ALARMA, HIGH);
      delay(500);
      digitalWrite(LED_ALARMA, LOW);
      delay(1500);
      
      // Pantalla 2 - Saldo restante
      lcd_clear();
      lcd_setCursor(0, 0);
      lcd_print("Saldo tarjeta:");
      lcd_setCursor(0, 1);
      lcd_print("$");
      lcd_printNum(nuevoSaldo);
      
      // Enviar confirmación serial
      Serial.println("================");
      Serial.print("PAGO Cliente ");
      Serial.println(clienteActual + 1);
      Serial.print("Cobrado: $");
      Serial.println(totalAPagar);
      Serial.println("================");
      
      delay(2000);
      
      // Limpiar cliente
      clientesMesa[clienteActual].total = 0;
      clientesMesa[clienteActual].items = "";
      
      // Quitar TODOS los pedidos pendientes asociados a este cliente
      int idx = 0;
      while (idx < numPedidosActivos) {
        if (listaPedidos[idx].cliente == clienteActual && listaPedidos[idx].pagado == false) {
          // mover los siguientes una posición atrás
          for (int j = idx; j < numPedidosActivos - 1; j++) {
            listaPedidos[j] = listaPedidos[j + 1];
          }
          numPedidosActivos--;
          // no incrementar idx; revisar el nuevo elemento en esta posición
        } else {
          idx++;
        }
      }

      // Depuración: mostrar estado de listaPedidos
      DBG_PRINT("[DEBUG] numPedidosActivos=");
      DBG_PRINTLN(numPedidosActivos);
      for (int k = 0; k < numPedidosActivos; k++) {
        DBG_PRINT("[DEBUG] Pedido "); DBG_PRINT(k);
        DBG_PRINT(" num="); DBG_PRINT(listaPedidos[k].numero);
        DBG_PRINT(" cliente="); DBG_PRINT(listaPedidos[k].cliente);
        DBG_PRINT(" total="); DBG_PRINT(listaPedidos[k].total);
        DBG_PRINTLN(listaPedidos[k].pagado);
      }

        // Después de quitar pedidos pendientes, borrar el cliente del LCD
        eliminarClienteAt(clienteActual);

        // Asegurarse de liberar tarjeta tras operación
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();

        estadoActual = MENU_PRINCIPAL;
        mostrarMenuPrincipal();
    } else {
      // En caso de fallo al escribir, asegurar limpieza y notificar
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      lcd_clear();
      lcd_print("Error tarjeta!");
      delay(2000);
    }
  } else {
    // Saldo insuficiente
    lcd_clear();
    lcd_setCursor(0, 0);
    lcd_print("Saldo: $");
    lcd_printNum(saldoTarjeta);
    lcd_setCursor(0, 1);
    lcd_print("Falta: $");
    lcd_printNum(totalAPagar - saldoTarjeta);
    delay(2500);
    
    lcd_clear();
    lcd_setCursor(0, 0);
    lcd_print("Que hacer?");
    lcd_setCursor(0, 1);
    lcd_print("C=Canc B=Recargar");
    
    char resp = 0;
    while (!resp) {
      resp = leerTecla();
      if (resp == 'C') {
        estadoActual = MENU_PRINCIPAL;
        mostrarMenuPrincipal();
        return;
      } else if (resp == 'B') {
        estadoActual = MODO_RECARGA;
        mostrarModoRecarga();
        return;
      }
      resp = 0;
      delay(50);
    }
  }
}

// ==================== ENVÍO A COCINA ====================
void enviarPedidoCocina() {
  // Solo enviar el pedido del cliente actual a cocina
  uint32_t totalCliente = clientesMesa[clienteActual].total;
  
  if (totalCliente == 0) {
    lcd_clear();
    lcd_print("Sin pedido!");
    delay(1500);
    return;
  }
  
  // Agregar a cola de cocina (solo para preparación)
  if (!agregarComanda(numPedidoGlobal, totalCliente)) {
    return;  // Error ya mostrado
  }
  
  // Enviar por serial
  Serial2.println("================");
  Serial2.print("PEDIDO #");
  Serial2.println(numPedidoGlobal);
  Serial2.print("Cliente ");
  Serial2.print(clienteActual + 1);
  Serial2.print(": $");
  Serial2.println(totalCliente);
  Serial2.println("================");
  
  // Debug en PC
  Serial.println("[DEBUG] Enviado a cocina por Serial2 (Pines 16/17)");

  // Mostrar confirmación clara - Pantalla 1
  lcd_clear();
  lcd_setCursor(0, 0);
  lcd_print("** ORDENADO! **");
  lcd_setCursor(0, 1);
  lcd_print("Pedido #");
  lcd_printNum(numPedidoGlobal);
  delay(2000);
  
  // Pantalla 2 - Total a pagar
  lcd_clear();
  lcd_setCursor(0, 0);
  lcd_print("Enviado a cocina");
  lcd_setCursor(0, 1);
  lcd_print("Cuenta: $");
  lcd_printNum(totalCliente);
  delay(2000);
  
  // IMPORTANTE: Guardar el pedido para cobrar después
  // Registrar en lista de pedidos pendientes de pago
  if (numPedidosActivos < MAX_PEDIDOS) {
    listaPedidos[numPedidosActivos].numero = numPedidoGlobal;
    listaPedidos[numPedidosActivos].total = totalCliente;
    listaPedidos[numPedidosActivos].pagado = false;
    listaPedidos[numPedidosActivos].cliente = clienteActual;
    numPedidosActivos++;
    // Depuración: mostrar estado de listaPedidos tras agregar
    DBG_PRINT("[DEBUG] Añadido pedido. numPedidosActivos=");
    DBG_PRINTLN(numPedidosActivos);
    for (int k = 0; k < numPedidosActivos; k++) {
      DBG_PRINT("[DEBUG] Pedido "); DBG_PRINT(k);
      DBG_PRINT(" num="); DBG_PRINT(listaPedidos[k].numero);
      DBG_PRINT(" cliente="); DBG_PRINT(listaPedidos[k].cliente);
      DBG_PRINT(" total="); DBG_PRINT(listaPedidos[k].total);
      DBG_PRINTLN(listaPedidos[k].pagado);
    }
  }
  
  // Limpiar items del cliente (ya se enviaron) pero MANTENER el total para cobrar
  clientesMesa[clienteActual].items = "";
  // NO limpiar el total aquí - se limpia al pagar
  
  numPedidoGlobal++;
}

// Eliminar cliente del arreglo `clientesMesa` en la posición `idx`.
// Si solo hay 1 cliente, limpiamos su contenido pero no reducimos totalClients.
void eliminarClienteAt(uint8_t idx) {
  if (idx >= totalClientes) return;
  if (totalClientes <= 1) {
    // Mantener al menos un cliente, simplemente limpiar
    clientesMesa[0].total = 0;
    clientesMesa[0].items = "";
    clienteActual = 0;
    totalClientes = 1;
    return;
  }

  // Desplazar clientes posteriores hacia la izquierda
  for (int i = idx; i < totalClientes - 1; i++) {
    clientesMesa[i] = clientesMesa[i + 1];
  }

  // Limpiar la última posición y reducir el contador
  clientesMesa[totalClientes - 1].total = 0;
  clientesMesa[totalClientes - 1].items = "";
  totalClientes--;

  // Ajustar clienteActual para que apunte a un cliente válido
  if (clienteActual >= totalClientes) {
    clienteActual = (totalClientes == 0) ? 0 : (totalClientes - 1);
  }
}

// ==================== BOTONES COCINA ====================
void procesarBotonSeleccionar() {
  Serial.println(">> BTN SELECCIONAR presionado");
  
  if (colaTam == 0) {
    Serial.println("   Cola vacia");
    lcd_clear();
    lcd_print("Cola vacia!");
    delay(1000);
    mostrarMenuPrincipal();
    return;
  }
  
  // Avanzar al siguiente pedido
  pedidoSeleccionado = (pedidoSeleccionado + 1) % colaTam;
  Comanda c = obtenerComandaActual();
  
  Serial.print("   Pedido #");
  Serial.println(c.numero);
  
  mostrar7Seg(c.numero % 10);
  
  lcd_clear();
  lcd_print("Pedido #");
  lcd_printNum(c.numero);
  lcd_setCursor(0, 1);
  lcd_print("Total: $");
  lcd_printNum(c.total);
  // Mostrar indicador Sel: X/Y en esquina derecha
  int selIndex = (pedidoSeleccionado >= 0) ? (pedidoSeleccionado + 1) : 1;
  lcd_setCursor(12, 0);
  lcd_print("S");
  lcd_printNum(selIndex);
  lcd_print("/");
  lcd_printNum(colaTam);
}

void procesarBotonListo() {
  unsigned long now = millis();
  if (now - lastListoTime < 800) return; // debounce/bloqueo rápido
  lastListoTime = now;

  Serial.println(">> BTN LISTO presionado");
  
  if (colaTam == 0) {
    Serial.println("   Cola vacia, ignorando");
    lcd_clear();
    lcd_print("Cola vacia!");
    delay(1000);
    mostrarMenuPrincipal();
    return;
  }
  
  // Obtener el pedido SELECCIONADO actualmente
  Comanda c = obtenerComandaActual();
  
  Serial.print("   Marcando listo pedido #");
  Serial.println(c.numero);
  
  // Remover el pedido SELECCIONADO (no el primero)
  removerComandaSeleccionada();
  
  // Encender LED alarma
  digitalWrite(LED_ALARMA, HIGH);
  
  // Enviar señal a mesero
  Serial.print("K");
  Serial.println(c.numero);
  
  lcd_clear();
  lcd_print("Pedido #");
  lcd_printNum(c.numero);
  lcd_setCursor(0, 1);
  lcd_print("LISTO! Recoger");
  
  delay(3000);
  digitalWrite(LED_ALARMA, LOW);
  
  mostrarMenuPrincipal();
}

// ==================== ACTUALIZACIÓN DISPLAY ====================
void actualizarDisplay() {
  // Parpadeo LED si hay pedidos listos (opcional)
  if (colaTam > 0 && estadoActual == MENU_PRINCIPAL) {
    // Mostrar contador de cola
    lcd_setCursor(14, 0);
    lcd_print("C");
    if (colaTam < 10) lcd_print("0");
    lcd_printNum(colaTam);
  }
}