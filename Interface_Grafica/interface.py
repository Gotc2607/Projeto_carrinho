# =================================================================
# == EGG0-1 - CONTROLE, VISUALIZAÇÃO E HISTÓRICO (SUPABASE)     ==
# =================================================================
import tkinter as tk
from tkinter import ttk, messagebox
import math
import serial
import serial.tools.list_ports
import threading
import time
from datetime import datetime
from supabase import create_client, Client

# --- CONFIGURAÇÃO DO SUPABASE ---
# !!! SUBSTITUA PELAS SUAS CHAVES DO PAINEL DO SUPABASE !!!
SUPABASE_URL = "https://xdigdkwgnqxsnarvpqxu.supabase.co"
SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InhkaWdka3dnbnF4c25hcnZwcXh1Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NjQ1NzAzMTQsImV4cCI6MjA4MDE0NjMxNH0.DdLkpy5oQaWEPGQJeu4Dxg6Hzd3oHlv9hXhIBqureo8"

supabase: Client = None

# --- Constantes da GUI ---
CANVAS_WIDTH = 600 
CANVAS_HEIGHT = 600
SCALE = 0.7
ROBOT_RADIUS = 10 * SCALE 

# --- Variáveis Globais ---
ser = None
robot_x, robot_y, robot_theta = 0, 0, 0 
path_points = []
cache_rotas_historico = [] # Armazena os dados crus vindos do banco para consulta

# =================================================================
# FUNÇÕES DE BANCO DE DADOS
# =================================================================
def init_db():
    global supabase
    try:
        if "SUA_URL" in SUPABASE_URL:
            print("AVISO: Configure o Supabase no código!")
            return
        supabase = create_client(SUPABASE_URL, SUPABASE_KEY)
        print("Supabase conectado.")
    except Exception as e:
        print(f"Erro Supabase: {e}")

def salvar_rota_db():
    nome = nome_rota_entry.get()
    cmds = command_listbox.get(0, tk.END)
    
    if not nome or not cmds:
        status_label.config(text="Erro: Nome ou comandos vazios")
        return
    
    status_label.config(text="Salvando...")
    
    def _thread_save():
        try:
            # 1. Cria Rota
            res = supabase.table("rotas").insert({
                "nome": nome, 
                "data_criacao": datetime.now().isoformat()
            }).execute()
            new_id = res.data[0]['id']
            
            # 2. Cria Comandos
            payload = []
            for i, c in enumerate(cmds):
                p = c.split()
                val = float(p[1]) if len(p)>1 else 0.0
                payload.append({"rota_id": new_id, "ordem": i, "comando": p[0], "valor": val})
            
            supabase.table("comandos").insert(payload).execute()
            
            root.after(0, lambda: status_label.config(text="Salvo com sucesso!"))
            root.after(0, atualizar_historico) # Atualiza a lista na outra aba
        except Exception as e:
            root.after(0, lambda: status_label.config(text=f"Erro: {str(e)[:30]}"))

    threading.Thread(target=_thread_save, daemon=True).start()

def atualizar_historico():
    """Baixa a lista de rotas do Supabase e preenche a Listbox do Histórico"""
    history_status.config(text="Atualizando lista...")
    
    def _thread_list():
        global cache_rotas_historico
        try:
            # Pega as ultimas 50 rotas
            res = supabase.table("rotas").select("*").order("data_criacao", desc=True).limit(50).execute()
            cache_rotas_historico = res.data
            
            def _update_ui_list():
                history_listbox.delete(0, tk.END)
                for r in cache_rotas_historico:
                    # Formata a data para ficar bonita (DD/MM HH:MM)
                    data_raw = r['data_criacao'].split('.')[0].replace('T', ' ')
                    history_listbox.insert(tk.END, f"{r['nome']}  |  {data_raw}")
                history_status.config(text="Lista atualizada.")
            
            root.after(0, _update_ui_list)
        except Exception as e:
            root.after(0, lambda: history_status.config(text=f"Erro: {e}"))

    threading.Thread(target=_thread_list, daemon=True).start()

