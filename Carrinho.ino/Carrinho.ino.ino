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
int TEMPO_BASE_CM = 30;
int TEMPO_BASE_GRAUS = 8;

// ---------------------------
// CONSTANTES DE TEMPO MÍNIMO
// ---------------------------
#define TEMPO_MINIMO_CM 15
#define TEMPO_MINIMO_GRAUS 200

// ---------------------------
// SERVO
// ---------------------------
Servo meuServo;
byte posicaoServo = 0;
bool servoScanning = false;
unsigned long lastServoMove = 0;
byte servoScanAngle = 0;
bool servoScanDirection = true;

// ---------------------------
// MEMÓRIA (EEPROM)
// ---------------------------
#define EEPROM_END_COMANDOS 0
#define MAX_COMANDOS 10
#define MAX_COMANDO_LEN 20
byte totalComandos = 0;

// ---------------------------
// FUNÇÕES DE MOVIMENTO
// ---------------------------
void parar() {
  digitalWrite(MOTOR_ESQ_FRENTE, LOW);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, LOW);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
  Serial.println(F("Parado."));
}

void andarFrente(int distancia) {
  if (distancia <= 0) {
    Serial.println(F("Distancia invalida"));
    return;
  }
  
  unsigned long tempoMovimento = distancia * TEMPO_BASE_CM;
  if (tempoMovimento < TEMPO_MINIMO_CM) {
    tempoMovimento = TEMPO_MINIMO_CM;
  }
  
  if (distancia > 150) distancia = 150;
  
  Serial.print(F("Andando "));
  Serial.print(distancia);
  Serial.println(F("cm"));
  
  digitalWrite(MOTOR_ESQ_FRENTE, HIGH);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, HIGH);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
  
  delay(tempoMovimento);
  parar();
}

void curvaEsquerda(int graus) {
  if (graus <= 0) {
    Serial.println(F("Angulo invalido"));
    return;
  }
  
  unsigned long tempoMovimento = graus * TEMPO_BASE_GRAUS;
  if (tempoMovimento < TEMPO_MINIMO_GRAUS) {
    tempoMovimento = TEMPO_MINIMO_GRAUS;
  }
  
  if (graus > 360) graus = 360;
  
  Serial.print(F("Esquerda "));
  Serial.print(graus);
  Serial.println(F("°"));
  
  digitalWrite(MOTOR_ESQ_FRENTE, LOW);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, HIGH);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
  
  delay(tempoMovimento);
  parar();
}

void curvaDireita(int graus) {
  if (graus <= 0) {
    Serial.println(F("Angulo invalido"));
    return;
  }
  
  unsigned long tempoMovimento = graus * TEMPO_BASE_GRAUS;
  if (tempoMovimento < TEMPO_MINIMO_GRAUS) {
    tempoMovimento = TEMPO_MINIMO_GRAUS;
  }
  
  if (graus > 360) graus = 360;
  
  Serial.print(F("Direita "));
  Serial.print(graus);
  Serial.println(F("°"));
  
  digitalWrite(MOTOR_ESQ_FRENTE, HIGH);
  digitalWrite(MOTOR_ESQ_TRAS, LOW);
  digitalWrite(MOTOR_DIR_FRENTE, LOW);
  digitalWrite(MOTOR_DIR_TRAS, LOW);
  
  delay(tempoMovimento);
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
  Serial.print(F("Servo: "));
  Serial.println(angulo);
}

void iniciarServoScan() {
  servoScanning = true;
  servoScanAngle = 0;
  servoScanDirection = true;
  Serial.println(F("Scan servo..."));
}

void atualizarServoScan() {
  if (!servoScanning) return;
  
  if (millis() - lastServoMove >= 20) {
    if (servoScanDirection) {
      servoScanAngle++;
      if (servoScanAngle >= 90) servoScanDirection = false;
    } else {
      servoScanAngle--;
      if (servoScanAngle <= 0) {
        servoScanDirection = true;
        servoScanning = false;
        Serial.println(F("Scan completo"));
      }
    }
    meuServo.write(servoScanAngle);
    lastServoMove = millis();
  }
}

