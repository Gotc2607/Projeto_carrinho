#include <Arduino.h>
#include <EEPROM.h>
#include <Servo.h>
#include <SoftwareSerial.h>

SoftwareSerial meuBT(10, 11);

// ---------------------------
// CONFIGURAÇÃO DOS PINOS
// ---------------------------
#define MOTOR_ESQ_FRENTE 5
#define MOTOR_ESQ_TRAS   4
#define MOTOR_DIR_FRENTE 8
#define MOTOR_DIR_TRAS   7
#define SERVO_PIN        6
#define pinoEncoderEsq   2
#define pinoEncoderDir   3
#define furosDoDisco     20

const float diametroRoda_cm = 6.3;
const float pi = 3.1415;
float distanciaEsq = 0;
float distanciaDir = 0;

volatile unsigned long pulsosEsq = 0;
volatile unsigned long pulsosDir = 0;

// ---------------------------
// FILA RAM
// ---------------------------
#define MAX_FILA_RAM 30 
char filaRAM[MAX_FILA_RAM][20];
byte totalFilaRAM = 0;

// ---------------------------
// SERVO
// ---------------------------
Servo meuServo;
byte posicaoServo = 0;
bool servoScanning = false;
unsigned long lastServoMove = 0;
byte servoScanAngle = 0;
bool servoScanDirection = true;

// ---------------------------
// MEMÓRIA (EEPROM)
// ---------------------------
#define MAX_COMANDO_LEN 20

// Interrupções
void contarEsq() { pulsosEsq++; }
void contarDir() { pulsosDir++; }

// ---------------------------
// FUNÇÕES DE MOVIMENTO (Idênticas às anteriores)
// ---------------------------
void posicao() {
    noInterrupts();
    unsigned long pulsosEsqCopia = pulsosEsq;
    unsigned long pulsosDirCopia = pulsosDir;
    pulsosEsq = 0;
    pulsosDir = 0;
    interrupts();

    float perimetro = diametroRoda_cm * pi;
    distanciaEsq = (float)(pulsosEsqCopia / (float)furosDoDisco) * perimetro;
    distanciaDir = (float)(pulsosDirCopia / (float)furosDoDisco) * perimetro;
}

void parar() {
  digitalWrite(MOTOR_ESQ_FRENTE, LOW);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, LOW);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
}

void andarFrente(int distancia) {
  if (distancia <= 0) return;
  if (distancia > 150) distancia = 150;
  float distAcumuladaEsq = 0; float distAcumuladaDir = 0;
  posicao();
  while(distAcumuladaEsq < distancia && distAcumuladaDir < distancia){
    digitalWrite(MOTOR_ESQ_FRENTE, HIGH); digitalWrite(MOTOR_ESQ_TRAS, LOW);
    digitalWrite(MOTOR_DIR_FRENTE, HIGH); digitalWrite(MOTOR_DIR_TRAS, LOW);
    posicao();
    distAcumuladaEsq += distanciaEsq; distAcumuladaDir += distanciaDir;
    delay(10);
  }
  parar();
}

void andarTras(int distancia) {
  if (distancia <= 0) return;
  float distAcumulada = 0;
  posicao();
  while(distAcumulada < distancia){
    digitalWrite(MOTOR_ESQ_FRENTE, LOW); digitalWrite(MOTOR_ESQ_TRAS, HIGH);
    digitalWrite(MOTOR_DIR_FRENTE, LOW); digitalWrite(MOTOR_DIR_TRAS, HIGH);
    posicao();
    distAcumulada += distanciaEsq;
    delay(10);
  }
  parar();
}

void curvaEsquerda(int graus) {
  if (graus <= 0) return;
  if (graus > 360) graus = 360;
  float distAcumuladaDir = 0;
  posicao();
  while((distAcumuladaDir / (20.5 * 0.0174533)) < graus){
    digitalWrite(MOTOR_ESQ_FRENTE, LOW); digitalWrite(MOTOR_ESQ_TRAS, LOW);
    digitalWrite(MOTOR_DIR_FRENTE, HIGH); digitalWrite(MOTOR_DIR_TRAS, LOW);
    posicao();
    distAcumuladaDir += distanciaDir;
    delay(10);
  }
  parar();
}

