#include <Arduino.h>
#include <EEPROM.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// Definição da comunicação serial Bluetooth (RX no pino 10, TX no pino 11)
SoftwareSerial meuBT(10, 11);

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
int TEMPO_BASE_CM = 50;   // tempo em ms por cm (ajustável)
int TEMPO_BASE_GRAUS = 15; // tempo em ms por grau (ajustável)

// ---------------------------
// SERVO
// ---------------------------
Servo meuServo;
int posicaoServo = 0;
bool servoScanning = false;
unsigned long lastServoMove = 0;
int servoScanAngle = 0;
bool servoScanDirection = true;

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
  Serial.println("Parado.");
}

void andarFrente(int distancia) {
  if (distancia <= 0) {
    Serial.println("Distância inválida para FRENTE");
    return;
  }
  if (distancia > 1000) distancia = 1000; // Limite de segurança
  
  digitalWrite(MOTOR_ESQ_FRENTE, HIGH);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, HIGH);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
  Serial.print("Andando FRENTE ");
  Serial.print(distancia);
  Serial.println(" cm");
  delay(distancia * TEMPO_BASE_CM);
  parar();
}

void curvaEsquerda(int graus) {
  if (graus <= 0) {
    Serial.println("Ângulo inválido para ESQUERDA");
    return;
  }
  if (graus > 360) graus = 360; // Limite de segurança
  
  digitalWrite(MOTOR_ESQ_FRENTE, LOW);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, HIGH);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
  Serial.print("Virando ESQUERDA ");
  Serial.print(graus);
  Serial.println(" graus");
  delay(graus * TEMPO_BASE_GRAUS);
  parar();
}

void curvaDireita(int graus) {
  if (graus <= 0) {
    Serial.println("Ângulo inválido para DIREITA");
    return;
  }
  if (graus > 360) graus = 360; // Limite de segurança
  
  digitalWrite(MOTOR_ESQ_FRENTE, HIGH);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, LOW);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
  Serial.print("Virando DIREITA ");
  Serial.print(graus);
  Serial.println(" graus");
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

void iniciarServoScan() {
  servoScanning = true;
  servoScanAngle = 0;
  servoScanDirection = true;
  Serial.println("Iniciando varredura do servo...");
}

void atualizarServoScan() {
  if (!servoScanning) return;
  
  if (millis() - lastServoMove >= 15) {
    if (servoScanDirection) {
      servoScanAngle++;
      if (servoScanAngle >= 90) {
        servoScanDirection = false;
      }
    } else {
      servoScanAngle--;
      if (servoScanAngle <= 0) {
        servoScanDirection = true;
        servoScanning = false;
        Serial.println("Varredura concluída.");
      }
    }
    
    meuServo.write(servoScanAngle);
    lastServoMove = millis();
  }
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

  cmd.trim();
  if (cmd.length() == 0) {
    Serial.println("Comando vazio não salvo!");
    return;
  }

  int addr = EEPROM_END_COMANDOS + totalComandos * MAX_COMANDO_LEN;
  for (int i = 0; i < MAX_COMANDO_LEN; i++) {
    if (i < cmd.length()) EEPROM.write(addr + i, cmd[i]);
    else EEPROM.write(addr + i, 0);
  }

  totalComandos++;
  EEPROM.write(EEPROM_END_COMANDOS + MAX_COMANDOS * MAX_COMANDO_LEN, totalComandos);

  Serial.print("Comando salvo (");
  Serial.print(totalComandos);
  Serial.print("/");
  Serial.print(MAX_COMANDOS);
  Serial.print("): ");
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
  if (totalComandos == 0) {
    Serial.println("Nenhum comando salvo!");
    return;
  }
  
  Serial.println("Executando comandos salvos...");
  for (int i = 0; i < totalComandos; i++) {
    String cmd = lerComandoDaEEPROM(i);
    Serial.print("Executando [");
    Serial.print(i + 1);
    Serial.print("/");
    Serial.print(totalComandos);
    Serial.print("]: ");
    Serial.println(cmd);
    delay(500);
    
    // Executa a string salva
    if (cmd.length() > 0) {
      cmd.trim();
      cmd.toUpperCase();
      
      if (cmd.startsWith("FRENTE ")) andarFrente(cmd.substring(7).toInt());
      else if (cmd.startsWith("ESQUERDA ")) curvaEsquerda(cmd.substring(9).toInt());
      else if (cmd.startsWith("DIREITA ")) curvaDireita(cmd.substring(8).toInt());
      else if (cmd.startsWith("SERVO ")) moverServo(cmd.substring(6).toInt());
      else if (cmd == "SERVO_SCAN") iniciarServoScan();
      else if (cmd == "PARAR") parar();
    }
    delay(300); // Pequena pausa entre comandos
  }
  Serial.println("Execução concluída.");
}

