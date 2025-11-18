# =================================================================
# == INTERFACE GRÁFICA PARA CONTROLE E VISUALIZAÇÃO DO CARRINHO  ==
# =================================================================
import tkinter as tk
from tkinter import ttk
import math
import serial
import serial.tools.list_ports
import threading
import time

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
# FUNÇÕES DE LÓGICA E SIMULAÇÃO (NOVO)
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

    # Converte o ângulo atual para radianos para usar no seno/cosseno
    # Nota: Na matemática padrão, 0 é direita, 90 é cima.
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
    
    # Mantém o ângulo entre 0 e 360 (opcional, mas bom para limpeza)
    robot_theta = robot_theta % 360

# =================================================================
# FUNÇÕES DE COMUNICAÇÃO
# =================================================================
def connect_serial():
    global ser
    port = port_combobox.get()
    if not port:
        status_label.config(text="Erro: Nenhuma porta selecionada")
        return
    try:
        # ATENÇÃO: Baud rate ajustado para 115200 para bater com o Arduino novo
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2) # Espera o Arduino reiniciar
        status_label.config(text=f"Conectado a {port}")
        connect_button.config(state="disabled")
        disconnect_button.config(state="normal")
        
        # Inicia a thread para ler respostas do Arduino (Debug)
        thread = threading.Thread(target=read_from_serial, daemon=True)
        thread.start()
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
    # Converte coordenadas do robô (cm) para coordenadas do canvas (pixels)
    # Origem (0,0) do robô será o CENTRO da tela
    canvas_x = (CANVAS_WIDTH / 2) + (robot_x * SCALE)
    canvas_y = (CANVAS_HEIGHT / 2) - (robot_y * SCALE) # Y invertido no Tkinter

    # Adiciona ponto ao rastro se o robô se moveu
    if len(path_points) == 0 or (path_points[-1][0] != canvas_x or path_points[-1][1] != canvas_y):
       path_points.append((canvas_x, canvas_y))

    canvas.delete("all")
    
    # Desenha grade (opcional, para referência)
    canvas.create_line(CANVAS_WIDTH/2, 0, CANVAS_WIDTH/2, CANVAS_HEIGHT, fill="#ddd")
    canvas.create_line(0, CANVAS_HEIGHT/2, CANVAS_WIDTH, CANVAS_HEIGHT/2, fill="#ddd")

    # Desenha o caminho
    if len(path_points) > 1:
        canvas.create_line(path_points, fill="blue", width=2)
    
    # Desenha o robô
    canvas.create_oval(canvas_x - ROBOT_RADIUS, canvas_y - ROBOT_RADIUS,
                       canvas_x + ROBOT_RADIUS, canvas_y + ROBOT_RADIUS,
                       fill="red", outline="black")
    
    # Linha de orientação (Nariz do robô)
    # Como o Y do canvas cresce para baixo, invertemos o seno visualmente
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
    
    # Roda em uma thread separada para não travar a tela enquanto espera
    def run_queue():
        for cmd in commands:
            # 1. Envia para o Arduino Real
            ser.write((cmd + '\n').encode('utf-8'))
            
            # 2. Simula na Interface Virtual
            simular_movimento(cmd)
            
            # 3. Atualiza a tela
            root.after(0, update_gui)
            
            # Pausa para dar efeito de animação e tempo para o robô físico andar
            # Ajuste este tempo se o robô real for mais lento
            time.sleep(0.8) 
            
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
root = tk.Tk()
root.title("OncoMap / Carrinho Control (Simulado)")

main_frame = ttk.Frame(root, padding="10")
main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

# --- Canvas ---
canvas = tk.Canvas(main_frame, width=CANVAS_WIDTH, height=CANVAS_HEIGHT, bg="white", relief="sunken", borderwidth=1)
canvas.grid(row=0, column=0, rowspan=10, padx=5, pady=5)

# --- Controles ---
controls_frame = ttk.LabelFrame(main_frame, text="Conexão", padding="10")
controls_frame.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N), padx=5, pady=5)

port_label = ttk.Label(controls_frame, text="Porta Serial:")
port_label.grid(row=0, column=0, sticky=tk.W)
ports = [port.device for port in serial.tools.list_ports.comports()]
port_combobox = ttk.Combobox(controls_frame, values=ports)
port_combobox.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E))
if ports: port_combobox.current(0)

connect_button = ttk.Button(controls_frame, text="Conectar", command=connect_serial)
connect_button.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=5)
disconnect_button = ttk.Button(controls_frame, text="Desconectar", command=disconnect_serial, state="disabled")
disconnect_button.grid(row=2, column=1, sticky=(tk.W, tk.E), pady=5)
status_label = ttk.Label(controls_frame, text="Desconectado", relief="sunken", anchor=tk.W)
status_label.grid(row=3, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)

# --- Comandos ---
cmd_frame = ttk.LabelFrame(main_frame, text="Criar Rota", padding="10")
cmd_frame.grid(row=1, column=1, sticky=(tk.W, tk.E, tk.N), padx=5, pady=5)

cmd_type_var = tk.StringVar(value="FRENTE")
cmd_types = ["FRENTE", "TRAS", "ESQUERDA", "DIREITA", "ENTREGAR"]
cmd_type_menu = ttk.OptionMenu(cmd_frame, cmd_type_var, cmd_types[0], *cmd_types)
cmd_type_menu.grid(row=0, column=0, sticky=(tk.W, tk.E))

cmd_value_entry = ttk.Entry(cmd_frame, width=10)
cmd_value_entry.grid(row=0, column=1, padx=5)
cmd_value_entry.insert(0, "10") # Valor padrão

add_cmd_button = ttk.Button(cmd_frame, text="Adicionar", command=add_command)
add_cmd_button.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)

# --- Fila ---
queue_frame = ttk.LabelFrame(main_frame, text="Fila de Execução", padding="10")
queue_frame.grid(row=2, column=1, sticky=(tk.W, tk.E, tk.N, tk.S))

command_listbox = tk.Listbox(queue_frame, height=15)
command_listbox.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S))

send_button = ttk.Button(queue_frame, text="ENVIAR E RODAR", command=send_commands)
send_button.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=5)
clear_button = ttk.Button(queue_frame, text="Limpar", command=clear_commands)
clear_button.grid(row=1, column=1, sticky=(tk.W, tk.E), pady=5)
reset_path_button = ttk.Button(queue_frame, text="Resetar Mapa", command=reset_path)
reset_path_button.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E))

# --- Inicialização ---
update_gui()
root.mainloop()

if ser and ser.is_open:
    ser.close()