void curvaDireita(int graus) {
  if (graus <= 0) return;
  if (graus > 360) graus = 360;
  float distAcumuladaEsq = 0;
  posicao();
  while((distAcumuladaEsq / (20.5 * 0.0174533)) < graus){
    digitalWrite(MOTOR_ESQ_FRENTE, HIGH); digitalWrite(MOTOR_ESQ_TRAS, LOW);
    digitalWrite(MOTOR_DIR_FRENTE, LOW); digitalWrite(MOTOR_DIR_TRAS, LOW);
    posicao();
    distAcumuladaEsq += distanciaEsq;
    delay(10);
  }
  parar();
}

void moverServo(int angulo) {
  servoScanning = false;
  if (angulo < 0) angulo = 0;
  if (angulo > 180) angulo = 180;
  meuServo.write(angulo);
  posicaoServo = angulo;
}

void iniciarServoScan() {
  servoScanning = true;
  servoScanAngle = 0;
  servoScanDirection = true;
}

void atualizarServoScan() {
  if (!servoScanning) return;
  if (millis() - lastServoMove >= 20) {
    if (servoScanDirection) {
      servoScanAngle++;
      if (servoScanAngle >= 90) servoScanDirection = false;
    } else {
      servoScanAngle--;
      if (servoScanAngle <= 0) {
        servoScanDirection = true;
        servoScanning = false;
      }
    }
    meuServo.write(servoScanAngle);
    lastServoMove = millis();
  }
}

// ---------------------------
// GERENCIAMENTO
// ---------------------------
void limparFilaRAM() {
  totalFilaRAM = 0;
  Serial.println(F("OK_CLR")); 
}

void adicionarNaFilaRAM(char* cmd) {
  if (totalFilaRAM >= MAX_FILA_RAM) {
    Serial.println(F("ERR_FULL"));
    return;
  }
  if (strlen(cmd) < 3) {
    Serial.print(F("LOG:Erro_Tamanho_Cmd:")); // LOG DE ERRO
    Serial.println(cmd);
    Serial.println(F("ERR_FMT"));
    return;
  }
  strncpy(filaRAM[totalFilaRAM], cmd, MAX_COMANDO_LEN - 1);
  filaRAM[totalFilaRAM][MAX_COMANDO_LEN - 1] = '\0';
  totalFilaRAM++;
  
  Serial.println(F("OK_ADD")); 
}

void processarComando(const char* comando, bool fromRAM);

void executarFilaRAM() {
  if (totalFilaRAM == 0) {
    Serial.println(F("ERR_EMPTY"));
    return;
  }
  Serial.println(F("OK_RUN"));
  
  for (byte i = 0; i < totalFilaRAM; i++) {
    char cmdTemp[MAX_COMANDO_LEN];
    strcpy(cmdTemp, filaRAM[i]);
    processarComando(cmdTemp, true); 
    delay(300);
  }
  Serial.println(F("FINISH"));
  totalFilaRAM = 0; 
}

// ---------------------------
// PROCESSAMENTO
// ---------------------------
void filtrarComandoDabble(char* str) {
  byte i = 0, j = 0;
  while (str[i] != '\0' && j < MAX_COMANDO_LEN - 1) {
    char c = str[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
        (c >= '0' && c <= '9') || c == '(' || c == ')' || c == '-') {
      if (c >= 'a' && c <= 'z') str[j++] = c - 32;
      else str[j++] = c;
    }
    else if (c == '@' || c == '\\') break;
    i++;
  }
  str[j] = '\0';
}

