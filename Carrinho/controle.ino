#include <Arduino.h>
#include <EEPROM.h>
#include <Servo.h>
#include <SoftwareSerial.h>

SoftwareSerial meuBT(10, 11);

// --- HARDWARE ---
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
float distanciaEsq = 0, distanciaDir = 0;
volatile unsigned long pulsosEsq = 0, pulsosDir = 0;

// --- FILA RAM ---
#define MAX_FILA_RAM 30
#define MAX_COMANDO_LEN 20
char filaRAM[MAX_FILA_RAM][MAX_COMANDO_LEN];
byte totalFilaRAM = 0;

Servo meuServo;
byte posicaoServo = 0;

// --- RESPOSTA ---
void responder(String msg) { Serial.println(msg); meuBT.println(msg); }

// --- ENCODERS ---
void contarEsq() { pulsosEsq++; }
void contarDir() { pulsosDir++; }
void posicao() {
    noInterrupts();
    unsigned long pe = pulsosEsq; unsigned long pd = pulsosDir;
    pulsosEsq = 0; pulsosDir = 0;
    interrupts();
    float perim = diametroRoda_cm * pi;
    distanciaEsq = (float)(pe / (float)furosDoDisco) * perim;
    distanciaDir = (float)(pd / (float)furosDoDisco) * perim;
}

void parar() {
  digitalWrite(MOTOR_ESQ_FRENTE, LOW); digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, LOW); digitalWrite(MOTOR_DIR_TRAS, LOW);
}

// --- MOVIMENTOS ---
bool wait_movement(float distMetaEsq, float distMetaDir, unsigned long timeoutMs) {
    unsigned long inicio = millis();
    float acEsq = 0, acDir = 0;
    posicao(); 
    while(true) {
        if (millis() - inicio > timeoutMs) { parar(); responder("ERR_TIMEOUT"); return false; }
        bool esqOk = (distMetaEsq <= 0) || (acEsq >= distMetaEsq);
        bool dirOk = (distMetaDir <= 0) || (acDir >= distMetaDir);
        if (esqOk && dirOk) break;
        
        if (!esqOk) { digitalWrite(MOTOR_ESQ_FRENTE, HIGH); digitalWrite(MOTOR_ESQ_TRAS, LOW); }
        else { digitalWrite(MOTOR_ESQ_FRENTE, LOW); digitalWrite(MOTOR_ESQ_TRAS, LOW); }
        
        if (!dirOk) { digitalWrite(MOTOR_DIR_FRENTE, HIGH); digitalWrite(MOTOR_DIR_TRAS, LOW); }
        else { digitalWrite(MOTOR_DIR_FRENTE, LOW); digitalWrite(MOTOR_DIR_TRAS, LOW); }

        posicao(); acEsq += distanciaEsq; acDir += distanciaDir;
        delay(10);
    }
    parar(); return true;
}

void andarFrente(int dist) {
  if (dist <= 0) return;
  if (dist > 200) dist = 200;
  wait_movement(dist, dist, 3000 + (dist * 100));
}

void andarTras(int dist) {
  if (dist <= 0) return;
  float da = 0; posicao();
  while(da < dist){
    digitalWrite(MOTOR_ESQ_FRENTE, LOW); digitalWrite(MOTOR_ESQ_TRAS, HIGH);
    digitalWrite(MOTOR_DIR_FRENTE, LOW); digitalWrite(MOTOR_DIR_TRAS, HIGH);
    posicao(); da += distanciaEsq; delay(10);
  }
  parar();
}

void curvaEsquerda(int graus) {
  if (graus <= 0) return;
  float alvo = graus * (20.5 * 0.0174533); 
  unsigned long ini = millis(); float ac = 0; posicao();
  while(ac < alvo){
    if (millis() - ini > 5000) { parar(); responder("ERR_TIMEOUT_G"); return; }
    digitalWrite(MOTOR_ESQ_FRENTE, LOW); digitalWrite(MOTOR_ESQ_TRAS, LOW);
    digitalWrite(MOTOR_DIR_FRENTE, HIGH); digitalWrite(MOTOR_DIR_TRAS, LOW);
    posicao(); ac += distanciaDir; delay(10);
  }
  parar();
}

void curvaDireita(int graus) {
  if (graus <= 0) return;
  float alvo = graus * (20.5 * 0.0174533); 
  unsigned long ini = millis(); float ac = 0; posicao();
  while(ac < alvo){
    if (millis() - ini > 5000) { parar(); responder("ERR_TIMEOUT_G"); return; }
    digitalWrite(MOTOR_ESQ_FRENTE, HIGH); digitalWrite(MOTOR_ESQ_TRAS, LOW);
    digitalWrite(MOTOR_DIR_FRENTE, LOW); digitalWrite(MOTOR_DIR_TRAS, LOW);
    posicao(); ac += distanciaEsq; delay(10);
  }
  parar();
}

// --- FUNÇÃO DO OVO (SERVO) ---
void depositarOvo() {
  // Movimento de "Varredura" para soltar o ovo
  meuServo.write(90); // Abre/Empurra
  delay(1000);        // Espera cair
  meuServo.write(0);  // Retorna/Fecha
  delay(500);
}

// --- GERENCIAMENTO FILA ---
void limparFilaRAM() { totalFilaRAM = 0; responder("OK_CLR"); }

