# 🤖 **Robô Entregador – Controle via Interface Gráfica (Python + Arduino)**

Este projeto implementa um sistema completo para controlar um robô 4WD com servo de entrega, capaz de executar rotas planejadas, simular o movimento na tela e registrar rotas na nuvem via **Supabase**.

Ele foi desenvolvido para testes reais com Arduino, para simulação via software e também para testes automatizados usando uma porta serial mock.

---

# 🧩 **Visão Geral**

O sistema oferece:

### ✅ **Interface gráfica em Python (Tkinter)**
- Criação de rotas por comandos.
- Execução da rota passo a passo.
- Visualização gráfica do movimento (Dead Reckoning).
- Marcação de pontos de entrega no mapa.
- Conexão com Arduino real, Bluetooth ou modo simulado.

### ✅ **Comunicação com Arduino**
- Protocolo próprio baseado em comandos:
  - `LIMPARFILA\`
  - `ADD(X(valor))\`
  - `EXECUTAR\`
- Respostas aguardadas:
  - `OK_CLR`
  - `OK_ADD`
  - `OK_RUN`
  - `STEP_DONE`
  - `FINISH`

### ✅ **Modo Simulado / MockSerial**
Permite testar toda a interface sem Arduino físico.

### ✅ **Integração com Supabase**
- Armazena rotas.
- Salva comandos na ordem correta.
- Carrega rotas do histórico.
- Mantém cache local para navegação rápida.

### ✅ **Testes automatizados**
O arquivo `test_interface.py` usa `unittest` para validar a lógica interna dos movimentos.

---

# 🛠️ **Tecnologias Utilizadas**

| Tecnologia | Uso |
|-----------|-----|
| **Python 3.x** | Interface gráfica e lógica principal |
| **Tkinter** | GUI |
| **PySerial** | Comunicação serial com Arduino |
| **Supabase Python Client** | Banco de dados na nuvem |
| **Arduino (C++)** | Firmware do robô |
| **MockSerial** | Testes sem hardware |
| **unittest** | Testes automatizados |

---

# 📡 **Arquitetura do Sistema**

### 🖥️ **Python / Interface**
A GUI controla:
- Construção da fila de comandos
- Conexão com portas reais ou mock
- Sincronização com Supabase
- Renderização gráfica do caminho
- Execução simulada ou real

### 🤖 **Arduino**
Recebe e executa comandos:
- Movimento para frente/trás
- Giros
- Comando `ENTREGAR` (servo)

### ☁️ **Supabase**
Tabelas:
- `rotas` → nome e data
- `comandos` → { rota_id, ordem, comando, valor }

---

# 📦 **Instalação**

### 1) Como rodar a interface Python

Instale dependências:

```bash
pip install pyserial supabase
```

Depois execute:

```bash
python interface.py
```

### 2) Permissões (Linux)

```bash
sudo usermod -a -G dialout $USER
sudo apt install libfuse2
```

---

# 🎮 **Como Usar**

### 1) Conecte o robô
- USB (Serial)
- Bluetooth
- ou selecione **TESTE** para usar MockSerial

### 2) Monte a rota
- Escolha um comando (FRENTE, ESQUERDA, DIREITA, ENTREGAR)
- Defina o valor (se necessário)
- Clique **Adicionar**

### 3) Execute
- Clique em **EXECUTAR**
- O mapa será atualizado conforme os passos são concluídos

### 4) Salve na nuvem
- Digite o nome da rota
- Clique **Salvar na Nuvem**

### 5) Histórico
- Aba **Histórico**
- Clique em **Atualizar**
- Clique em **Carregar** para simular imediatamente a rota do banco

---

# 🔌 **Protocolo de Comunicação (Resumo)**

### Comandos enviados:

```
LIMPARFILA\
ADD(F(100))\
ADD(E(90))\
ADD(O(0))\
EXECUTAR\
```

### Respostas esperadas:

```
OK_CLR
OK_ADD
OK_ADD
OK_ADD
OK_RUN
STEP_DONE 0
STEP_DONE 1
STEP_DONE 2
FINISH
```

---

# 🧪 **Testes Automatizados**

Exemplo de execução:

```bash
python -m unittest test_interface.py
```

---

# ✨ **Modo Teste (MockSerial)**

Ideal para quando você está sem Arduino.

- Simula respostas `OK_ADD`, `STEP_DONE`, etc.
- Permite testar toda a interface sem hardware.

---

# 🗺️ **Visualização Gráfica**

A interface exibe:
- Plano cartesiano
- Caminho percorrido
- Posição atual do robô
- Ângulo
- Marcadores de entregas (ovos) em dourado

---

# 📄 **Licença**

Uso livre para projetos pessoais, acadêmicos e de pesquisa.
