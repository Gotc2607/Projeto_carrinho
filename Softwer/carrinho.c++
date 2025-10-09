// =================================================================
// == CÓDIGO EMBARCADO PARA O CARRINHO DE MOVIMENTO PRECISO       ==
// =================================================================
#include <Encoder.h>

// --- CONFIGURAÇÕES DE HARDWARE (AJUSTE CONFORME SEU CARRINHO) ---
// Pinos do Driver de Motor (exemplo para L298N)
#define MOTOR_A_IN1 8
#define MOTOR_A_IN2 9
#define MOTOR_B_IN3 10
#define MOTOR_B_IN4 11

// Pinos dos Encoders (devem ser pinos de interrupção)
#define ENCODER_A_PIN_A 2
#define ENCODER_A_PIN_B 4
#define ENCODER_B_PIN_A 3
#define ENCODER_B_PIN_B 5

Encoder encoderA(ENCODER_A_PIN_A, ENCODER_A_PIN_B);
Encoder encoderB(ENCODER_B_PIN_A, ENCODER_B_PIN_B);

// --- PARÂMETROS FÍSICOS (MEÇA E AJUSTE COM PRECISÃO) ---
const float WHEEL_DIAMETER = 6.5; // em cm
const float WHEEL_CIRCUMFERENCE = WHEEL_DIAMETER * PI;
const float WHEEL_BASE = 15.0; // Distância entre o centro das duas rodas (em cm)
const float COUNTS_PER_REVOLUTION = 1320; // Pulsos do encoder por volta completa da roda (ajuste isso!)

// --- VARIÁVEIS DE ESTADO E ODometria ---
// Posição e orientação do carrinho
double xPos = 0.0;
double yPos = 0.0;
double theta = 0.0; // Ângulo em radianos

// Posições anteriores dos encoders para cálculo do delta
long prevCountA = 0;
long prevCountB = 0;

// Buffer para comandos seriais
String serialCommandBuffer = "";
bool newCommandReceived = false;

void setup() {
  Serial.begin(115200); // Usar uma velocidade alta para a telemetria
  
  // Configurar pinos dos motores como saída
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN3, OUTPUT);
  pinMode(MOTOR_B_IN4, OUTPUT);

  // Zera as posições dos encoders
  encoderA.write(0);
  encoderB.write(0);

  serialCommandBuffer.reserve(100); // Aloca memória para o buffer
  Serial.println("Carrinho pronto. Aguardando comandos.");
}

void loop() {
  // 1. Verificar se há novos comandos chegando pela serial
  checkSerial();

  // 2. Se um novo comando foi recebido, processá-lo
  if (newCommandReceived) {
    processCommand(serialCommandBuffer);
    serialCommandBuffer = "";
    newCommandReceived = false;
  }

  // 3. Atualizar a odometria constantemente
  updateOdometry();

  // 4. Enviar telemetria para a GUI (ex: 10 vezes por segundo)
  static unsigned long lastTelemetryTime = 0;
  if (millis() - lastTelemetryTime > 100) {
    sendTelemetry();
    lastTelemetryTime = millis();
  }

  // A lógica de controle de movimento (PID) seria chamada aqui
  // quando um movimento está ativo. Por simplicidade, este esqueleto
  // apenas calcula a posição.
}

// --- FUNÇÕES PRINCIPAIS ---

void updateOdometry() {
  long currentCountA = encoderA.read();
  long currentCountB = encoderB.read();

  long deltaA = currentCountA - prevCountA;
  long deltaB = currentCountB - prevCountB;

  prevCountA = currentCountA;
  prevCountB = currentCountB;

  // Calcula a distância percorrida por cada roda
  float distA = (deltaA / COUNTS_PER_REVOLUTION) * WHEEL_CIRCUMFERENCE;
  float distB = (deltaB / COUNTS_PER_REVOLUTION) * WHEEL_CIRCUMFERENCE;

  // Calcula a distância média e a mudança de ângulo
  float distCenter = (distA + distB) / 2.0;
  float deltaTheta = (distA - distB) / WHEEL_BASE;

  // Atualiza a posição (X, Y) e orientação (theta)
  xPos += distCenter * cos(theta + deltaTheta / 2.0);
  yPos += distCenter * sin(theta + deltaTheta / 2.0);
  theta += deltaTheta;

  // Mantém theta entre -PI e PI para evitar que o valor cresça indefinidamente
  if (theta > PI) theta -= 2 * PI;
  if (theta < -PI) theta += 2 * PI;
}

void sendTelemetry() {
  // Formato: "T;X;Y;Theta\n" (T de Telemetria)
  // Theta é enviado em graus para facilitar na GUI
  Serial.print("T;");
  Serial.print(xPos, 2);
  Serial.print(";");
  Serial.print(yPos, 2);
  Serial.print(";");
  Serial.print(degrees(theta), 2);
  Serial.println();
}

void processCommand(String cmd) {
  cmd.trim();
  Serial.print("Comando recebido: ");
  Serial.println(cmd);

  // Aqui você implementaria a lógica para cada comando
  // Ex: "FRENTE 20", "ESQUERDA 90"
  // Esta lógica ativaria o controle dos motores (idealmente com PID)
  // para atingir o alvo de distância ou ângulo.
  
  // Exemplo simples de como você começaria:
  if (cmd.startsWith("FRENTE")) {
    float distancia = cmd.substring(7).toFloat();
    // Chamar função de movimento: moveForwardPID(distancia);
    Serial.print("Movendo para frente ");
    Serial.println(distancia);
    
  } else if (cmd.startsWith("ESQUERDA")) {
    float angulo = cmd.substring(9).toFloat();
    // Chamar função de giro: turnPID(-angulo);
    Serial.print("Girando para a esquerda ");
    Serial.println(angulo);
  }
  // Adicionar outros comandos (DIREITA, TRAS, ENTREGAR...)
}


void checkSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      newCommandReceived = true;
    } else {
      serialCommandBuffer += c;
    }
  }
}

// --- Funções de Controle de Motor (exemplo básico) ---
void setMotorSpeed(int motorA_speed, int motorB_speed) {
  // Lógica para acionar os motores. 
  // Velocidades positivas = para frente, negativas = para trás.
  // Esta parte é MUITO dependente do seu driver.
  // Você precisará mapear a velocidade (-255 a 255) para os comandos do seu driver.
}