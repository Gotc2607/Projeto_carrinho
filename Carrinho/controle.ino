#include <Arduino.h>
#include <EEPROM.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// RX pino 10, TX pino 11
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
float distanciaEsq = 0;
float distanciaDir = 0;

volatile unsigned long pulsosEsq = 0;
volatile unsigned long pulsosDir = 0;

// --- FILA RAM ---
#define MAX_FILA_RAM 30
#define MAX_COMANDO_LEN 20
char filaRAM[MAX_FILA_RAM][MAX_COMANDO_LEN];
byte totalFilaRAM = 0;

Servo meuServo;
byte posicaoServo = 0;
bool servoScanning = false;
unsigned long lastServoMove = 0;
byte servoScanAngle = 0;
bool servoScanDirection = true;

// --- FUNÇÃO AUXILIAR DE RESPOSTA ---
void responder(String msg) {
  Serial.println(msg); 
  meuBT.println(msg);  
}

// --- ENCODERS ---
void contarEsq() { pulsosEsq++; }
void contarDir() { pulsosDir++; }

void posicao() {
    noInterrupts();
    unsigned long pe = pulsosEsq;
    unsigned long pd = pulsosDir;
    pulsosEsq = 0; pulsosDir = 0;
    interrupts();
    float perimetro = diametroRoda_cm * pi;
    distanciaEsq = (float)(pe / (float)furosDoDisco) * perimetro;
    distanciaDir = (float)(pd / (float)furosDoDisco) * perimetro;
}

void parar() {
  digitalWrite(MOTOR_ESQ_FRENTE, LOW); digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, LOW); digitalWrite(MOTOR_DIR_TRAS, LOW);
}

// --- MOVIMENTO COM TIMEOUT (SEGURANÇA) ---
bool wait_movement(float distMetaEsq, float distMetaDir, unsigned long timeoutMs) {
    unsigned long inicio = millis();
    float acEsq = 0, acDir = 0;
    posicao(); // Zera contadores locais
    
    while(true) {
        // 1. Verifica Tempo
        if (millis() - inicio > timeoutMs) {
            parar();
            responder("ERR_TIMEOUT");
            return false; // Falha
        }
        
        // 2. Verifica Conclusão
        bool esqOk = (distMetaEsq <= 0) || (acEsq >= distMetaEsq);
        bool dirOk = (distMetaDir <= 0) || (acDir >= distMetaDir);
        
        if (esqOk && dirOk) break; // Sucesso
        
        // 3. Motores
        if (!esqOk) {
             digitalWrite(MOTOR_ESQ_FRENTE, HIGH); digitalWrite(MOTOR_ESQ_TRAS, LOW);
        } else {
             digitalWrite(MOTOR_ESQ_FRENTE, LOW); digitalWrite(MOTOR_ESQ_TRAS, LOW);
        }
        
        if (!dirOk) {
             digitalWrite(MOTOR_DIR_FRENTE, HIGH); digitalWrite(MOTOR_DIR_TRAS, LOW);
        } else {
             digitalWrite(MOTOR_DIR_FRENTE, LOW); digitalWrite(MOTOR_DIR_TRAS, LOW);
        }

        // 4. Atualiza Posição
        posicao();
        acEsq += distanciaEsq;
        acDir += distanciaDir;
        delay(10);
    }
    parar();
    return true;
}

// --- IMPLEMENTAÇÃO LÓGICA DE MOVIMENTO ---
// Nota: Usamos wait_movement para gerenciar o loop e o tempo
void andarFrente(int distancia) {
  if (distancia <= 0) return;
  if (distancia > 200) distancia = 200;
  // Timeout dinâmico: Mínimo 3s + 100ms por cm. Ex: 50cm = 8s
  unsigned long t = 3000 + (distancia * 100); 
  wait_movement(distancia, distancia, t);
}

void andarTras(int distancia) {
  // Simplificado para trás (sem timeout complexo por enquanto, ou copie a logica acima invertendo pinos)
  if (distancia <= 0) return;
  float da = 0; posicao();
  while(da < distancia){
    digitalWrite(MOTOR_ESQ_FRENTE, LOW); digitalWrite(MOTOR_ESQ_TRAS, HIGH);
    digitalWrite(MOTOR_DIR_FRENTE, LOW); digitalWrite(MOTOR_DIR_TRAS, HIGH);
    posicao(); da += distanciaEsq;
    delay(10);
  }
  parar();
}

void curvaEsquerda(int graus) {
  if (graus <= 0) return;
  // Calculo arco: graus * largura / conversao
  float distAlvo = graus * (20.5 * 0.0174533); 
  
  // Loop manual aqui para garantir pinagem de giro (Esq Para, Dir Frente)
  unsigned long inicio = millis();
  float acDir = 0;
  posicao();
  
  while(acDir < distAlvo){
    if (millis() - inicio > 5000) { parar(); responder("ERR_TIMEOUT_G"); return; }
    
    digitalWrite(MOTOR_ESQ_FRENTE, LOW); digitalWrite(MOTOR_ESQ_TRAS, LOW);
    digitalWrite(MOTOR_DIR_FRENTE, HIGH); digitalWrite(MOTOR_DIR_TRAS, LOW);
    posicao(); 
    acDir += distanciaDir;
    delay(10);
  }
  parar();
}

void curvaDireita(int graus) {
  if (graus <= 0) return;
  float distAlvo = graus * (20.5 * 0.0174533); 
  
  unsigned long inicio = millis();
  float acEsq = 0;
  posicao();
  
  while(acEsq < distAlvo){
    if (millis() - inicio > 5000) { parar(); responder("ERR_TIMEOUT_G"); return; }
    
    digitalWrite(MOTOR_ESQ_FRENTE, HIGH); digitalWrite(MOTOR_ESQ_TRAS, LOW);
    digitalWrite(MOTOR_DIR_FRENTE, LOW); digitalWrite(MOTOR_DIR_TRAS, LOW);
    posicao(); 
    acEsq += distanciaEsq;
    delay(10);
  }
  parar();
}