def carregar_rota_selecionada():
    """Pega a rota clicada na aba Histórico e joga na aba Controle"""
    selection = history_listbox.curselection()
    if not selection:
        return
    
    index = selection[0]
    rota_dados = cache_rotas_historico[index]
    rota_id = rota_dados['id']
    
    history_status.config(text=f"Carregando ID {rota_id}...")
    
    def _thread_load():
        try:
            # Busca comandos daquela rota
            res = supabase.table("comandos").select("*").eq("rota_id", rota_id).order("ordem").execute()
            cmds_db = res.data
            
            def _aplicar_na_tela():
                # 1. Limpa tudo atual
                reset_path()
                clear_commands()
                nome_rota_entry.delete(0, tk.END)
                nome_rota_entry.insert(0, rota_dados['nome'])
                
                # 2. Preenche a lista e desenha o mapa
                for item in cmds_db:
                    cmd_str = item['comando']
                    if item['comando'] != "ENTREGAR":
                        cmd_str += f" {item['valor']}"
                    
                    command_listbox.insert(tk.END, cmd_str)
                    simular_movimento(cmd_str) # Desenha no canvas
                    update_gui() # Atualiza visual
                
                # 3. Muda para a aba de controle automaticamente
                tabs.select(tab_controle)
                history_status.config(text="Rota carregada!")
                status_label.config(text=f"Rota '{rota_dados['nome']}' carregada.")

            root.after(0, _aplicar_na_tela)
            
        except Exception as e:
             root.after(0, lambda: history_status.config(text=f"Erro load: {e}"))

    threading.Thread(target=_thread_load, daemon=True).start()

# =================================================================
# FUNÇÕES LÓGICA / SERIAL / VISUAL
# =================================================================
def simular_movimento(cmd_str):
    global robot_x, robot_y, robot_theta
    parts = cmd_str.split()
    tipo = parts[0].upper()
    val = float(parts[1]) if len(parts) > 1 else 0
    rad = math.radians(robot_theta)
    
    if tipo == "FRENTE":
        robot_x += val * math.cos(rad); robot_y += val * math.sin(rad)
    elif tipo == "TRAS":
        robot_x -= val * math.cos(rad); robot_y -= val * math.sin(rad)
    elif tipo == "ESQUERDA": robot_theta += val
    elif tipo == "DIREITA": robot_theta -= val
    robot_theta %= 360

def update_gui():
    cx = (CANVAS_WIDTH/2) + (robot_x * SCALE)
    cy = (CANVAS_HEIGHT/2) - (robot_y * SCALE)
    if not path_points or (path_points[-1] != (cx, cy)): path_points.append((cx, cy))
    
    canvas.delete("all")
    canvas.create_line(CANVAS_WIDTH/2, 0, CANVAS_WIDTH/2, CANVAS_HEIGHT, fill="#eee")
    canvas.create_line(0, CANVAS_HEIGHT/2, CANVAS_WIDTH, CANVAS_HEIGHT/2, fill="#eee")
    if len(path_points) > 1: canvas.create_line(path_points, fill="blue", width=2)
    
    canvas.create_oval(cx-ROBOT_RADIUS, cy-ROBOT_RADIUS, cx+ROBOT_RADIUS, cy+ROBOT_RADIUS, fill="red")
    ex = cx + ROBOT_RADIUS*1.5 * math.cos(math.radians(robot_theta))
    ey = cy - ROBOT_RADIUS*1.5 * math.sin(math.radians(robot_theta))
    canvas.create_line(cx, cy, ex, ey, fill="black", width=2)

def connect_serial():
    global ser
    try:
        ser = serial.Serial(port_combobox.get(), 9600, timeout=1)
        time.sleep(2)
        status_label.config(text="Conectado!")
        threading.Thread(target=read_serial, daemon=True).start()
    except Exception as e: status_label.config(text=str(e))

def read_serial():
    while ser and ser.is_open:
        try: 
            l = ser.readline().decode().strip()
            if l: print(f"Arduino: {l}")
        except: pass

def send_commands():
    cmds = command_listbox.get(0, tk.END)
    def _run():
        for c in cmds:
            simular_movimento(c)
            root.after(0, update_gui)
            # Envio serial simplificado para o exemplo
            if ser and ser.is_open: 
                p = c.split()
                # Conversão simples ex: FRENTE 10 -> F(10)\
                code = p[0][0] + (f"({p[1]})" if len(p)>1 else "(0)") + "\\" 
                ser.write(code.encode())
            time.sleep(0.5) 
    threading.Thread(target=_run, daemon=True).start()

def add_cmd():
    v = cmd_val.get()
    t = cmd_type.get()
    command_listbox.insert(tk.END, f"{t} {v}" if t != "ENTREGAR" else t)

def clear_commands(): command_listbox.delete(0, tk.END)
def reset_path(): 
    global path_points, robot_x, robot_y, robot_theta
    path_points.clear(); robot_x=0; robot_y=0; robot_theta=0
    update_gui()

