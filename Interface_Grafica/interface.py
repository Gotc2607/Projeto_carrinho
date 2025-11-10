# =================================================================
# == INTERFACE GRÁFICA PARA CONTROLE E VISUALIZAÇÃO DO CARRINHO  ==
# =================================================================
import tkinter as tk
from tkinter import ttk
import math  # <--- LINHA ADICIONADA
import serial
import serial.tools.list_ports
import threading
import time

# --- Constantes da GUI ---
CANVAS_WIDTH = 600
CANVAS_HEIGHT = 600
SCALE = 10  # 10 pixels = 1 cm
ROBOT_RADIUS = 1.5 * SCALE # Raio do ícone do robô em pixels

# --- Variáveis Globais ---
ser = None
robot_x, robot_y, robot_theta = 0, 0, 0
path_points = []

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
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2) # Espera o Arduino reiniciar
        status_label.config(text=f"Conectado a {port}")
        connect_button.config(state="disabled")
        disconnect_button.config(state="normal")
        # Inicia a thread para ler dados da serial
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
    global robot_x, robot_y, robot_theta
    while ser and ser.is_open:
        try:
            line = ser.readline().decode('utf-8').strip()
            if line.startswith("T;"): # Checa se é um pacote de telemetria
                parts = line.split(';')
                if len(parts) == 4:
                    _, x, y, theta = parts
                    robot_x = float(x)
                    robot_y = float(y)
                    robot_theta = float(theta)
        except:
            # Ignora erros de decodificação ou desconexão
            pass

# =================================================================
# FUNÇÕES DA GUI
# =================================================================
def update_gui():
    # Converte coordenadas do robô (cm) para coordenadas do canvas (pixels)
    # A origem do robô é (0,0), a do canvas é no centro (width/2, height/2)
    canvas_x = (CANVAS_WIDTH / 2) + (robot_x * SCALE)
    canvas_y = (CANVAS_HEIGHT / 2) - (robot_y * SCALE) # Y é invertido no canvas

    # Adiciona o ponto atual à lista do caminho
    if len(path_points) == 0 or (path_points[-1][0] != canvas_x or path_points[-1][1] != canvas_y):
       path_points.append((canvas_x, canvas_y))

    # Limpa o canvas e redesenha tudo
    canvas.delete("all")
    
    # Desenha o caminho percorrido
    if len(path_points) > 1:
        canvas.create_line(path_points, fill="blue", width=2)
    
    # Desenha o robô (um círculo com uma linha para a orientação)
    canvas.create_oval(canvas_x - ROBOT_RADIUS, canvas_y - ROBOT_RADIUS,
                       canvas_x + ROBOT_RADIUS, canvas_y + ROBOT_RADIUS,
                       fill="red", outline="black")
    
    # Linha de orientação - CORRIGIDO: usando math em vez de tk.math
    end_x = canvas_x + ROBOT_RADIUS * 1.5 * math.cos(math.radians(-robot_theta))
    end_y = canvas_y + ROBOT_RADIUS * 1.5 * math.sin(math.radians(-robot_theta))
    canvas.create_line(canvas_x, canvas_y, end_x, end_y, fill="black", width=2)

    # Agenda a próxima atualização
    root.after(50, update_gui) # Atualiza a GUI a cada 50ms (20 FPS)

def add_command():
    cmd_type = cmd_type_var.get()
    value = cmd_value_entry.get()
    if value.isnumeric() or cmd_type == "ENTREGAR":
        if cmd_type == "ENTREGAR":
            command = "ENTREGAR"
        else:
            command = f"{cmd_type} {value}"
        command_listbox.insert(tk.END, command)
    else:
        status_label.config(text="Erro: valor deve ser um número")

def clear_commands():
    command_listbox.delete(0, tk.END)

def send_commands():
    if not ser or not ser.is_open:
        status_label.config(text="Erro: Carrinho não conectado")
        return
    
    commands = command_listbox.get(0, tk.END)
    for cmd in commands:
        ser.write((cmd + '\n').encode('utf-8'))
        time.sleep(0.1) # Pequena pausa entre comandos
    status_label.config(text=f"Enviado {len(commands)} comandos")