// ---------------------------
// CALIBRAÇÃO
// ---------------------------
void calibrar(const char* tipo, int valor) {
  if (strcmp(tipo, "FRENTE") == 0) {
    TEMPO_BASE_CM = valor;
    Serial.print(F("Novo tempo cm: "));
    Serial.println(TEMPO_BASE_CM);
  } else if (strcmp(tipo, "GIRO") == 0) {
    TEMPO_BASE_GRAUS = valor;
    Serial.print(F("Novo tempo grau: "));
    Serial.println(TEMPO_BASE_GRAUS);
  }
}

// ---------------------------
// EEPROM
// ---------------------------
void salvarComandoNaEEPROM(const char* cmd) {
  if (totalComandos >= MAX_COMANDOS) {
    Serial.println(F("Memoria cheia!"));
    return;
  }

  char cmdLimpo[MAX_COMANDO_LEN];
  byte j = 0;
  
  // Aplica o mesmo filtro usado nos comandos
  for (byte i = 0; cmd[i] != '\0' && i < MAX_COMANDO_LEN - 1; i++) {
    char c = cmd[i];
    if ((c >= 'A' && c <= 'Z') || 
        (c >= 'a' && c <= 'z') || 
        (c >= '0' && c <= '9') || 
        c == '(' || c == ')' || c == '-') {
      if (c >= 'a' && c <= 'z') {
        cmdLimpo[j++] = c - 32;
      } else {
        cmdLimpo[j++] = c;
      }
    }
  }
  cmdLimpo[j] = '\0';

  int addr = EEPROM_END_COMANDOS + totalComandos * MAX_COMANDO_LEN;
  byte i;
  for (i = 0; i < MAX_COMANDO_LEN - 1 && cmdLimpo[i] != '\0'; i++) {
    EEPROM.write(addr + i, cmdLimpo[i]);
  }
  EEPROM.write(addr + i, '\0');

  totalComandos++;
  EEPROM.write(EEPROM_END_COMANDOS + MAX_COMANDOS * MAX_COMANDO_LEN, totalComandos);

  Serial.print(F("Salvo: "));
  Serial.println(cmdLimpo);
}

void lerComandoDaEEPROM(int index, char* buffer) {
  if (index < 0 || index >= totalComandos) {
    buffer[0] = '\0';
    return;
  }
  
  int addr = EEPROM_END_COMANDOS + index * MAX_COMANDO_LEN;
  byte i;
  for (i = 0; i < MAX_COMANDO_LEN; i++) {
    buffer[i] = EEPROM.read(addr + i);
    if (buffer[i] == '\0') break;
  }
  if (i == MAX_COMANDO_LEN) buffer[MAX_COMANDO_LEN - 1] = '\0';
}

void executarComandosSalvos() {
  if (totalComandos == 0) {
    Serial.println(F("Nenhum comando salvo!"));
    return;
  }
  
  Serial.println(F("Executando..."));
  char buffer[MAX_COMANDO_LEN];
  
  for (byte i = 0; i < totalComandos; i++) {
    lerComandoDaEEPROM(i, buffer);
    Serial.print(F("["));
    Serial.print(i + 1);
    Serial.print(F("]: "));
    Serial.println(buffer);
    delay(300);
    
    processarComando(buffer);
    delay(200);
  }
  Serial.println(F("Concluido."));
}

void limparEEPROM() {
  for (byte i = 0; i < MAX_COMANDOS; i++) {
    int addr = EEPROM_END_COMANDOS + i * MAX_COMANDO_LEN;
    EEPROM.write(addr, 0);
  }
  totalComandos = 0;
  EEPROM.write(EEPROM_END_COMANDOS + MAX_COMANDOS * MAX_COMANDO_LEN, 0);
  Serial.println(F("EEPROM limpa!"));
}

