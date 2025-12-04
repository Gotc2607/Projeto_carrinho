
## 🛠️ FASE 1: Preparação do Computador

Antes de mexer no robô, vamos deixar o seu PC pronto.

1. Instalar o Python (se não tiver)

    Certifique-se de ter o Python instalado. Ao instalar, marque a opção "Add Python to PATH".

2. Instalar as Bibliotecas Necessárias O código da interface usa ferramentas específicas. Abra o seu terminal (CMD ou PowerShell) e digite o seguinte comando:
```bash
    pip install pyserial supabase
```
(Nota: O tkinter geralmente já vem com o Python. Se der erro, instale-o também).

3. Instalar o Arduino IDE

    Tenha o software do Arduino instalado para enviar o código para a placa.

## 🤖 FASE 2: Preparando o Robô (Cérebro e Segurança)

1. Segurança Primeiro (IMPORTANTE)

    Pegue uma caixa pequena ou um livro grosso.

    Coloque o chassi do robô em cima desse objeto de forma que as rodas fiquem suspensas no ar.

    Isso garante que, se algo der errado, o robô não sai correndo pela mesa.

2. Alimentação

    Conecte as baterias externas (pilhas ou Li-Ion) do robô e ligue a chave (switch).

    Lembrete: O cabo USB alimenta o Arduino, mas não tem força para girar os motores. As baterias precisam estar ligadas.

3. Carregar o Código no Arduino

    Conecte o Arduino ao PC pelo cabo USB.

    Abra o arquivo Carrinho.ino no Arduino IDE.

    Selecione a porta correta em Ferramentas > Porta.

    Clique no botão Carregar (seta para a direita).

    Aguarde a mensagem "Carregado com sucesso".

    Atenção: Agora FECHE o Arduino IDE. Se ele ficar aberto, a interface Python não conseguirá conectar.

## 🖥️ FASE 3: Iniciando a Interface Python

1. Abrir a Interface

    Vá até a pasta onde salvou o arquivo interface.py.

    Clique com o botão direito e abra no seu editor (VS Code, PyCharm) ou rode pelo terminal:
    Bash

    python interface.py

2. Entendendo a Tela

    Você verá o mapa, os botões de controle e, lá embaixo, o Terminal de Logs (a tela preta de mensagens).

## 🔗 FASE 4: O Teste Seguro (Via Cabo USB)

Recomendo fazer este primeiro teste com o cabo USB conectado, pois é mais estável.

1. Conectar

    Na interface, na área "Conexão", verifique se a caixa "Modo Simulação" está DESMARCADA.

    Escolha a porta COM (ex: COM3) na lista.

    Clique em Conectar.

    Sinal de Sucesso: No terminal de logs (parte inferior), aparecerá em verde: Porta Serial aberta com sucesso.

2. Enviar um Comando Simples

    Na área "Comandos", verifique se está selecionado FRENTE e digite 10 na caixa de valor.

    Clique em Adicionar.

    Clique no botão EXECUTAR LOTE.

3. O Momento da Verdade

    Olhe para o robô: As rodas devem girar brevemente.

    Olhe para o Log: Você deve ver mensagens azuis (RX) dizendo OK_ADD e OK_RUN.

4. Testar o "Entregar"

    Clique em "Limpar".

    Selecione ENTREGAR no menu e clique em Adicionar.

    Clique em EXECUTAR LOTE.

    O Servo motor deve se mover para 90 graus.

## 📡 FASE 5: Indo Sem Fio (Bluetooth)

Se tudo funcionou no cabo, vamos soltar as amarras.

1. Emparelhar no Windows (Só precisa fazer uma vez)

    Desconecte o cabo USB do robô (o Arduino ficará ligado pelas baterias).

    No Windows, vá em Configurações > Bluetooth e outros dispositivos.

    Clique em Adicionar dispositivo > Bluetooth.

    Procure por HC-05, HC-06 ou similar.

    Senha padrão: 1234 ou 0000.

    Espere conectar.

2. Descobrir a Porta Bluetooth

    O Windows cria portas COM virtuais para o Bluetooth.

    Reinicie a interface Python (interface.py) para ela atualizar a lista.

    Na lista de portas, agora devem aparecer novas opções (ex: COM5, COM6).

    Tente conectar na primeira porta nova que apareceu.

3. Testar Sem Fio

    Clique em Conectar.

    Mande o comando FRENTE 10 e clique em EXECUTAR LOTE.

    Se as rodas girarem, parabéns! Seu sistema está 100% funcional.

## ❓ Resolução de Problemas Rápidos

    Erro "Acesso Negado" ao conectar:

        Verifique se o Arduino IDE ou outro programa está usando a porta. Feche tudo e tente de novo.

    Interface conecta, mas robô não move:

        Verifique se as baterias do robô estão carregadas e a chave ligada.

    Log mostra "Timeout" (letras vermelhas):

        A conexão caiu ou você escolheu a porta COM errada. Tente outra porta na lista.

    Robô faz movimentos estranhos:

        Use o botão "Resetar Mapa" ou reconecte a bateria do Arduino para reiniciar o cérebro dele.