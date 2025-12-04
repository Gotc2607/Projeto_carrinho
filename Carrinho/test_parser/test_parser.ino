#include <Arduino.h>  
#include <string.h>   

// --- CÓDIGO DE TESTE ---

#define MAX_FILA_RAM 30
#define MAX_COMANDO_LEN 20
char filaRAM[MAX_FILA_RAM][MAX_COMANDO_LEN];
byte totalFilaRAM = 0;

// Mock da função responder
void responder(String msg) { 
  Serial.print("[RESP] "); Serial.println(msg); 
}

void limparFilaRAM() { totalFilaRAM = 0; responder("OK_CLR"); }

void adicionarNaFilaRAM(char* cmd) {
  if (totalFilaRAM >= MAX_FILA_RAM) { responder("ERR_FULL"); return; }
  strncpy(filaRAM[totalFilaRAM], cmd, MAX_COMANDO_LEN - 1);
  filaRAM[totalFilaRAM][MAX_COMANDO_LEN - 1] = '\0';
  totalFilaRAM++;
  responder("OK_ADD"); 
}

void filtrar(char* str) {
  byte i = 0, j = 0;
  while (str[i] != '\0' && j < MAX_COMANDO_LEN - 1) {
    char c = str[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
        (c >= '0' && c <= '9') || c == '(' || c == ')' || c == '-') {
      if (c >= 'a' && c <= 'z') str[j++] = c - 32;
      else str[j++] = c;
    } else if (c == '@' || c == '\\') break;
    i++;
  }
  str[j] = '\0';
}

void processarComandoTeste(const char* comando) {
  char cmdLimpo[MAX_COMANDO_LEN];
  strncpy(cmdLimpo, comando, MAX_COMANDO_LEN - 1);
  cmdLimpo[MAX_COMANDO_LEN - 1] = '\0';
  filtrar(cmdLimpo); 

  Serial.print("Input: "); Serial.print(comando);
  Serial.print(" -> Processado: "); Serial.println(cmdLimpo);

  if (strncmp(cmdLimpo, "ADD(", 4) == 0) {
     char sub[MAX_COMANDO_LEN]; byte i=4, j=0;
     while (cmdLimpo[i] != ')' && cmdLimpo[i] != '\0') sub[j++] = cmdLimpo[i++];
     sub[j] = '\0'; 
     adicionarNaFilaRAM(sub); 
  }
}

void setup() {
  Serial.begin(9600);
  while(!Serial); // Aguarda a serial conectar (importante em alguns Arduinos)
  Serial.println("\n--- INICIANDO SUITE DE TESTES ---");

  // TESTE 1: Limpeza de string suja
  char t1[] = "aDd(f(100))\\"; 
  filtrar(t1);
  if (strcmp(t1, "ADD(F(100))") == 0) Serial.println("TESTE 1 (Filtro): PASSOU");
  else Serial.println("TESTE 1 (Filtro): FALHOU");

  // TESTE 2: Adicionar na Fila e verificar Limites
  limparFilaRAM();
  processarComandoTeste("ADD(F(50))");
  processarComandoTeste("ADD(D(90))");
  
  if (totalFilaRAM == 2) Serial.println("TESTE 2 (Contagem): PASSOU");
  else { Serial.print("TESTE 2 (Contagem): FALHOU. Total: "); Serial.println(totalFilaRAM); }

  if (strcmp(filaRAM[0], "F(50)") == 0) Serial.println("TESTE 3 (Item 1): PASSOU");
  else Serial.println("TESTE 3 (Item 1): FALHOU");
  
  // TESTE 4: Overflow da Fila
  Serial.println("Enchendo fila...");
  for(int i=0; i<35; i++) adicionarNaFilaRAM("F(1)");
  if(totalFilaRAM == MAX_FILA_RAM) Serial.println("TESTE 4 (Protecao Max): PASSOU");
  else Serial.println("TESTE 4 (Protecao Max): FALHOU");
}

void loop() {}