void processarComando(const char* comando, bool fromRAM) {
  char cmdLimpo[MAX_COMANDO_LEN];
  strncpy(cmdLimpo, comando, MAX_COMANDO_LEN - 1);
  cmdLimpo[MAX_COMANDO_LEN - 1] = '\0';
  filtrarComandoDabble(cmdLimpo);
  
  if (strlen(cmdLimpo) == 0) return;

  // Comandos de Sistema
  if (strcmp(cmdLimpo, "LIMPARFILA") == 0) {
    limparFilaRAM();
    return;
  }
  else if (strcmp(cmdLimpo, "EXECUTAR") == 0) {
    executarFilaRAM();
    return;
  }
  // Captura ADD
  else if (strncmp(cmdLimpo, "ADD(", 4) == 0) {
     char subComando[MAX_COMANDO_LEN];
     byte i = 4, j = 0;
     while (cmdLimpo[i] != ')' && cmdLimpo[i] != '\0') {
       subComando[j++] = cmdLimpo[i++];
     }
     subComando[j] = '\0';
     adicionarNaFilaRAM(subComando);
     return;
  }

  // --- MOVIMENTOS ---
  if (!fromRAM) Serial.println(F("CMD_DIRECT")); 

  if (cmdLimpo[0] == 'F' && cmdLimpo[1] == '(') andarFrente(atoi(cmdLimpo + 2));
  else if (cmdLimpo[0] == 'T' && cmdLimpo[1] == '(') andarTras(atoi(cmdLimpo + 2));
  else if (cmdLimpo[0] == 'G' && cmdLimpo[1] == '(') {
      int ang = atoi(cmdLimpo + 2);
      if (ang >= 0) curvaDireita(ang); else curvaEsquerda(-ang);
  }
  else if (cmdLimpo[0] == 'E' && cmdLimpo[1] == '(') curvaEsquerda(atoi(cmdLimpo + 2));
  else if (cmdLimpo[0] == 'D' && cmdLimpo[1] == '(') curvaDireita(atoi(cmdLimpo + 2));
  else if (cmdLimpo[0] == 'S' && cmdLimpo[1] == '(') moverServo(atoi(cmdLimpo + 2));
  else if (strcmp(cmdLimpo, "PARAR") == 0) parar();
  else {
      // LOG DE COMANDO DESCONHECIDO
      Serial.print(F("LOG:Desconhecido:"));
      Serial.println(cmdLimpo);
  }
}

// ---------------------------
// SETUP & LOOP
// ---------------------------
void setup() {
  Serial.begin(9600);
  meuBT.begin(9600);
  
  pinMode(MOTOR_ESQ_FRENTE, OUTPUT); pinMode(MOTOR_ESQ_TRAS, OUTPUT);
  pinMode(MOTOR_DIR_FRENTE, OUTPUT); pinMode(MOTOR_DIR_TRAS, OUTPUT);
  pinMode(pinoEncoderEsq, INPUT_PULLUP); pinMode(pinoEncoderDir, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinoEncoderEsq), contarEsq, FALLING);
  attachInterrupt(digitalPinToInterrupt(pinoEncoderDir), contarDir, FALLING);

  meuServo.attach(SERVO_PIN);
  meuServo.write(0);
  Serial.println(F("EGG0-1 ONLINE (LOGGER ATIVO)"));
}

void lerEntrada(Stream &porta) {
    char buffer[MAX_COMANDO_LEN];
    byte index = 0;
    unsigned long inicio = millis();
    bool fim = false;
    
    while (porta.available() && index < MAX_COMANDO_LEN - 1 && !fim) {
      char c = porta.read();
      if (c == '\\') fim = true;
      else buffer[index++] = c;
      
      // LOG DE TIMEOUT (Ruído ou cabo solto)
      if (millis() - inicio > 100) {
          Serial.println(F("LOG:Timeout_Leitura_Serial"));
          break;
      }
    }
    buffer[index] = '\0';
    if (index > 0) processarComando(buffer, false);
}

void loop() {
  if (meuBT.available()) lerEntrada(meuBT);
  if (Serial.available()) lerEntrada(Serial);
  atualizarServoScan();
}