# =================================================================
# INTERFACE GRÁFICA (SETUP)
# =================================================================
init_db()
root = tk.Tk()
root.title("EGG0-1 Control")

# Layout Principal: Esquerda (Mapa) | Direita (Abas)
main_frame = ttk.Frame(root, padding=5)
main_frame.grid(row=0, column=0, sticky="nsew")

# --- LADO ESQUERDO: MAPA ---
canvas = tk.Canvas(main_frame, width=CANVAS_WIDTH, height=CANVAS_HEIGHT, bg="white", relief="sunken")
canvas.grid(row=0, column=0, rowspan=2, padx=5, pady=5)

# --- LADO DIREITO: SISTEMA DE ABAS ---
right_panel = ttk.Frame(main_frame)
right_panel.grid(row=0, column=1, sticky="ns")

tabs = ttk.Notebook(right_panel)
tabs.grid(row=0, column=0, sticky="nsew")

# === ABA 1: CONTROLE (O que já existia) ===
tab_controle = ttk.Frame(tabs, padding=10)
tabs.add(tab_controle, text="Controle & Criação")

# Conexão
lf_conn = ttk.LabelFrame(tab_controle, text="Conexão")
lf_conn.pack(fill="x", pady=5)
try: ports = [p.device for p in serial.tools.list_ports.comports()]
except: ports = []
port_combobox = ttk.Combobox(lf_conn, values=ports); 
if ports: port_combobox.current(0)
port_combobox.pack(fill="x", padx=5, pady=2)
ttk.Button(lf_conn, text="Conectar", command=connect_serial).pack(fill="x", padx=5)
status_label = ttk.Label(lf_conn, text="Offline", foreground="blue")
status_label.pack(anchor="w", padx=5)

# Comandos
lf_cmds = ttk.LabelFrame(tab_controle, text="Adicionar Comandos")
lf_cmds.pack(fill="x", pady=5)
cmd_type = tk.StringVar(value="FRENTE")
ttk.OptionMenu(lf_cmds, cmd_type, "FRENTE", "FRENTE", "TRAS", "DIREITA", "ESQUERDA", "ENTREGAR").pack(fill="x")
cmd_val = ttk.Entry(lf_cmds); cmd_val.insert(0, "10"); cmd_val.pack(fill="x", pady=2)
ttk.Button(lf_cmds, text="Adicionar", command=add_cmd).pack(fill="x")

# Fila
lf_queue = ttk.LabelFrame(tab_controle, text="Fila Atual")
lf_queue.pack(fill="both", expand=True, pady=5)
command_listbox = tk.Listbox(lf_queue, height=8)
command_listbox.pack(fill="both", expand=True, padx=5, pady=2)
ttk.Button(lf_queue, text="Limpar", command=clear_commands).pack(side="left", padx=2)
ttk.Button(lf_queue, text="Executar", command=send_commands).pack(side="right", padx=2)

# Salvar
lf_save = ttk.LabelFrame(tab_controle, text="Salvar Rota")
lf_save.pack(fill="x", pady=5)
ttk.Label(lf_save, text="Nome:").pack(anchor="w")
nome_rota_entry = ttk.Entry(lf_save)
nome_rota_entry.pack(fill="x", padx=5)
ttk.Button(lf_save, text="Salvar na Nuvem", command=salvar_rota_db).pack(fill="x", padx=5, pady=5)
ttk.Button(lf_save, text="Resetar Mapa", command=reset_path).pack(fill="x", padx=5)


# === ABA 2: HISTÓRICO (NOVIDADE) ===
tab_historico = ttk.Frame(tabs, padding=10)
tabs.add(tab_historico, text="Histórico (DB)")

ttk.Label(tab_historico, text="Rotas Salvas no Supabase:").pack(anchor="w")

# Lista de Histórico
history_listbox = tk.Listbox(tab_historico, height=20)
history_listbox.pack(fill="both", expand=True, pady=5)

# Botões do Histórico
btn_frame_hist = ttk.Frame(tab_historico)
btn_frame_hist.pack(fill="x")

ttk.Button(btn_frame_hist, text="Atualizar Lista", command=atualizar_historico).pack(fill="x", pady=2)
ttk.Button(btn_frame_hist, text="CARREGAR ROTA SELECIONADA", command=carregar_rota_selecionada).pack(fill="x", pady=5)

history_status = ttk.Label(tab_historico, text="...", font=("Arial", 8))
history_status.pack(anchor="w")

# Inicializa visual
update_gui()
root.mainloop()