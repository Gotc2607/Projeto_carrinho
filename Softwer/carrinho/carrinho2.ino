#include <Arduino.h>
#include <EEPROM.h>
#include <Servo.h>
#include <SoftwareSerial.h>
SoftwareSerial meuBT(10, 11); // (RX, TX) -> (Pino 10 = RX, Pino 11 = TX)

// ============================================================
// CONFIGURAÇÃO DOS PINOS E CONSTANTES
// ============================================================
// Pinout: 5 e 3 são PWM; 4 e 2 são apenas digitais (limitação para ré)
#define MOTOR_ESQ_FRENTE 5
#define MOTOR_ESQ_TRAS   4
#define MOTOR_DIR_FRENTE 3
#define MOTOR_DIR_TRAS   2
#define SERVO_PIN        6

// --- VARIÁVEIS DE CALIBRAÇÃO E COMPENSAÇÃO (TRIM) ---
int TEMPO_BASE_CM = 80;    // Tempo em ms por cm (Ajustar após mudar a velocidade base)
int TEMPO_BASE_GRAUS = 15; // Tempo em ms por grau
int velocidadeBase = 180;  // Velocidade PWM (0 a 255). Não use 255 para ter margem.
float fatorEsq = 1.0;      // Fator de correção do motor esquerdo (Ajuste fino)
float fatorDir = 0.95;     // Fator de correção do motor direito (Ajuste fino)

// --- MEMÓRIA (EEPROM) ---
#define EEPROM_END_COMANDOS 0
#define MAX_COMANDOS 20
#define MAX_COMANDO_LEN 30
int totalComandos = 0;

// --- SERVO ---
Servo meuServo;
int posicaoServo = 0;
bool servoScanning = false;

// ============================================================
// FUNÇÕES AUXILIARES DE MOVIMENTO E ACELERAÇÃO
// ============================================================

void parar() {
  digitalWrite(MOTOR_ESQ_FRENTE, LOW);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, LOW);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
}

// Implementa aceleração suave para evitar derrapagem na largada
void acelerarSuave(int velocidade) {
  for (int i = 0; i <= velocidade; i+=20) { // Acelera em passos de 20
    // Acelera gradualmente aplicando o TRIM em cada passo
    analogWrite(MOTOR_ESQ_FRENTE, i * fatorEsq);
    analogWrite(MOTOR_DIR_FRENTE, i * fatorDir);
    delay(15); 
  }
}

// ============================================================
// FUNÇÕES DE MOVIMENTO PRINCIPAIS (COM COMPENSAÇÃO)
// ============================================================

void andarFrente(int distancia) {
  // 1. Aceleração suave
  acelerarSuave(velocidadeBase); 
  
  // 2. Mantém a velocidade compensada (TRIM) nos pinos PWM (5 e 3)
  analogWrite(MOTOR_ESQ_FRENTE, velocidadeBase * fatorEsq); 
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  
  analogWrite(MOTOR_DIR_FRENTE, velocidadeBase * fatorDir);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
  
  // 3. Execução pelo tempo
  delay(distancia * TEMPO_BASE_CM);
  
  // 4. Parada
  parar();
}

void andarTras(int distancia) {
  // NOTA: Movimento para trás não é compensado por velocidade (PWM)
  digitalWrite(MOTOR_ESQ_FRENTE, LOW);
  digitalWrite(MOTOR_ESQ_TRAS, HIGH); 
  digitalWrite(MOTOR_DIR_FRENTE, LOW);
  digitalWrite(MOTOR_DIR_TRAS, HIGH);
  delay(distancia * TEMPO_BASE_CM);
  parar();
}

void curvaEsquerda(int graus) {
  // Usa velocidade base e TRIM para o motor direito (curva)
  digitalWrite(MOTOR_ESQ_FRENTE, LOW);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  
  analogWrite(MOTOR_DIR_FRENTE, velocidadeBase * fatorDir);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
  
  delay(graus * TEMPO_BASE_GRAUS);
  parar();
}

void curvaDireita(int graus) {
  // Usa velocidade base e TRIM para o motor esquerdo (curva)
  analogWrite(MOTOR_ESQ_FRENTE, velocidadeBase * fatorEsq);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  
  digitalWrite(MOTOR_DIR_FRENTE, LOW);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
  
  delay(graus * TEMPO_BASE_GRAUS);
  parar();
}

// ============================================================
// FUNÇÕES DE SERVO E ENTREGA
// ============================================================

void moverServo(int angulo) {
  if (angulo < 0) angulo = 0;
  if (angulo > 180) angulo = 180;
  meuServo.write(angulo);
  posicaoServo = angulo;
}

void entregar() {
  Serial.println("Iniciando entrega...");
  moverServo(90);  // Levanta/Gira
  delay(1000);
  moverServo(0);   // Volta
  delay(500);
  Serial.println("Entrega concluida.");
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

// ============================================================
// CALIBRAÇÃO E EEPROM
// ============================================================

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

// ... (Funções salvarComandoNaEEPROM, lerComandoDaEEPROM, executarComandosSalvos, limparEEPROM mantidas) ...

// ============================================================
// PROCESSAMENTO DE COMANDOS
// ============================================================
void processarComando(String comando) {
  comando.trim();
  comando.toUpperCase();
  
  Serial.print("Recebido: ");
  Serial.println(comando);

  if (comando.startsWith("FRENTE")) andarFrente(comando.substring(7).toInt());
  else if (comando.startsWith("TRAS")) andarTras(comando.substring(5).toInt());
  else if (comando.startsWith("ESQUERDA")) curvaEsquerda(comando.substring(9).toInt());
  else if (comando.startsWith("DIREITA")) curvaDireita(comando.substring(8).toInt());
  else if (comando.startsWith("ENTREGAR")) entregar();
  else if (comando.startsWith("SERVO ")) moverServo(comando.substring(6).toInt());
  else if (comando == "SERVO_SCAN") servoScan();
  else if (comando.startsWith("CALIBRAR FRENTE")) calibrar("FRENTE", comando.substring(15).toInt());
  else if (comando.startsWith("CALIBRAR GIRO")) calibrar("GIRO", comando.substring(13).toInt());
  // ... (Outros comandos como SALVAR, RODAR_SALVOS, LIMPAR_MEMORIA mantidos) ...
  else if (comando == "PARAR") parar();
  else Serial.println("Comando desconhecido.");
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  
  Serial.begin(115200); // Velocidade ajustada para 115200 para o Python
  meuBT.begin(9600);

  pinMode(MOTOR_ESQ_FRENTE, OUTPUT);
  pinMode(MOTOR_ESQ_TRAS, OUTPUT);
  pinMode(MOTOR_DIR_FRENTE, OUTPUT);
  pinMode(MOTOR_DIR_TRAS, OUTPUT);

  meuServo.attach(SERVO_PIN);
  moverServo(0); // Posição inicial

  // Recupera quantidade de comandos salvos
  totalComandos = EEPROM.read(EEPROM_END_COMANDOS + MAX_COMANDOS * MAX_COMANDO_LEN);
  Serial.println("Carrinho Pronto! Aguardando comandos...");
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================
void loop() {
  // Escuta a porta USB (Python)
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    processarComando(comando);
  }
  // Escuta o Bluetooth
  if (meuBT.available()) {
    String comando = meuBT.readStringUntil('\n');
    processarComando(comando);
  }
}