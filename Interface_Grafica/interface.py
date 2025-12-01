# =================================================================
# == INTERFACE GRÁFICA PARA CONTROLE E VISUALIZAÇÃO DO CARRINHO  ==
# ==               COM PERSISTÊNCIA EM BANCO DE DADOS            ==
# =================================================================
import tkinter as tk
from tkinter import ttk
import math
import serial
import serial.tools.list_ports
import threading
import time
import sqlite3
from datetime import datetime

# --- Constantes da GUI ---
CANVAS_WIDTH = 600
CANVAS_HEIGHT = 600
SCALE = 10  # 10 pixels = 1 cm
ROBOT_RADIUS = 1.5 * SCALE 

# --- Variáveis Globais ---
ser = None
robot_x, robot_y, robot_theta = 0, 0, 0 # X(cm), Y(cm), Theta(graus)
path_points = []

# =================================================================
# FUNÇÕES DE BANCO DE DADOS (SQLite)
# =================================================================
def init_db():
    """Cria o banco de dados e as tabelas se não existirem"""
    try:
        conn = sqlite3.connect('rotas_carrinho.db')
        cursor = conn.cursor()
        
        # Tabela para guardar o cabeçalho da rota
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS rotas (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                nome TEXT NOT NULL,
                data_criacao TEXT
            )
        ''')
        
        # Tabela para guardar os comandos
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS comandos (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                rota_id INTEGER,
                ordem INTEGER,
                comando TEXT,
                valor REAL,
                FOREIGN KEY (rota_id) REFERENCES rotas (id)
            )
        ''')
        conn.commit()
        conn.close()
        print("Banco de dados inicializado com sucesso.")
    except Exception as e:
        print(f"Erro ao iniciar banco de dados: {e}")

def salvar_rota_db():
    """Salva a lista atual de comandos no banco de dados"""
    nome_rota = nome_rota_entry.get()
    comandos = command_listbox.get(0, tk.END)
    
    if not nome_rota:
        status_label.config(text="Erro: Digite um nome para a rota!")
        return
    
    if not comandos:
        status_label.config(text="Erro: Lista de comandos vazia!")
        return

    try:
        conn = sqlite3.connect('rotas_carrinho.db')
        cursor = conn.cursor()
        
        # 1. Salvar a Rota
        data_atual = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        cursor.execute("INSERT INTO rotas (nome, data_criacao) VALUES (?, ?)", (nome_rota, data_atual))
        rota_id = cursor.lastrowid # Pega o ID gerado
        
        # 2. Salvar os Comandos
        for i, cmd in enumerate(comandos):
            parts = cmd.split()
            acao = parts[0]
            val = float(parts[1]) if len(parts) > 1 else 0.0
            
            cursor.execute("INSERT INTO comandos (rota_id, ordem, comando, valor) VALUES (?, ?, ?, ?)", 
                           (rota_id, i, acao, val))
            
        conn.commit()
        conn.close()
        
        status_label.config(text=f"Sucesso: Rota '{nome_rota}' salva!")
        nome_rota_entry.delete(0, tk.END) # Limpa o campo após salvar
        
    except Exception as e:
        status_label.config(text=f"Erro ao salvar no BD: {e}")

# =================================================================
# FUNÇÕES DE LÓGICA E SIMULAÇÃO
# =================================================================
def simular_movimento(cmd_str):
    """Calcula a nova posição do robô virtual baseado no comando enviado"""
    global robot_x, robot_y, robot_theta
    
    parts = cmd_str.split()
    tipo = parts[0].upper()
    
    valor = 0
    if len(parts) > 1:
        try:
            valor = float(parts[1])
        except ValueError:
            valor = 0

    rad = math.radians(robot_theta)

    if tipo == "FRENTE":
        robot_x += valor * math.cos(rad)
        robot_y += valor * math.sin(rad)
    elif tipo == "TRAS":
        robot_x -= valor * math.cos(rad)
        robot_y -= valor * math.sin(rad)
    elif tipo == "ESQUERDA":
        robot_theta += valor
    elif tipo == "DIREITA":
        robot_theta -= valor
    
    robot_theta = robot_theta % 360

# =================================================================
# FUNÇÕES DE COMUNICAÇÃO SERIAL
# =================================================================
def connect_serial():
    global ser
    port = port_combobox.get()
    if not port:
        status_label.config(text="Erro: Nenhuma porta selecionada")
        return
    try:
        # ATENÇÃO: Baud rate 9600 para Arduino padrão
        ser = serial.Serial(port, 9600, timeout=1)
        time.sleep(2) # Espera o Arduino reiniciar
        status_label.config(text=f"Conectado a {port}")
        connect_button.config(state="disabled")
        disconnect_button.config(state="normal")
        
        # Inicia thread de leitura
        threading.Thread(target=read_from_serial, daemon=True).start()
    except serial.SerialException as e:
        status_label.config(text=f"Erro ao conectar: {e}")