void moverServo(int angulo) {
  if (angulo < 0) angulo = 0; if (angulo > 180) angulo = 180;
  meuServo.write(angulo); posicaoServo = angulo;
  delay(500); // Espera servo chegar
}

// --- GERENCIAMENTO FILA ---
void limparFilaRAM() {
  totalFilaRAM = 0;
  responder("OK_CLR"); 
}

void adicionarNaFilaRAM(char* cmd) {
  if (totalFilaRAM >= MAX_FILA_RAM) { responder("ERR_FULL"); return; }
  if (strlen(cmd) < 3) { responder("ERR_FMT"); return; }
  strncpy(filaRAM[totalFilaRAM], cmd, MAX_COMANDO_LEN - 1);
  filaRAM[totalFilaRAM][MAX_COMANDO_LEN - 1] = '\0';
  totalFilaRAM++;
  responder("OK_ADD"); 
}

void processarComando(const char* comando, bool fromRAM);

void executarFilaRAM() {
  if (totalFilaRAM == 0) { responder("ERR_EMPTY"); return; }
  
  responder("OK_RUN"); // Confirma inicio
  
  for (byte i = 0; i < totalFilaRAM; i++) {
    char cmdTemp[MAX_COMANDO_LEN];
    strcpy(cmdTemp, filaRAM[i]);
    
    processarComando(cmdTemp, true); 
    
    // --- NOVO: AVISA QUE TERMINOU ESTE PASSO ---
    String stepMsg = "STEP_DONE ";
    stepMsg += i;
    responder(stepMsg);
    // -------------------------------------------
    
    delay(200); // Pausa para estabilizar
  }
  responder("FINISH");
  totalFilaRAM = 0; 
}

void filtrarComandoDabble(char* str) {
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
  strncpy(cmdLimpo, comando, MAX_COMANDO_LEN - 1);
  cmdLimpo[MAX_COMANDO_LEN - 1] = '\0';
  filtrarComandoDabble(cmdLimpo);
  if (strlen(cmdLimpo) == 0) return;

  if (strcmp(cmdLimpo, "LIMPARFILA") == 0) { limparFilaRAM(); return; }
  else if (strcmp(cmdLimpo, "EXECUTAR") == 0) { executarFilaRAM(); return; }
  else if (strncmp(cmdLimpo, "ADD(", 4) == 0) {
     char sub[MAX_COMANDO_LEN]; byte i=4, j=0;
     while (cmdLimpo[i] != ')' && cmdLimpo[i] != '\0') sub[j++] = cmdLimpo[i++];
     sub[j] = '\0'; adicionarNaFilaRAM(sub); return;
  }

  if (cmdLimpo[0] == 'F' && cmdLimpo[1] == '(') andarFrente(atoi(cmdLimpo + 2));
  else if (cmdLimpo[0] == 'T' && cmdLimpo[1] == '(') andarTras(atoi(cmdLimpo + 2));
  else if (cmdLimpo[0] == 'G' && cmdLimpo[1] == '(') {
      int ang = atoi(cmdLimpo + 2); if (ang >= 0) curvaDireita(ang); else curvaEsquerda(-ang);
  }
  else if (cmdLimpo[0] == 'E' && cmdLimpo[1] == '(') curvaEsquerda(atoi(cmdLimpo + 2));
  else if (cmdLimpo[0] == 'D' && cmdLimpo[1] == '(') curvaDireita(atoi(cmdLimpo + 2));
  else if (cmdLimpo[0] == 'S' && cmdLimpo[1] == '(') moverServo(atoi(cmdLimpo + 2));
  else if (strcmp(cmdLimpo, "PARAR") == 0) parar();
}

void setup() {
  Serial.begin(9600);
  meuBT.begin(9600);
  pinMode(MOTOR_ESQ_FRENTE, OUTPUT); pinMode(MOTOR_ESQ_TRAS, OUTPUT);
  pinMode(MOTOR_DIR_FRENTE, OUTPUT); pinMode(MOTOR_DIR_TRAS, OUTPUT);
  pinMode(pinoEncoderEsq, INPUT_PULLUP); pinMode(pinoEncoderDir, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinoEncoderEsq), contarEsq, FALLING);
  attachInterrupt(digitalPinToInterrupt(pinoEncoderDir), contarDir, FALLING);
  meuServo.attach(SERVO_PIN); meuServo.write(0);
  responder("EGG0-1 SAFE READY");
}

void loop() {
  if (meuBT.available()) {
    char buffer[MAX_COMANDO_LEN]; byte index = 0;
    unsigned long inicio = millis(); bool fim = false;
    while (meuBT.available() && index < MAX_COMANDO_LEN - 1 && !fim) {
      char c = meuBT.read();
      if (c == '\\') fim = true; else buffer[index++] = c;
      if (millis() - inicio > 100) break; 
    }
    buffer[index] = '\0';
    if (index > 0) processarComando(buffer, false);
  }
  // USB fallback
  if (Serial.available()) {
     char buffer[MAX_COMANDO_LEN]; byte index = 0;
     while (Serial.available() && index < MAX_COMANDO_LEN - 1) {
       char c = Serial.read();
       if (c == '\n' || c == '\r' || c == '\\') break;
       buffer[index++] = c;
       delay(2);
     }
     buffer[index] = '\0';
     if (index > 0) processarComando(buffer, false);
  }
}