// =================================================================
// === CARRINHO BLUETOOTH COM TEMPO E MEMÓRIA DE PERCURSO (SEM ENCODER)
// =================================================================
#include <Arduino.h>
#include <EEPROM.h>

// --- Pinos do driver L298N ---
#define MOTOR_A_IN1 8
#define MOTOR_A_IN2 9
#define MOTOR_B_IN3 10
#define MOTOR_B_IN4 11
#define MOTOR_A_ENA 5
#define MOTOR_B_ENB 6

// --- Configurações ---
#define MAX_MOVIMENTOS 100
#define EEPROM_START 0
int velocidadePadrao = 150;

// --- Estrutura de movimento salvo ---
struct Movimento {
  char acao;       // 'F', 'T', 'E', 'D', 'P'
  int velocidade;  // 0–255
  unsigned long tempo; // em milissegundos
};

// --- Variáveis globais ---
Movimento memoria[MAX_MOVIMENTOS];
int qtdMovimentos = 0;
String comando = "";

// --- Funções de controle de motor ---
void frente(int vel) {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_B_IN3, HIGH);
  digitalWrite(MOTOR_B_IN4, LOW);
  analogWrite(MOTOR_A_ENA, vel);
  analogWrite(MOTOR_B_ENB, vel);
}

void tras(int vel) {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
  digitalWrite(MOTOR_B_IN3, LOW);
  digitalWrite(MOTOR_B_IN4, HIGH);
  analogWrite(MOTOR_A_ENA, vel);
  analogWrite(MOTOR_B_ENB, vel);
}

void esquerda(int vel) {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
  digitalWrite(MOTOR_B_IN3, HIGH);
  digitalWrite(MOTOR_B_IN4, LOW);
  analogWrite(MOTOR_A_ENA, vel);
  analogWrite(MOTOR_B_ENB, vel);
}

void direita(int vel) {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_B_IN3, LOW);
  digitalWrite(MOTOR_B_IN4, HIGH);
  analogWrite(MOTOR_A_ENA, vel);
  analogWrite(MOTOR_B_ENB, vel);
}

void parar() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_B_IN3, LOW);
  digitalWrite(MOTOR_B_IN4, LOW);
}

// --- Grava movimento na memória e EEPROM ---
void salvarMovimento(char acao, int vel, unsigned long tempo) {
  if (qtdMovimentos >= MAX_MOVIMENTOS) {
    Serial.println("Memória cheia!");
    return;
  }

  Movimento m = {acao, vel, tempo};
  memoria[qtdMovimentos] = m;
  int addr = EEPROM_START + qtdMovimentos * sizeof(Movimento);
  EEPROM.put(addr, m);
  qtdMovimentos++;
  Serial.print("Movimento salvo: ");
  Serial.print(acao);
  Serial.print(" por ");
  Serial.print(tempo);
  Serial.println(" ms");
}

// --- Carrega da EEPROM ---
void carregarMemoria() {
  qtdMovimentos = 0;
  for (int i = 0; i < MAX_MOVIMENTOS; i++) {
    Movimento m;
    int addr = EEPROM_START + i * sizeof(Movimento);
    EEPROM.get(addr, m);
    if (m.acao == 0xFF || m.acao == 0) break;
    memoria[i] = m;
    qtdMovimentos++;
  }
  Serial.print("Comandos carregados: ");
  Serial.println(qtdMovimentos);
}

// --- Apaga EEPROM ---
void limparEEPROM() {
  for (int i = 0; i < MAX_MOVIMENTOS * sizeof(Movimento); i++) {
    EEPROM.write(EEPROM_START + i, 0xFF);
  }
  qtdMovimentos = 0;
  Serial.println("Memória apagada!");
}

// --- Reexecuta percurso ---
void replay() {
  Serial.println("Reproduzindo percurso...");
  for (int i = 0; i < qtdMovimentos; i++) {
    Movimento m = memoria[i];
    Serial.print("Passo ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(m.acao);
    Serial.print(" por ");
    Serial.print(m.tempo);
    Serial.println(" ms");

    switch (m.acao) {
      case 'F': frente(m.velocidade); break;
      case 'T': tras(m.velocidade); break;
      case 'E': esquerda(m.velocidade); break;
      case 'D': direita(m.velocidade); break;
      case 'P': parar(); break;
    }
    delay(m.tempo);
    parar();
    delay(300); // pequena pausa entre movimentos
  }
  Serial.println("Percurso finalizado!");
}

// --- Processa comandos recebidos ---
void processarComando(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  Serial.print("Recebido: ");
  Serial.println(cmd);

  // divide o comando: exemplo -> "FRENTE 2000"
  int espaco = cmd.indexOf(' ');
  String acaoStr = cmd;
  unsigned long tempo = 1000; // padrão
  if (espaco > 0) {
    acaoStr = cmd.substring(0, espaco);
    tempo = cmd.substring(espaco + 1).toInt();
    if (tempo <= 0) tempo = 1000;
  }

  char acao = 0;
  if (acaoStr == "FRENTE") acao = 'F';
  else if (acaoStr == "TRAS") acao = 'T';
  else if (acaoStr == "ESQUERDA") acao = 'E';
  else if (acaoStr == "DIREITA") acao = 'D';
  else if (acaoStr == "PARAR") acao = 'P';
  else if (acaoStr == "REPLAY") { replay(); return; }
  else if (acaoStr == "LIMPAR") { limparEEPROM(); return; }
  else if (acaoStr.startsWith("VEL")) {
    int novaVel = cmd.substring(3).toInt();
    if (novaVel >= 0 && novaVel <= 255) {
      velocidadePadrao = novaVel;
      Serial.print("Velocidade ajustada para ");
      Serial.println(velocidadePadrao);
    }
    return;
  } else {
    Serial.println("Comando inválido.");
    return;
  }

  // Executa e grava o movimento
  switch (acao) {
    case 'F': frente(velocidadePadrao); break;
    case 'T': tras(velocidadePadrao); break;
    case 'E': esquerda(velocidadePadrao); break;
    case 'D': direita(velocidadePadrao); break;
    case 'P': parar(); break;
  }
  delay(tempo);
  parar();
  salvarMovimento(acao, velocidadePadrao, tempo);
}

// --- Setup ---
void setup() {
  Serial.begin(9600);
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN3, OUTPUT);
  pinMode(MOTOR_B_IN4, OUTPUT);
  pinMode(MOTOR_A_ENA, OUTPUT);
  pinMode(MOTOR_B_ENB, OUTPUT);
  parar();

  carregarMemoria();
  Serial.println("Carrinho pronto. Envie comandos via Bluetooth:");
  Serial.println("Exemplos:");
  Serial.println("  FRENTE 1800");
  Serial.println("  ESQUERDA 700");
  Serial.println("  DIREITA 600");
  Serial.println("  PARAR");
  Serial.println("  REPLAY");
  Serial.println("  LIMPAR");
}

// --- Loop principal ---
void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (comando.length() > 0) {
        processarComando(comando);
        comando = "";
      }
    } else {
      comando += c;
    }
  }
}