def disconnect_serial():
    global ser
    if ser and ser.is_open:
        ser.close()
        status_label.config(text="Desconectado")
        connect_button.config(state="normal")
        disconnect_button.config(state="disabled")

def read_from_serial():
    """Lê o retorno do Arduino apenas para mostrar no terminal/debug"""
    while ser and ser.is_open:
        try:
            line = ser.readline().decode('utf-8').strip()
            if line:
                print(f"[Arduino]: {line}")
        except:
            pass

# =================================================================
# FUNÇÕES DA GUI
# =================================================================
def update_gui():
    canvas_x = (CANVAS_WIDTH / 2) + (robot_x * SCALE)
    canvas_y = (CANVAS_HEIGHT / 2) - (robot_y * SCALE) 

    if len(path_points) == 0 or (path_points[-1][0] != canvas_x or path_points[-1][1] != canvas_y):
       path_points.append((canvas_x, canvas_y))

    canvas.delete("all")
    
    # Grade
    canvas.create_line(CANVAS_WIDTH/2, 0, CANVAS_WIDTH/2, CANVAS_HEIGHT, fill="#ddd")
    canvas.create_line(0, CANVAS_HEIGHT/2, CANVAS_WIDTH, CANVAS_HEIGHT/2, fill="#ddd")

    # Caminho
    if len(path_points) > 1:
        canvas.create_line(path_points, fill="blue", width=2)
    
    # Robô
    canvas.create_oval(canvas_x - ROBOT_RADIUS, canvas_y - ROBOT_RADIUS,
                       canvas_x + ROBOT_RADIUS, canvas_y + ROBOT_RADIUS,
                       fill="red", outline="black")
    
    # Orientação
    end_x = canvas_x + ROBOT_RADIUS * 1.5 * math.cos(math.radians(robot_theta))
    end_y = canvas_y - ROBOT_RADIUS * 1.5 * math.sin(math.radians(robot_theta))
    canvas.create_line(canvas_x, canvas_y, end_x, end_y, fill="black", width=3)

def add_command():
    cmd_type = cmd_type_var.get()
    value = cmd_value_entry.get()
    
    if (value.replace('.','',1).isdigit()) or cmd_type == "ENTREGAR":
        if cmd_type == "ENTREGAR":
            command = "ENTREGAR"
        else:
            command = f"{cmd_type} {value}"
        command_listbox.insert(tk.END, command)
    else:
        status_label.config(text="Erro: valor inválido")

def clear_commands():
    command_listbox.delete(0, tk.END)

def send_commands():
    if not ser or not ser.is_open:
        status_label.config(text="Erro: Não conectado!")
        return
    
    commands = command_listbox.get(0, tk.END)
    status_label.config(text="Executando fila...")
    
    def run_queue():
        for cmd in commands:
            # --- TRADUÇÃO DE PROTOCOLO (Python -> Arduino) ---
            parts = cmd.split()
            acao = parts[0]
            val = parts[1] if len(parts) > 1 else "0"
            
            cmd_arduino = ""
            tempo_espera = 1.0 

            if acao == "FRENTE":
                cmd_arduino = f"F({val})\\"
                tempo_espera = (float(val) * 0.05) + 0.5 
            elif acao == "TRAS":
                cmd_arduino = f"T({val})\\"
                tempo_espera = (float(val) * 0.05) + 0.5
            elif acao == "DIREITA":
                cmd_arduino = f"D({val})\\"
                tempo_espera = 1.0
            elif acao == "ESQUERDA":
                cmd_arduino = f"E({val})\\"
                tempo_espera = 1.0
            elif acao == "ENTREGAR":
                cmd_arduino = "S(90)\\" 
                tempo_espera = 1.5

            # 1. Envia para o Arduino Real
            if cmd_arduino:
                print(f"Enviando Serial: {cmd_arduino}")
                ser.write(cmd_arduino.encode('utf-8'))
            
            # 2. Simula na Interface Virtual
            simular_movimento(cmd)
            
            # 3. Atualiza a tela (thread-safe para Tkinter)
            root.after(0, update_gui)
            
            # 4. Pausa para o robô físico ter tempo de executar
            time.sleep(tempo_espera) 
            
        status_label.config(text=f"Concluído: {len(commands)} comandos")

    threading.Thread(target=run_queue, daemon=True).start()

