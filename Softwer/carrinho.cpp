#include <Arduino.h>
#include <EEPROM.h>

// ================================
// CONFIGURAÇÃO DE PINOS
// ================================
#define MOTOR_ESQ_FRENTE 5
#define MOTOR_ESQ_TRAS   4
#define MOTOR_DIR_FRENTE 3
#define MOTOR_DIR_TRAS   2

// ================================
// CONSTANTES DE CALIBRAÇÃO
// ================================
const int TEMPO_BASE_CM = 50;     // ms por cm (ajustar conforme o carrinho)
const int TEMPO_BASE_GRAUS = 10;  // ms por grau (ajustar após testes)
const int MAX_COMANDOS = 50;      // quantidade máxima de comandos que pode gravar

// ================================
// ESTRUTURA DE COMANDO
// ================================
struct Movimento {
  char tipo;      // 'F' = Frente, 'E' = Esquerda, 'D' = Direita
  float valor;    // distância (cm) ou ângulo (graus)
};

// ================================
// FUNÇÕES DE MOTORES
// ================================
void parar() {
  digitalWrite(MOTOR_ESQ_FRENTE, LOW);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, LOW);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
}

void frente() {
  digitalWrite(MOTOR_ESQ_FRENTE, HIGH);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, HIGH);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
}

void curvaEsquerda() {
  digitalWrite(MOTOR_ESQ_FRENTE, LOW);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, HIGH);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
}

void curvaDireita() {
  digitalWrite(MOTOR_ESQ_FRENTE, HIGH);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, LOW);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
}

// ================================
// MOVIMENTOS BASEADOS EM TEMPO
// ================================
void moverFrente(float distancia_cm) {
  long tempo = distancia_cm * TEMPO_BASE_CM;
  frente();
  delay(tempo);
  parar();
}

void girarEsquerda(float angulo_graus) {
  long tempo = angulo_graus * TEMPO_BASE_GRAUS;
  curvaEsquerda();
  delay(tempo);
  parar();
}

void girarDireita(float angulo_graus) {
  long tempo = angulo_graus * TEMPO_BASE_GRAUS;
  curvaDireita();
  delay(tempo);
  parar();
}

// ================================
// EEPROM - ARMAZENAMENTO DE MOVIMENTOS
// ================================
int totalComandos = 0;

void salvarMovimento(char tipo, float valor) {
  if (totalComandos >= MAX_COMANDOS) {
    Serial.println("⚠️ Memória cheia! Não é possível salvar mais comandos.");
    return;
  }
  Movimento m;
  m.tipo = tipo;
  m.valor = valor;
  EEPROM.put(totalComandos * sizeof(Movimento), m);
  totalComandos++;
  EEPROM.put(EEPROM.length() - sizeof(int), totalComandos); // grava total no final da EEPROM
  Serial.println("Comando salvo na memória.");
}

void carregarTotalComandos() {
  EEPROM.get(EEPROM.length() - sizeof(int), totalComandos);
  if (totalComandos < 0 || totalComandos > MAX_COMANDOS) totalComandos = 0;
}

void limparMemoria() {
  for (int i = 0; i < EEPROM.length(); i++) EEPROM.write(i, 0);
  totalComandos = 0;
  Serial.println("Memória limpa!");
}

void replayMovimentos() {
  Serial.print("Reproduzindo ");
  Serial.print(totalComandos);
  Serial.println(" comandos gravados...");

  for (int i = 0; i < totalComandos; i++) {
    Movimento m;
    EEPROM.get(i * sizeof(Movimento), m);

    if (m.tipo == 'F') moverFrente(m.valor);
    else if (m.tipo == 'E') girarEsquerda(m.valor);
    else if (m.tipo == 'D') girarDireita(m.valor);

    delay(500); // pequena pausa entre comandos
  }
  Serial.println("Reprodução finalizada.");
}

// ================================
// COMUNICAÇÃO BLUETOOTH
// ================================
String comando = "";

void setup() {
  Serial.begin(9600); // comunicação com módulo Bluetooth (TX/RX)

  pinMode(MOTOR_ESQ_FRENTE, OUTPUT);
  pinMode(MOTOR_ESQ_TRAS, OUTPUT);
  pinMode(MOTOR_DIR_FRENTE, OUTPUT);
  pinMode(MOTOR_DIR_TRAS, OUTPUT);

  parar();
  carregarTotalComandos();

  Serial.println("Carrinho pronto! Aguardando comandos Bluetooth...");
  Serial.print("Comandos gravados na memória: ");
  Serial.println(totalComandos);
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      comando.trim();
      Serial.print("Recebido: ");
      Serial.println(comando);
      processarComando(comando);
      comando = "";
    } else {
      comando += c;
    }
  }
}

// ================================
// INTERPRETAÇÃO DOS COMANDOS
// ================================
void processarComando(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  if (cmd.startsWith("FRENTE")) {
    float dist = cmd.substring(7).toFloat();
    Serial.print("Andando para frente ");
    Serial.println(dist);
    moverFrente(dist);
    salvarMovimento('F', dist);
  } 
  else if (cmd.startsWith("ESQUERDA")) {
    float ang = cmd.substring(9).toFloat();
    Serial.print("Virando à esquerda ");
    Serial.println(ang);
    girarEsquerda(ang);
    salvarMovimento('E', ang);
  } 
  else if (cmd.startsWith("DIREITA")) {
    float ang = cmd.substring(8).toFloat();
    Serial.print("Virando à direita ");
    Serial.println(ang);
    girarDireita(ang);
    salvarMovimento('D', ang);
  } 
  else if (cmd == "PARAR") {
    parar();
  } 
  else if (cmd == "REPLAY") {
    replayMovimentos();
  } 
  else if (cmd == "LIMPAR") {
    limparMemoria();
  } 
  else {
    Serial.println("Comando desconhecido!");
  }
}