void listarComandosSalvos() {
  if (totalComandos == 0) {
    Serial.println(F("Nenhum comando."));
    return;
  }
  
  Serial.println(F("Comandos salvos:"));
  char buffer[MAX_COMANDO_LEN];
  for (byte i = 0; i < totalComandos; i++) {
    lerComandoDaEEPROM(i, buffer);
    Serial.print(F("["));
    Serial.print(i + 1);
    Serial.print(F("] "));
    Serial.println(buffer);
  }
}

// ---------------------------
// FILTRO PARA DABBLE
// ---------------------------
void filtrarComandoDabble(char* str) {
  byte i = 0, j = 0;
  
  // Filtro ULTRA RESTRITO - mantém APENAS caracteres válidos para comandos
  while (str[i] != '\0' && j < MAX_COMANDO_LEN - 1) {
    char c = str[i];
    
    // Mantém APENAS: letras, números, parênteses e hífen (para números negativos)
    if ((c >= 'A' && c <= 'Z') || 
        (c >= 'a' && c <= 'z') || 
        (c >= '0' && c <= '9') || 
        c == '(' || c == ')' || c == '-') {
      
      // Converte para maiúsculas se for letra minúscula
      if (c >= 'a' && c <= 'z') {
        str[j++] = c - 32;
      } else {
        str[j++] = c;
      }
    }
    // Se encontrar @ ou \, para completamente
    else if (c == '@' || c == '\\') {
      break;
    }
    
    i++;
  }
  
  str[j] = '\0';
}

