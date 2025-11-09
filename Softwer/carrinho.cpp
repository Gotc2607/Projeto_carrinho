#include <Arduino.h>
#include <EEPROM.h>
#include <Servo.h>

// ---------------------------
// CONFIGURAÇÃO DOS PINOS
// ---------------------------
#define MOTOR_ESQ_FRENTE 5
#define MOTOR_ESQ_TRAS   4
#define MOTOR_DIR_FRENTE 3
#define MOTOR_DIR_TRAS   2
#define SERVO_PIN        6

// ---------------------------
// VARIÁVEIS DE CALIBRAÇÃO
// ---------------------------
int TEMPO_BASE_CM = 50;     // tempo em ms por cm (ajustável)
int TEMPO_BASE_GRAUS = 15;  // tempo em ms por grau (ajustável)

// ---------------------------
// SERVO
// ---------------------------
Servo meuServo;
int posicaoServo = 0;
bool servoScanning = false; // controle para varredura automática

// ---------------------------
// MEMÓRIA (EEPROM)
// ---------------------------
#define EEPROM_END_COMANDOS 0
#define MAX_COMANDOS 20
String comandosSalvos[MAX_COMANDOS];
int totalComandos = 0;

// ---------------------------
// FUNÇÕES DE MOVIMENTO
// ---------------------------
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

// ---------------------------
// CONTROLE DO SERVO
// ---------------------------
void moverServo(int angulo) {
  servoScanning = false; // interrompe modo automático, se ativo
  if (angulo < 0) angulo = 0;
  if (angulo > 180) angulo = 180;
  meuServo.write(angulo);
  posicaoServo = angulo;
  Serial.print("Servo movido para: ");
  Serial.println(angulo);
}

void servoScan() {
  servoScanning = true;
  Serial.println("Iniciando varredura automática do servo...");

  for (int ang = 0; ang <= 90 && servoScanning; ang++) {
    meuServo.write(ang);
    delay(15);
  }
  delay(100);
  for (int ang = 90; ang >= 0 && servoScanning; ang--) {
    meuServo.write(ang);
    delay(15);
  }

  servoScanning = false;
  Serial.println("Varredura concluída.");
}

// ---------------------------
// CALIBRAÇÃO
// ---------------------------
void calibrar(String tipo, int valor) {
  if (tipo == "FRENTE") {
    TEMPO_BASE_CM = valor;
    Serial.print("Novo tempo base por cm: ");
    Serial.println(TEMPO_BASE_CM);
  } else if (tipo == "GIRO") {
    TEMPO_BASE_GRAUS = valor;
    Serial.print("Novo tempo base por grau: ");
    Serial.println(TEMPO_BASE_GRAUS);
  } else {
    Serial.println("Tipo de calibração inválido!");
  }
}

// ---------------------------
// PROCESSAMENTO DE COMANDOS
// ---------------------------
void processarComando(String comando) {
  comando.trim();
  comando.toUpperCase(); // ignora diferença de maiúsculas/minúsculas
  Serial.print("Recebido: ");
  Serial.println(comando);

  if (comando.startsWith("FRENTE")) {
    int valor = comando.substring(7).toInt();
    andarFrente(valor);
  } 
  else if (comando.startsWith("ESQUERDA")) {
    int valor = comando.substring(9).toInt();
    curvaEsquerda(valor);
  } 
  else if (comando.startsWith("DIREITA")) {
    int valor = comando.substring(8).toInt();
    curvaDireita(valor);
  }
  else if (comando.startsWith("SERVO ")) {
    int angulo = comando.substring(6).toInt();
    moverServo(angulo);
  }
  else if (comando == "SERVO_SCAN") {
    servoScan();
  }
  else if (comando.startsWith("CALIBRAR FRENTE")) {
    int valor = comando.substring(15).toInt();
    calibrar("FRENTE", valor);
  }
  else if (comando.startsWith("CALIBRAR GIRO")) {
    int valor = comando.substring(13).toInt();
    calibrar("GIRO", valor);
  }
  else if (comando == "PARAR") {
    parar();
    servoScanning = false;
  }
  else {
    Serial.println("Comando desconhecido.");
  }
}

// ---------------------------
// SETUP
// ---------------------------
void setup() {
  Serial.begin(9600);
  
  pinMode(MOTOR_ESQ_FRENTE, OUTPUT);
  pinMode(MOTOR_ESQ_TRAS, OUTPUT);
  pinMode(MOTOR_DIR_FRENTE, OUTPUT);
  pinMode(MOTOR_DIR_TRAS, OUTPUT);

  meuServo.attach(SERVO_PIN);
  meuServo.write(0);

  Serial.println("Carrinho com servo pronto! Aguardando comandos Bluetooth...");
}

// ---------------------------
// LOOP PRINCIPAL
// ---------------------------
void loop() {
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    processarComando(comando);
  }

  // se quiser que o servo continue varrendo continuamente:
  // if (servoScanning) servoScan();
}
