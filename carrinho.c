#include <Servo.h>

// --- Definição dos Pinos ---

// Pinos para o controle do Motor Direito (Motor A no L298N)
#define ENA 5   // Pino de Velocidade A (deve ser PWM ~)
#define IN1 7
#define IN2 8

// Pinos para o controle do Motor Esquerdo (Motor B no L298N)
#define ENB 6   // Pino de Velocidade B (deve ser PWM ~)
#define IN3 9
#define IN4 10

// Pino para o Servo Motor da garra/mecanismo do ovo
#define SERVO_PIN 3 // Deve ser um pino PWM (~)

// --- Variáveis Globais ---
Servo garraServo; // Cria o objeto para controlar o servo

// Posições do servo (ajuste conforme a necessidade do seu mecanismo)
const int POSICAO_SEGURAR = 90; // Ângulo em graus para segurar o ovo
const int POSICAO_SOLTAR = 0;   // Ângulo em graus para depositar o ovo

// --- Configuração Inicial ---
void setup() {
  // Inicia a comunicação serial para debugging (opcional)
  Serial.begin(9600);
  Serial.println("Iniciando o carrinho...");

  // Configura os pinos dos motores como SAÍDA
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Anexa o servo ao pino definido e o move para a posição inicial
  garraServo.attach(SERVO_PIN);
  segurarOvo(); // Garante que o carrinho comece segurando o ovo

  // Para garantir que os motores estejam parados no início
  parar();
}

// --- Funções de Movimento ---

void parar() {
  Serial.println("Parando...");
  // Desliga o Motor Direito
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  // Desliga o Motor Esquerdo
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void andarFrente(int velocidade) {
  Serial.println("Andando para frente...");
  // Define a direção para FRENTE para ambos os motores
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  
  // Define a velocidade dos motores (0 a 255)
  analogWrite(ENA, velocidade);
  analogWrite(ENB, velocidade);
}

void girarDireita(int velocidade) {
  Serial.println("Girando para a direita...");
  // Motor esquerdo para frente e motor direito para trás (giro no próprio eixo)
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH); // Motor direito para trás
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);  // Motor esquerdo para frente

  // Define a velocidade do giro
  analogWrite(ENA, velocidade);
  analogWrite(ENB, velocidade);
}

void girarEsquerda(int velocidade) {
  Serial.println("Girando para a esquerda...");
  // Motor direito para frente e motor esquerdo para trás (giro no próprio eixo)
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);  // Motor direito para frente
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH); // Motor esquerdo para trás
  
  // Define a velocidade do giro
  analogWrite(ENA, velocidade);
  analogWrite(ENB, velocidade);
}

// --- Funções do Mecanismo do Ovo ---

void segurarOvo() {
  Serial.println("Fechando a garra...");
  garraServo.write(POSICAO_SEGURAR);
}

void depositarOvo() {
  Serial.println("Depositando o ovo...");
  garraServo.write(POSICAO_SOLTAR);
}

// --- Loop Principal (O Percurso) ---
void loop() {
  // --- Exemplo de um Percurso ---
  // Você pode alterar os tempos (delay) e velocidades para se adequar ao seu trajeto.
  
  Serial.println("\nIniciando o percurso!");
  
  // 1. Anda para frente por 3 segundos
  andarFrente(200); // Velocidade 200 de 255
  delay(3000);

  // 2. Para por 1 segundo
  parar();
  delay(1000);

  // 3. Gira 90 graus para a direita (ajuste o tempo para o seu carrinho)
  girarDireita(180);
  delay(700); // O tempo necessário para girar 90 graus depende dos motores e da superfície

  // 4. Para por 1 segundo
  parar();
  delay(1000);

  // 5. Anda para frente por mais 2 segundos para chegar ao pote
  andarFrente(200);
  delay(2000);

  // 6. Para no local de depósito
  parar();
  delay(1500); // Espera um pouco

  // 7. Deposita o ovo
  depositarOvo();
  delay(2000); // Dá tempo para o ovo cair

  // 8. Levanta a garra novamente
  segurarOvo();
  delay(1000);

  // 9. Fim do percurso. O carrinho fica parado.
  Serial.println("Percurso finalizado!");
  parar();
  
  // Trava o loop para que o percurso seja executado apenas uma vez.
  // Se quiser que ele repita, remova as duas linhas abaixo.
  while(1) {
    delay(1000);
  }
}