void adicionarNaFilaRAM(char* cmd) {
  if (totalFilaRAM >= MAX_FILA_RAM) { responder("ERR_FULL"); return; }
  strncpy(filaRAM[totalFilaRAM], cmd, MAX_COMANDO_LEN - 1);
  filaRAM[totalFilaRAM][MAX_COMANDO_LEN - 1] = '\0';
  totalFilaRAM++;
  responder("OK_ADD"); 
}

void processarComando(const char* cmd, bool fromRAM);

void executarFilaRAM() {
  if (totalFilaRAM == 0) { responder("ERR_EMPTY"); return; }
  responder("OK_RUN"); 
  for (byte i = 0; i < totalFilaRAM; i++) {
    char temp[MAX_COMANDO_LEN]; strcpy(temp, filaRAM[i]);
    processarComando(temp, true); 
    String msg = "STEP_DONE "; msg += i; responder(msg);
    delay(200); 
  }
  responder("FINISH"); totalFilaRAM = 0; 
}

void filtrar(char* str) {
  byte i = 0, j = 0;
  while (str[i] != '\0' && j < MAX_COMANDO_LEN - 1) {
    char c = str[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
        (c >= '0' && c <= '9') || c == '(' || c == ')' || c == '-') {
      if (c >= 'a' && c <= 'z') str[j++] = c - 32; else str[j++] = c;
    } else if (c == '@' || c == '\\') break;
    i++;
  }
  str[j] = '\0';
}

void processarComando(const char* comando, bool fromRAM) {
  char cmdLimpo[MAX_COMANDO_LEN];
  strncpy(cmdLimpo, comando, MAX_COMANDO_LEN - 1); cmdLimpo[MAX_COMANDO_LEN - 1] = '\0';
  filtrar(cmdLimpo); if (strlen(cmdLimpo) == 0) return;

  if (strcmp(cmdLimpo, "LIMPARFILA") == 0) { limparFilaRAM(); return; }
  else if (strcmp(cmdLimpo, "EXECUTAR") == 0) { executarFilaRAM(); return; }
  else if (strncmp(cmdLimpo, "ADD(", 4) == 0) {
     char sub[MAX_COMANDO_LEN]; byte i=4, j=0;
     while (cmdLimpo[i] != ')' && cmdLimpo[i] != '\0') sub[j++] = cmdLimpo[i++];
     sub[j] = '\0'; adicionarNaFilaRAM(sub); return;
  }

  // --- COMANDOS FISICOS ---
  if (cmdLimpo[0] == 'F' && cmdLimpo[1] == '(') andarFrente(atoi(cmdLimpo + 2));
  else if (cmdLimpo[0] == 'T' && cmdLimpo[1] == '(') andarTras(atoi(cmdLimpo + 2));
  else if (cmdLimpo[0] == 'G' && cmdLimpo[1] == '(') {
      int ang = atoi(cmdLimpo + 2); if (ang >= 0) curvaDireita(ang); else curvaEsquerda(-ang);
  }
  else if (cmdLimpo[0] == 'E' && cmdLimpo[1] == '(') curvaEsquerda(atoi(cmdLimpo + 2));
  else if (cmdLimpo[0] == 'D' && cmdLimpo[1] == '(') curvaDireita(atoi(cmdLimpo + 2));
  else if (cmdLimpo[0] == 'S' && cmdLimpo[1] == '(') { meuServo.write(atoi(cmdLimpo + 2)); }
  
  // --- NOVO COMANDO: OVO ---
  else if (cmdLimpo[0] == 'O' && cmdLimpo[1] == '(') { depositarOvo(); } 
  // -------------------------
  
  else if (strcmp(cmdLimpo, "PARAR") == 0) parar();
}

void setup() {
  Serial.begin(9600); meuBT.begin(9600);
  pinMode(MOTOR_ESQ_FRENTE, OUTPUT); pinMode(MOTOR_ESQ_TRAS, OUTPUT);
  pinMode(MOTOR_DIR_FRENTE, OUTPUT); pinMode(MOTOR_DIR_TRAS, OUTPUT);
  pinMode(pinoEncoderEsq, INPUT_PULLUP); pinMode(pinoEncoderDir, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinoEncoderEsq), contarEsq, FALLING);
  attachInterrupt(digitalPinToInterrupt(pinoEncoderDir), contarDir, FALLING);
  meuServo.attach(SERVO_PIN); meuServo.write(0);
  responder("EGG0-1 FINAL READY");
}

void loop() {
  if (meuBT.available()) {
    char buf[MAX_COMANDO_LEN]; byte idx = 0; unsigned long ini = millis();
    while (meuBT.available() && idx < MAX_COMANDO_LEN - 1) {
      char c = meuBT.read(); if (c == '\\') break; buf[idx++] = c;
      if (millis() - ini > 100) break; 
    }
    buf[idx] = '\0'; if (idx > 0) processarComando(buf, false);
  }
  if (Serial.available()) {
     char buf[MAX_COMANDO_LEN]; byte idx = 0;
     while (Serial.available() && idx < MAX_COMANDO_LEN - 1) {
       char c = Serial.read(); if (c == '\n' || c == '\\') break; buf[idx++] = c; delay(2);
     }
     buf[idx] = '\0'; if (idx > 0) processarComando(buf, false);
  }
}