def reset_path():
    global path_points, robot_x, robot_y, robot_theta
    path_points = []
    robot_x, robot_y, robot_theta = 0, 0, 0
    update_gui()

# =================================================================
# SETUP DA JANELA PRINCIPAL
# =================================================================
# Inicializa o Banco de Dados antes de abrir a janela
init_db()

root = tk.Tk()
root.title("OncoMap / Carrinho Control (Com Banco de Dados)")

main_frame = ttk.Frame(root, padding="10")
main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

# --- Canvas (Esquerda) ---
canvas = tk.Canvas(main_frame, width=CANVAS_WIDTH, height=CANVAS_HEIGHT, bg="white", relief="sunken", borderwidth=1)
canvas.grid(row=0, column=0, rowspan=10, padx=5, pady=5)

# --- Controles de Conexão (Direita Topo) ---
controls_frame = ttk.LabelFrame(main_frame, text="Conexão", padding="10")
controls_frame.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N), padx=5, pady=5)

port_label = ttk.Label(controls_frame, text="Porta Serial:")
port_label.grid(row=0, column=0, sticky=tk.W)
try:
    ports = [port.device for port in serial.tools.list_ports.comports()]
except:
    ports = []
port_combobox = ttk.Combobox(controls_frame, values=ports)
port_combobox.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E))
if ports: port_combobox.current(0)

connect_button = ttk.Button(controls_frame, text="Conectar", command=connect_serial)
connect_button.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=5)
disconnect_button = ttk.Button(controls_frame, text="Desconectar", command=disconnect_serial, state="disabled")
disconnect_button.grid(row=2, column=1, sticky=(tk.W, tk.E), pady=5)
status_label = ttk.Label(controls_frame, text="Desconectado", relief="sunken", anchor=tk.W)
status_label.grid(row=3, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)

# --- Criação de Comandos (Direita Meio) ---
cmd_frame = ttk.LabelFrame(main_frame, text="Criar Rota", padding="10")
cmd_frame.grid(row=1, column=1, sticky=(tk.W, tk.E, tk.N), padx=5, pady=5)

cmd_type_var = tk.StringVar(value="FRENTE")
cmd_types = ["FRENTE", "TRAS", "ESQUERDA", "DIREITA", "ENTREGAR"]
cmd_type_menu = ttk.OptionMenu(cmd_frame, cmd_type_var, cmd_types[0], *cmd_types)
cmd_type_menu.grid(row=0, column=0, sticky=(tk.W, tk.E))

cmd_value_entry = ttk.Entry(cmd_frame, width=10)
cmd_value_entry.grid(row=0, column=1, padx=5)
cmd_value_entry.insert(0, "10") 

add_cmd_button = ttk.Button(cmd_frame, text="Adicionar", command=add_command)
add_cmd_button.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)

# --- Fila e Banco de Dados (Direita Baixo) ---
queue_frame = ttk.LabelFrame(main_frame, text="Fila e Banco de Dados", padding="10")
queue_frame.grid(row=2, column=1, sticky=(tk.W, tk.E, tk.N, tk.S))

command_listbox = tk.Listbox(queue_frame, height=10)
command_listbox.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S))

# Botões de Controle da Fila
send_button = ttk.Button(queue_frame, text="ENVIAR E RODAR", command=send_commands)
send_button.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=2)
clear_button = ttk.Button(queue_frame, text="Limpar Lista", command=clear_commands)
clear_button.grid(row=1, column=1, sticky=(tk.W, tk.E), pady=2)

# Separador Visual
ttk.Separator(queue_frame, orient='horizontal').grid(row=2, column=0, columnspan=2, sticky='ew', pady=5)

# Controles do Banco de Dados
ttk.Label(queue_frame, text="Nome da Rota:").grid(row=3, column=0, sticky=tk.W)
nome_rota_entry = ttk.Entry(queue_frame)
nome_rota_entry.grid(row=3, column=1, sticky=(tk.W, tk.E), pady=2)

save_db_button = ttk.Button(queue_frame, text="Salvar no Histórico", command=salvar_rota_db)
save_db_button.grid(row=4, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)

reset_path_button = ttk.Button(queue_frame, text="Resetar Mapa (Tela)", command=reset_path)
reset_path_button.grid(row=5, column=0, columnspan=2, sticky=(tk.W, tk.E))

# --- Inicialização ---
update_gui()
root.mainloop()

# Fecha serial ao sair se estiver aberta
if ser and ser.is_open:
    ser.close()