void limparEEPROM() {
  // Limpa apenas o espaço usado pelos comandos
  for (int i = 0; i < MAX_COMANDOS; i++) {
    int addr = EEPROM_END_COMANDOS + i * MAX_COMANDO_LEN;
    EEPROM.write(addr, 0); // Apenas primeiro byte para marcar como vazio
  }
  totalComandos = 0;
  // Atualiza o contador na EEPROM para 0
  EEPROM.write(EEPROM_END_COMANDOS + MAX_COMANDOS * MAX_COMANDO_LEN, totalComandos);
  Serial.println("EEPROM limpa!");
}

void listarComandosSalvos() {
  if (totalComandos == 0) {
    Serial.println("Nenhum comando salvo.");
    return;
  }
  
  Serial.println("Comandos salvos:");
  for (int i = 0; i < totalComandos; i++) {
    String cmd = lerComandoDaEEPROM(i);
    Serial.print("[");
    Serial.print(i + 1);
    Serial.print("]: ");
    Serial.println(cmd);
  }
}

// ---------------------------
// PROCESSAMENTO DE COMANDOS (ROBUSTO)
// ---------------------------
void processarComando(String comando) {
  
  // Remove o caractere de Retorno de Carro ("\r") se existir
  if (comando.endsWith("\r")) {
    comando.remove(comando.length() - 1);
  }
  
  // Remove espaços em branco no início e fim
  comando.trim();
  
  // Se o comando for vazio após o trim/remove, ignore
  if (comando.length() == 0) {
    return;
  }
  
  // Converte para maiúsculas para fácil comparação
  comando.toUpperCase(); 

  Serial.print("Recebido: ");
  Serial.println(comando);

  // Processamento robusto com verificação de parâmetros
  if (comando.startsWith("FRENTE ")) {
    andarFrente(comando.substring(7).toInt());
  }
  else if (comando.startsWith("ESQUERDA ")) {
    curvaEsquerda(comando.substring(9).toInt());
  }
  else if (comando.startsWith("DIREITA ")) {
    curvaDireita(comando.substring(8).toInt());
  }
  else if (comando.startsWith("SERVO ")) {
    moverServo(comando.substring(6).toInt());
  }
  else if (comando == "SERVO_SCAN") {
    iniciarServoScan();
  }
  else if (comando.startsWith("CALIBRAR FRENTE ")) {
    calibrar("FRENTE", comando.substring(16).toInt());
  }
  else if (comando.startsWith("CALIBRAR GIRO ")) {
    calibrar("GIRO", comando.substring(14).toInt());
  }
  else if (comando.startsWith("SALVAR ")) {
    String cmdParaSalvar = comando.substring(7);
    if (cmdParaSalvar.length() > 0) {
      salvarComandoNaEEPROM(cmdParaSalvar);
    } else {
      Serial.println("Erro: Comando vazio após SALVAR");
    }
  }
  else if (comando == "RODAR_SALVOS") {
    executarComandosSalvos();
  }
  else if (comando == "LISTAR_SALVOS") {
    listarComandosSalvos();
  }
  else if (comando == "LIMPAR_MEMORIA") {
    limparEEPROM();
  }
  else if (comando == "PARAR") {
    parar();
  }
  else if (comando == "AJUDA" || comando == "HELP") {
    Serial.println("Comandos disponíveis:");
    Serial.println("FRENTE [distancia], ESQUERDA [graus], DIREITA [graus]");
    Serial.println("SERVO [angulo], SERVO_SCAN, PARAR");
    Serial.println("CALIBRAR FRENTE [valor], CALIBRAR GIRO [valor]");
    Serial.println("SALVAR [comando], RODAR_SALVOS, LISTAR_SALVOS");
    Serial.println("LIMPAR_MEMORIA, AJUDA");
  }
  else {
    Serial.println("Comando desconhecido. Digite AJUDA para ver opções.");
  }
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

  // Recupera quantidade de comandos salvos (APENAS LEITURA)
  totalComandos = EEPROM.read(EEPROM_END_COMANDOS + MAX_COMANDOS * MAX_COMANDO_LEN);
  // Sanity check
  if (totalComandos > MAX_COMANDOS) totalComandos = 0;

  Serial.println("=== CARRINHO ROBÓTICO PRONTO ===");
  Serial.println("Aguardando comandos Bluetooth...");
  meuBT.println("Carrinho pronto! Envie comandos.");
  Serial.print("Comandos salvos na memória: ");
  Serial.println(totalComandos);
  Serial.println("Digite 'AJUDA' para ver comandos disponíveis");
}

// ---------------------------
// LOOP PRINCIPAL (FINAL)
// ---------------------------
void loop() {
  // Verifica comandos Bluetooth
  if (meuBT.available()) {
    String comando = meuBT.readStringUntil('\n');
    processarComando(comando);
  }
  
  // Atualiza varredura do servo (não-bloqueante)
  atualizarServoScan();
}