#include <Arduino.h>
#include <EEPROM.h>
#include <Servo.h>
#include <SoftwareSerial.h>

SoftwareSerial meuBT(10, 11);

// --- PINOS ---
#define MOTOR_ESQ_FRENTE 5
#define MOTOR_ESQ_TRAS   4
#define MOTOR_DIR_FRENTE 3
#define MOTOR_DIR_TRAS   2
#define SERVO_PIN        6

// --- CALIBRAÇÃO ---
int TEMPO_BASE_CM = 50;
int TEMPO_BASE_GRAUS = 15;

Servo meuServo;
int posicaoServo = 0;

// --- MEMÓRIA ---
#define EEPROM_END_COMANDOS 0
#define MAX_COMANDOS 20
#define MAX_COMANDO_LEN 30
int totalComandos = 0;

// ============================================================
// MOVIMENTAÇÃO
// ============================================================
void parar() {
  digitalWrite(MOTOR_ESQ_FRENTE, LOW);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, LOW);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
}

void andarFrente(int distancia) {
  digitalWrite(MOTOR_ESQ_FRENTE, HIGH);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, HIGH);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
  delay(distancia * TEMPO_BASE_CM);
  parar();
}

// [NOVO] Função para andar para trás (O Python pede "TRAS")
void andarTras(int distancia) {
  digitalWrite(MOTOR_ESQ_FRENTE, LOW);
  digitalWrite(MOTOR_ESQ_TRAS, HIGH);
  digitalWrite(MOTOR_DIR_FRENTE, LOW);
  digitalWrite(MOTOR_DIR_TRAS, HIGH);
  delay(distancia * TEMPO_BASE_CM);
  parar();
}

void curvaEsquerda(int graus) {
  digitalWrite(MOTOR_ESQ_FRENTE, LOW);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, HIGH);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
  delay(graus * TEMPO_BASE_GRAUS);
  parar();
}

void curvaDireita(int graus) {
  digitalWrite(MOTOR_ESQ_FRENTE, HIGH);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, LOW);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
  delay(graus * TEMPO_BASE_GRAUS);
  parar();
}

// ============================================================
// SERVO E ENTREGA
// ============================================================
void moverServo(int angulo) {
  if (angulo < 0) angulo = 0;
  if (angulo > 180) angulo = 180;
  meuServo.write(angulo);
  posicaoServo = angulo;
}

// [NOVO] Função Entregar (simula derrubar algo com o servo)
void entregar() {
  Serial.println("Iniciando entrega...");
  meuServo.write(90);  // Levanta/Gira
  delay(1000);
  meuServo.write(0);   // Volta
  delay(500);
  Serial.println("Entrega concluida.");
}

// ============================================================
// PROCESSAMENTO DE COMANDOS
// ============================================================
void processarComando(String comando) {
  comando.trim();
  comando.toUpperCase();
  
  // Envia de volta para o Python saber que recebeu (Debug)
  Serial.print("Recebido: ");
  Serial.println(comando);

  if (comando.startsWith("FRENTE")) andarFrente(comando.substring(7).toInt());
  else if (comando.startsWith("TRAS")) andarTras(comando.substring(5).toInt()); // [NOVO]
  else if (comando.startsWith("ESQUERDA")) curvaEsquerda(comando.substring(9).toInt());
  else if (comando.startsWith("DIREITA")) curvaDireita(comando.substring(8).toInt());
  else if (comando.startsWith("ENTREGAR")) entregar(); // [NOVO]
  else if (comando.startsWith("SERVO ")) moverServo(comando.substring(6).toInt());
  else if (comando == "PARAR") parar();
  else Serial.println("Comando desconhecido.");
}

void setup() {
  // [IMPORTANTE] Mudei para 115200 para igualar ao Python
  Serial.begin(115200); 
  meuBT.begin(9600);

  pinMode(MOTOR_ESQ_FRENTE, OUTPUT);
  pinMode(MOTOR_ESQ_TRAS, OUTPUT);
  pinMode(MOTOR_DIR_FRENTE, OUTPUT);
  pinMode(MOTOR_DIR_TRAS, OUTPUT);

  meuServo.attach(SERVO_PIN);
  meuServo.write(0);

  Serial.println("Carrinho Pronto! Conectado via USB.");
}

void loop() {
  // [IMPORTANTE] Agora lemos da USB (Serial) também, não só do Bluetooth
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    processarComando(comando);
  }

  if (meuBT.available()) {
    String comando = meuBT.readStringUntil('\n');
    processarComando(comando);
  }
}