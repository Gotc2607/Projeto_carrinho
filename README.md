# 🤖 OncoMap - Robô Entregador Controlado via Interface Gráfica

Este projeto consiste em um sistema completo para controle de um carrinho robótico (4WD) através de uma interface gráfica (GUI) desenvolvida em Python. O sistema permite planejar rotas, visualizar o caminho estimado em tempo real (Dead Reckoning) e executar tarefas de entrega utilizando um braço robótico (Servo Motor).

![Status do Projeto](https://img.shields.io/badge/Status-Funcional-brightgreen)
![Python Version](https://img.shields.io/badge/Python-3.x-blue)
![Arduino](https://img.shields.io/badge/Hardware-Arduino-00979D)

## 📋 Funcionalidades

* **Controle de Movimento:** Comandos precisos para Frente, Trás, Esquerda e Direita.
* **Sistema de Entrega:** Comando específico para acionar o Servo Motor e realizar uma entrega.
* **Simulação Visual:** A interface desenha o caminho que o robô está percorrendo na tela enquanto envia os comandos.
* **Fila de Comandos:** Permite montar uma lista de ações complexas e enviá-las todas de uma vez.
* **Suporte Híbrido:** Código preparado para comunicação via USB (Serial) e Bluetooth.

## 🛠️ Hardware Necessário

* 1x Placa Arduino (Uno, Nano ou Mega)
* 1x Driver de Motor (Ponte H L298N ou similar)
* 4x Motores DC com Rodas (Chassi 4WD)
* 1x Servo Motor (ex: SG90) para o mecanismo de entrega
* 1x Módulo Bluetooth HC-05/HC-06 (Opcional para uso sem fio)
* Baterias para alimentação (Recomendado: Li-Ion 18650 ou LiPo)

### 🔌 Pinagem (Conexões)

Conecte os componentes no Arduino seguindo a configuração definida no firmware:

| Componente | Pino Arduino | Função |
| :--- | :---: | :--- |
| **Motor Esq. Frente** | 5 | Controle Rodas Esquerdas (Frente) |
| **Motor Esq. Trás** | 4 | Controle Rodas Esquerdas (Trás) |
| **Motor Dir. Frente** | 3 | Controle Rodas Direitas (Frente) |
| **Motor Dir. Trás** | 2 | Controle Rodas Direitas (Trás) |
| **Servo Motor** | 6 | Braço de Entrega |
| **Bluetooth RX** | 10 | Comunicação Wireless |
| **Bluetooth TX** | 11 | Comunicação Wireless |

> **Nota:** O GND do Arduino deve estar conectado ao GND da fonte externa das baterias (Terra Comum).

---

## 💻 Instalação e Configuração

### 1. Firmware (Arduino)

1.  Baixe e instale o [Arduino IDE](https://www.arduino.cc/en/software).
2.  Abra o arquivo `carrinho.ino`.
3.  Conecte o Arduino ao PC via USB.
4.  Selecione a placa e a porta correta em **Ferramentas**.
5.  Faça o **Upload** do código.

### 2. Software de Controle (Python)

Certifique-se de ter o Python instalado. Em seguida, instale a biblioteca de comunicação serial:

```bash
pip install pyserial
```
# 🚀 Como Usar

## Passo 1: Conexão Física

  - Ligue a alimentação do robô (baterias).
  - Conecte o Arduino ao computador via cabo USB.

## Passo 2: Executar a Interface

Abra o terminal na pasta do projeto e execute:
Bash

python interface.py

Passo 3: Operação

  - Na interface, localize a seção "Porta Serial".
  - Selecione a porta correspondente ao Arduino (ex: COM3 no Windows ou /dev/ttyUSB0 no Linux) e clique em Conectar.
  - Na área "Criar Rota", escolha o comando (FRENTE, ESQUERDA, etc.) e a distância/ângulo.
  - Clique em Adicionar. Repita para quantos movimentos desejar.
  - Clique em ENVIAR E RODAR.

# 🐧 Dicas para Usuários Linux

Se você tiver problemas para rodar ou conectar, verifique as permissões:

- 1. Permissão na Porta USB (Erro de Permissão/Acesso Negado): Adicione seu usuário ao grupo dialout para ter acesso à porta serial:
Bash

  ```sudo usermod -a -G dialout $USER```

  (Reinicie o computador após este comando)

- 2. Arduino IDE (AppImage): Se estiver usando o Arduino IDE via AppImage e ele não abrir ou der erro de Sandbox:
Bash

# Instalar dependência antiga
```sudo apt install libfuse2```

# Rodar sem sandbox (se der erro na inicialização)
```./arduino-ide.AppImage --no-sandbox```

# 📝 Licença

Este projeto é de código aberto. Use livremente para estudos e projetos acadêmicos.
