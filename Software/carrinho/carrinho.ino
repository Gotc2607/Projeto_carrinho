#include <Arduino.h>
#include <EEPROM.h>
#include <Servo.h>
#include <SoftwareSerial.h> //
SoftwareSerial meuBT(10, 11); // (RX, TX) -> (Pino 10 = RX, Pino 11 = TX)

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
bool servoScanning = false;

// ---------------------------
// MEMÓRIA (EEPROM)
// ---------------------------
#define EEPROM_END_COMANDOS 0
#define MAX_COMANDOS 20
#define MAX_COMANDO_LEN 30
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
  servoScanning = false;
  if (angulo < 0) angulo = 0;
  if (angulo > 180) angulo = 180;
  meuServo.write(angulo);
  posicaoServo = angulo;
  Serial.print("Servo movido para: ");
  Serial.println(angulo);
}

void servoScan() {
  servoScanning = true;
  Serial.println("Iniciando varredura do servo...");

  for (int ang = 0; ang <= 90 && servoScanning; ang++) {
    meuServo.write(ang);
    delay(15);
  }
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
// EEPROM - SALVAR E EXECUTAR
// ---------------------------
void salvarComandoNaEEPROM(String cmd) {
  if (totalComandos >= MAX_COMANDOS) {
    Serial.println("Memória cheia!");
    return;
  }

  int addr = EEPROM_END_COMANDOS + totalComandos * MAX_COMANDO_LEN;
  for (int i = 0; i < MAX_COMANDO_LEN; i++) {
    if (i < cmd.length()) EEPROM.write(addr + i, cmd[i]);
    else EEPROM.write(addr + i, 0);
  }

  totalComandos++;
  EEPROM.write(EEPROM_END_COMANDOS + MAX_COMANDOS * MAX_COMANDO_LEN, totalComandos);

  Serial.print("Comando salvo: ");
  Serial.println(cmd);
}

String lerComandoDaEEPROM(int index) {
  if (index < 0 || index >= totalComandos) return "";
  String cmd = "";
  int addr = EEPROM_END_COMANDOS + index * MAX_COMANDO_LEN;
  for (int i = 0; i < MAX_COMANDO_LEN; i++) {
    char c = EEPROM.read(addr + i);
    if (c == 0) break;
    cmd += c;
  }
  return cmd;
}

void executarComandosSalvos() {
  Serial.println("Executando comandos salvos...");
  for (int i = 0; i < totalComandos; i++) {
    String cmd = lerComandoDaEEPROM(i);
    Serial.print("> ");
    Serial.println(cmd);
    delay(500);
    // Executa normalmente
    if (cmd.length() > 0) {
      cmd.trim();
      cmd.toUpperCase();
      if (cmd.startsWith("FRENTE")) andarFrente(cmd.substring(7).toInt());
      else if (cmd.startsWith("ESQUERDA")) curvaEsquerda(cmd.substring(9).toInt());
      else if (cmd.startsWith("DIREITA")) curvaDireita(cmd.substring(8).toInt());
      else if (cmd.startsWith("SERVO ")) moverServo(cmd.substring(6).toInt());
      else if (cmd == "SERVO_SCAN") servoScan();
      else if (cmd == "PARAR") parar();
    }
  }
  Serial.println("Execução concluída.");
}

void limparEEPROM() {
  for (int i = EEPROM_END_COMANDOS; i < EEPROM_END_COMANDOS + MAX_COMANDOS * MAX_COMANDO_LEN + 1; i++) {
    EEPROM.write(i, 0);
  }
  totalComandos = 0;
  Serial.println("EEPROM limpa!");
}

// ---------------------------
// PROCESSAMENTO DE COMANDOS
// ---------------------------
void processarComando(String comando) {
  comando.trim();
  comando.toUpperCase();
  Serial.print("Recebido: ");
  Serial.println(comando);

  if (comando.startsWith("FRENTE")) andarFrente(comando.substring(7).toInt());
  else if (comando.startsWith("ESQUERDA")) curvaEsquerda(comando.substring(9).toInt());
  else if (comando.startsWith("DIREITA")) curvaDireita(comando.substring(8).toInt());
  else if (comando.startsWith("SERVO ")) moverServo(comando.substring(6).toInt());
  else if (comando == "SERVO_SCAN") servoScan();
  else if (comando.startsWith("CALIBRAR FRENTE")) calibrar("FRENTE", comando.substring(15).toInt());
  else if (comando.startsWith("CALIBRAR GIRO")) calibrar("GIRO", comando.substring(13).toInt());
  else if (comando.startsWith("SALVAR ")) salvarComandoNaEEPROM(comando.substring(7));
  else if (comando == "RODAR_SALVOS") executarComandosSalvos();
  else if (comando == "LIMPAR_MEMORIA") limparEEPROM();
  else if (comando == "PARAR") parar();
  else Serial.println("Comando desconhecido.");
}

// ---------------------------
// SETUP
// ---------------------------
void setup() {
  
  Serial.begin(9600);
  meuBT.begin(9600);
  
  pinMode(MOTOR_ESQ_FRENTE, OUTPUT);
  pinMode(MOTOR_ESQ_TRAS, OUTPUT);
  pinMode(MOTOR_DIR_FRENTE, OUTPUT);
  pinMode(MOTOR_DIR_TRAS, OUTPUT);

  meuServo.attach(SERVO_PIN);
  meuServo.write(0);

  // Recupera quantidade de comandos salvos
  totalComandos = EEPROM.read(EEPROM_END_COMANDOS + MAX_COMANDOS * MAX_COMANDO_LEN);

  Serial.println("Carrinho com EEPROM e Servo pronto! Aguardando comandos Bluetooth...");
  meuBT.println("Aguardando comandos Bluetooth...");
  Serial.print("Comandos salvos: ");
  Serial.println(totalComandos);
}

// ---------------------------
// LOOP PRINCIPAL
// ---------------------------
void loop() {
  if (meuBT.available()) {
    String comando = meuBT.readStringUntil('\n');
    processarComando(comando);
  }
}