// ---------------------------
// PROCESSAMENTO DE COMANDOS
// ---------------------------
void processarComando(const char* comando) {
  char comandoLimpo[MAX_COMANDO_LEN];
  char comandoFormatado[MAX_COMANDO_LEN];
  
  // Copia o comando para buffer local
  strncpy(comandoLimpo, comando, MAX_COMANDO_LEN - 1);
  comandoLimpo[MAX_COMANDO_LEN - 1] = '\0';
  
  // Aplica filtro específico para Dabble
  filtrarComandoDabble(comandoLimpo);
  
  // Verifica se está vazio após filtro
  if (strlen(comandoLimpo) == 0) {
    return;
  }

  Serial.print(F("Comando: "));
  Serial.println(comandoLimpo);

  // Tenta converter comandos sem parênteses para o formato com parênteses
  if (comandoLimpo[0] == 'F' || comandoLimpo[0] == 'G' || 
      comandoLimpo[0] == 'E' || comandoLimpo[0] == 'D' || 
      comandoLimpo[0] == 'S') {
    
    // Se não tem parênteses, adiciona
    if (comandoLimpo[1] != '(') {
      int valor = atoi(comandoLimpo + 1);
      snprintf(comandoFormatado, MAX_COMANDO_LEN, "%c(%d)", comandoLimpo[0], valor);
      strcpy(comandoLimpo, comandoFormatado);
      Serial.print(F("Formatado para: "));
      Serial.println(comandoLimpo);
    }
  }

  // NOVOS COMANDOS PADRONIZADOS - sintaxe F(50), G(90), S(45)
  if (comandoLimpo[0] == 'F' && comandoLimpo[1] == '(') {
    int distancia = atoi(comandoLimpo + 2);
    andarFrente(distancia);
  }
  else if (comandoLimpo[0] == 'G' && comandoLimpo[1] == '(') {
    int angulo = atoi(comandoLimpo + 2);
    if (angulo >= 0) {
      curvaDireita(angulo);
    } else {
      curvaEsquerda(-angulo);
    }
  }
  else if (comandoLimpo[0] == 'E' && comandoLimpo[1] == '(') {
    int angulo = atoi(comandoLimpo + 2);
    curvaEsquerda(angulo);
  }
  else if (comandoLimpo[0] == 'D' && comandoLimpo[1] == '(') {
    int angulo = atoi(comandoLimpo + 2);
    curvaDireita(angulo);
  }
  else if (comandoLimpo[0] == 'S' && comandoLimpo[1] == '(') {
    int angulo = atoi(comandoLimpo + 2);
    moverServo(angulo);
  }
  // COMANDO SALVAR - NOVA FUNCIONALIDADE
  else if (comandoLimpo[0] == 'S' && comandoLimpo[1] == 'A' && comandoLimpo[2] == 'L' && 
           comandoLimpo[3] == 'V' && comandoLimpo[4] == 'A' && comandoLimpo[5] == 'R' && 
           comandoLimpo[6] == '(') {
    
    // Extrai o comando dentro dos parênteses
    char comandoParaSalvar[MAX_COMANDO_LEN];
    byte i = 7, j = 0;
    
    while (comandoLimpo[i] != ')' && comandoLimpo[i] != '\0' && j < MAX_COMANDO_LEN - 1) {
      comandoParaSalvar[j++] = comandoLimpo[i++];
    }
    comandoParaSalvar[j] = '\0';
    
    if (strlen(comandoParaSalvar) > 0) {
      salvarComandoNaEEPROM(comandoParaSalvar);
    } else {
      Serial.println(F("Erro: Comando vazio!"));
    }
  }
  // COMANDOS DE SISTEMA (sem parâmetros)
  else if (strcmp(comandoLimpo, "SERVOSCAN") == 0) {
    iniciarServoScan();
  }
  else if (strcmp(comandoLimpo, "RODARSALVOS") == 0) {
    executarComandosSalvos();
  }
  else if (strcmp(comandoLimpo, "LISTARSALVOS") == 0) {
    listarComandosSalvos();
  }
  else if (strcmp(comandoLimpo, "LIMPARMEMORIA") == 0) {
    limparEEPROM();
  }
  else if (strcmp(comandoLimpo, "PARAR") == 0) {
    parar();
  }
  else if (strcmp(comandoLimpo, "AJUDA") == 0) {
    Serial.println(F("=== COMANDOS PADRONIZADOS ==="));
    Serial.println(F("F(50)\\     - Anda 50cm"));
    Serial.println(F("G(90)\\     - Gira 90° direita"));
    Serial.println(F("G(-90)\\    - Gira 90° esquerda"));
    Serial.println(F("E(90)\\     - Esquerda 90°"));
    Serial.println(F("D(90)\\     - Direita 90°"));
    Serial.println(F("S(45)\\     - Servo 45°"));
    Serial.println(F("SALVAR(F(50))\\ - Salva comando"));
    Serial.println(F("SERVOSCAN\\       - Varredura servo"));
    Serial.println(F("RODARSALVOS\\     - Executa sequencia"));
    Serial.println(F("LISTARSALVOS\\    - Lista salvos"));
    Serial.println(F("LIMPARMEMORIA\\   - Limpa memoria"));
    Serial.println(F("PARAR\\           - Para motores"));
    Serial.println(F("AJUDA\\           - Mostra ajuda"));
  }
  else {
    Serial.print(F("Comando desconhecido: "));
    Serial.println(comandoLimpo);
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

  totalComandos = EEPROM.read(EEPROM_END_COMANDOS + MAX_COMANDOS * MAX_COMANDO_LEN);
  if (totalComandos > MAX_COMANDOS) totalComandos = 0;

  Serial.println(F("=== CARRINHO DABBLE ==="));
  Serial.println(F("Comandos: F(dist), G(ang), S(ang)"));
  Serial.println(F("Use '\\' como terminador"));
  Serial.println(F("Digite AJUDA\\ para ver comandos"));
}

// ---------------------------
// LOOP PRINCIPAL
// ---------------------------
void loop() {
  if (meuBT.available()) {
    char buffer[MAX_COMANDO_LEN];
    byte index = 0;
    unsigned long inicio = millis();
    bool comandoCompleto = false;
    
    // Lê até encontrar '\' (nosso terminador)
    while (meuBT.available() && index < MAX_COMANDO_LEN - 1 && !comandoCompleto) {
      char c = meuBT.read();
      
      if (c == '\\') {
        comandoCompleto = true;
        break;
      }
      
      buffer[index++] = c;
      
      if (millis() - inicio > 200) break;
    }
    buffer[index] = '\0';
    
    if (index > 0) {
      processarComando(buffer);
    }
  }
  
  // Atualiza servo scan
  atualizarServoScan();
  
  delay(10);
}