def reset_path():
    global path_points, robot_x, robot_y, robot_theta
    path_points = []
    # Opcional: Enviar comando de reset de odometria para o Arduino
    # if ser and ser.is_open:
    #     ser.write('RESET\n'.encode('utf-8'))
    # robot_x, robot_y, robot_theta = 0,0,0

# =================================================================
# SETUP DA JANELA PRINCIPAL
# =================================================================
root = tk.Tk()
root.title("Controlador do Carrinho Robótico")

main_frame = ttk.Frame(root, padding="10")
main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

# --- Lado Esquerdo: Canvas ---
canvas = tk.Canvas(main_frame, width=CANVAS_WIDTH, height=CANVAS_HEIGHT, bg="white", relief="sunken", borderwidth=1)
canvas.grid(row=0, column=0, rowspan=10, padx=5, pady=5)

# --- Lado Direito: Controles ---
controls_frame = ttk.LabelFrame(main_frame, text="Controles", padding="10")
controls_frame.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N), padx=5, pady=5)

# Conexão Serial
port_label = ttk.Label(controls_frame, text="Porta Serial:")
port_label.grid(row=0, column=0, sticky=tk.W)
ports = [port.device for port in serial.tools.list_ports.comports()]
port_combobox = ttk.Combobox(controls_frame, values=ports)
port_combobox.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E))
if ports:
    port_combobox.current(0)

connect_button = ttk.Button(controls_frame, text="Conectar", command=connect_serial)
connect_button.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=5)
disconnect_button = ttk.Button(controls_frame, text="Desconectar", command=disconnect_serial, state="disabled")
disconnect_button.grid(row=2, column=1, sticky=(tk.W, tk.E), pady=5)

status_label = ttk.Label(controls_frame, text="Desconectado", relief="sunken", anchor=tk.W)
status_label.grid(row=3, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)


# Montador de Comandos
cmd_frame = ttk.LabelFrame(main_frame, text="Montar Comandos", padding="10")
cmd_frame.grid(row=1, column=1, sticky=(tk.W, tk.E, tk.N), padx=5, pady=5)

cmd_type_var = tk.StringVar(value="FRENTE")
cmd_types = ["FRENTE", "TRAS", "ESQUERDA", "DIREITA", "ENTREGAR"]
cmd_type_menu = ttk.OptionMenu(cmd_frame, cmd_type_var, cmd_types[0], *cmd_types)
cmd_type_menu.grid(row=0, column=0, sticky=(tk.W, tk.E))

cmd_value_entry = ttk.Entry(cmd_frame, width=10)
cmd_value_entry.grid(row=0, column=1)

add_cmd_button = ttk.Button(cmd_frame, text="Adicionar à Fila", command=add_command)
add_cmd_button.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)

# Fila de Comandos
queue_frame = ttk.LabelFrame(main_frame, text="Fila de Execução", padding="10")
queue_frame.grid(row=2, column=1, sticky=(tk.W, tk.E, tk.N, tk.S))

command_listbox = tk.Listbox(queue_frame, height=10)
command_listbox.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S))

send_button = ttk.Button(queue_frame, text="Enviar e Executar", command=send_commands)
send_button.grid(row=1, column=0, sticky=(tk.W, tk.E), pady=5)
clear_button = ttk.Button(queue_frame, text="Limpar Fila", command=clear_commands)
clear_button.grid(row=1, column=1, sticky=(tk.W, tk.E), pady=5)
reset_path_button = ttk.Button(queue_frame, text="Resetar Caminho", command=reset_path)
reset_path_button.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E))


# --- INICIALIZAÇÃO ---
update_gui() # Inicia o loop de atualização da GUI
root.mainloop()

# Garante que a porta serial seja fechada ao sair
if ser and ser.is_open:
    ser.close()