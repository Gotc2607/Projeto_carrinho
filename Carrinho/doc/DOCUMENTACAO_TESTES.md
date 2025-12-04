# 🧪 Documentação dos Testes Automatizados do Projeto

Esta documentação descreve todos os testes implementados no projeto, explica sua finalidade, como executá‑los e como expandir a suíte de testes.  

Os testes são essenciais para garantir que a lógica do movimento, simulação e comportamento do robô funcionem corretamente, mesmo sem hardware físico conectado.

---

# 📁 Estrutura dos Arquivos de Teste

```
test_interface.py       → Testes da lógica interna de movimento e entrega
mock_serial.py          → Simulador de porta serial para testes sem Arduino
interface.py            → Arquivo testado
```

---

# 🧩 Objetivo dos Testes

Os testes validam quatro aspectos principais:

### ✔ 1. Movimentação Linear  
Garante que o robô avance corretamente no eixo X quando executa o comando:

```python
simular_movimento("FRENTE 100")
```

### ✔ 2. Giros e Movimento Combinado  
Valida a trigonometria do robô ao girar e mover-se:

```python
simular_movimento("ESQUERDA 90")
simular_movimento("FRENTE 100")
```

### ✔ 3. Registro de Entregas  
Confirma que o ponto onde um ovo foi deixado é corretamente registrado no mapa e armazenado na lista interna.

### ✔ 4. Reset e Estado Inicial  
O método `setUp()` garante que cada teste comece com o robô na posição 0,0 e rotação 0°.

---

# 📜 Código dos Testes (explicado)

## 1. Teste de Movimento para Frente

```python
def test_movimento_frente(self):
    interface.simular_movimento("FRENTE 100", animate=False)
    self.assertAlmostEqual(interface.robot_x, 100)
    self.assertEqual(interface.robot_y, 0)
```

**O que valida:**  
- Movimento no eixo X  
- Ausência de movimento no eixo Y  

---

## 2. Teste de Giro + Movimento

```python
def test_giro_e_movimento(self):
    interface.simular_movimento("ESQUERDA 90", animate=False)
    interface.simular_movimento("FRENTE 100", animate=False)
    self.assertAlmostEqual(interface.robot_x, 0, delta=0.1)
    self.assertAlmostEqual(interface.robot_y, 100, delta=0.1)
    self.assertEqual(interface.robot_theta, 90)
```

**O que valida:**  
- Geração correta do novo ângulo  
- Cálculo trigonométrico da nova posição  
- Pequena tolerância numérica (`delta=0.1`)  

---

## 3. Teste de Entrega

```python
def test_comando_ovo(self):
    interface.simular_movimento("FRENTE 50", animate=False)
    interface.simular_movimento("ENTREGAR", animate=False)

    self.assertEqual(len(interface.ovos_entregues), 1)
    expected_x = (interface.CANVAS_WIDTH/2) + (50 * interface.SCALE)
    self.assertEqual(interface.ovos_entregues[0][0], expected_x)
```

**O que valida:**  
- Registro do ponto de entrega  
- Conversão da coordenada no canvas  
- Uso correto da escala (SCALE)  

---

# 🔧 Como Executar os Testes

No terminal, navegue até o diretório do projeto e execute:

```bash
python -m unittest test_interface.py
```

Ou para rodar todos os testes automaticamente:

```bash
python -m unittest discover
```

---

# 🧱 Como Funciona o Ambiente de Testes

A função `setUp()` é executada antes de cada teste:

```python
def setUp(self):
    interface.robot_x = 0
    interface.robot_y = 0
    interface.robot_theta = 0
    interface.ovos_entregues = []
    interface.path_points = []
```

Isso garante:
- Ambiente limpo  
- Resultados consistentes  
- Isolamento entre testes  

---

# 🤖 Testes com o MockSerial

Embora ainda não exista um arquivo de testes separado para comunicação serial, o arquivo `mock_serial.py` permite que você:

- Simule respostas do Arduino  
- Teste o protocolo de comunicação  
- Garanta que a interface responda corretamente a cada ACK  

Exemplo de respostas simuladas:

```
OK_CLR
OK_ADD
OK_RUN
STEP_DONE 0
FINISH
```

---

# ➕ Como Criar Novos Testes

Aqui estão ideias para expandir a suíte:

### 📌 Testar:
- Reset do mapa (`reset_path()`)
- Execução de sequências completas
- Histórico e carregamento de rotas no Supabase (usando mock)
- Comportamento com valores negativos
- Limites: giros maiores que 360°, posições muito grandes

Exemplo de novo teste:

```python
def test_reset_path(self):
    interface.simular_movimento("FRENTE 30")
    interface.reset_path()
    self.assertEqual(interface.robot_x, 0)
    self.assertEqual(interface.robot_y, 0)
    self.assertEqual(interface.robot_theta, 0)
```

---

# 📝 Conclusão

Os testes atuais já garantem que:
- A movimentação está correta
- A trigonometria funciona
- Os pontos de entrega são registrados corretamente

E com o ambiente MockSerial é possível expandir a cobertura para validar a comunicação completa sem depender do hardware.

---

# 📄 Licença

Este documento segue a mesma licença do projeto principal.
