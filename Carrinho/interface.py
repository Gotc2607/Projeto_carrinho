# =================================================================
# == EGG0-1 - INTERFACE COM ANIMAÇÃO DE ENTREGA (VISUAL OVO)    ==
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
ovos_entregues = [] # Lista de coordenadas onde ovos foram deixados
cache_rotas_historico = []

# =================================================================
# SUPABASE
# =================================================================
def init_db():
    global supabase
    try:
        supabase = create_client(SUPABASE_URL, SUPABASE_KEY)
        print("Supabase conectado.")
    except Exception as e: print(f"Erro Supabase: {e}")

def salvar_rota_db():
    nome = nome_rota_entry.get()
    cmds = command_listbox.get(0, tk.END)
    if not nome or not cmds: return
    status_label.config(text="Salvando...")
    def _thread_save():
        try:
            res = supabase.table("rotas").insert({"nome": nome, "data_criacao": datetime.now().isoformat()}).execute()
            new_id = res.data[0]['id']
            payload = []
            for i, c in enumerate(cmds):
                p = c.split(); val = float(p[1]) if len(p)>1 else 0.0
                payload.append({"rota_id": new_id, "ordem": i, "comando": p[0], "valor": val})
            supabase.table("comandos").insert(payload).execute()
            root.after(0, lambda: status_label.config(text="Salvo!"))
            root.after(0, atualizar_historico)
        except Exception as e: root.after(0, lambda: status_label.config(text=f"Erro: {str(e)[:15]}"))
    threading.Thread(target=_thread_save, daemon=True).start()

def atualizar_historico():
    def _thread_list():
        global cache_rotas_historico
        try:
            res = supabase.table("rotas").select("*").order("data_criacao", desc=True).limit(50).execute()
            cache_rotas_historico = res.data
            def _update_ui():
                history_listbox.delete(0, tk.END)
                for r in cache_rotas_historico:
                    history_listbox.insert(tk.END, f"{r['nome']}  |  {r['data_criacao'][:16]}")
            root.after(0, _update_ui)
        except: pass
    threading.Thread(target=_thread_list, daemon=True).start()

def carregar_rota_selecionada():
    sel = history_listbox.curselection()
    if not sel: return
    r_data = cache_rotas_historico[sel[0]]
    def _thread_load():
        try:
            res = supabase.table("comandos").select("*").eq("rota_id", r_data['id']).order("ordem").execute()
            def _apply():
                reset_path(); clear_commands()
                nome_rota_entry.delete(0, tk.END); nome_rota_entry.insert(0, r_data['nome'])
                for it in res.data:
                    c = it['comando']
                    if c != "ENTREGAR": c += f" {it['valor']}"
                    command_listbox.insert(tk.END, c)
                    simular_movimento(c, animate=False) # Só calcula posição final
                    update_gui()
                tabs.select(tab_controle)
            root.after(0, _apply)
        except: pass
    threading.Thread(target=_thread_load, daemon=True).start()

# =================================================================
# LÓGICA VISUAL E ANIMAÇÃO
# =================================================================
def simular_movimento(cmd_str, animate=True):
    global robot_x, robot_y, robot_theta, ovos_entregues
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
    
    # --- LÓGICA DA ENTREGA ---
    if tipo == "ENTREGAR":
        # Salva a coordenada atual como um ponto de entrega
        ovos_entregues.append((
            (CANVAS_WIDTH/2) + (robot_x * SCALE), 
            (CANVAS_HEIGHT/2) - (robot_y * SCALE)
        ))
        
        # --- ANIMAÇÃO VISUAL ---
        if animate:
            # Efeito Piscar: Dourado -> Vermelho
            for _ in range(3):
                canvas.itemconfig("robo_body", fill="gold") # Muda cor
                root.update()
                time.sleep(0.15)
                canvas.itemconfig("robo_body", fill="red")  # Volta cor
                root.update()
                time.sleep(0.15)

def update_gui():
    cx = (CANVAS_WIDTH/2) + (robot_x * SCALE)
    cy = (CANVAS_HEIGHT/2) - (robot_y * SCALE)
    if not path_points or (path_points[-1] != (cx, cy)): path_points.append((cx, cy))
    
    canvas.delete("all")
    # Grid
    canvas.create_line(CANVAS_WIDTH/2, 0, CANVAS_WIDTH/2, CANVAS_HEIGHT, fill="#eee")
    canvas.create_line(0, CANVAS_HEIGHT/2, CANVAS_WIDTH, CANVAS_HEIGHT/2, fill="#eee")
    
    # Rastro Azul
    if len(path_points) > 1: canvas.create_line(path_points, fill="blue", width=2)
    
    # --- DESENHA OS OVOS ENTREGUES ---
    for (ox, oy) in ovos_entregues:
        # Desenha um círculo amarelo com borda preta onde o ovo foi deixado
        canvas.create_oval(ox-5, oy-5, ox+5, oy+5, fill="gold", outline="black", width=1)
    
    # Robô (Com TAG "robo_body" para animação)
    canvas.create_oval(cx-ROBOT_RADIUS, cy-ROBOT_RADIUS, cx+ROBOT_RADIUS, cy+ROBOT_RADIUS, fill="red", tags="robo_body")
    
    # Direção
    ex = cx + ROBOT_RADIUS*1.5 * math.cos(math.radians(robot_theta))
    ey = cy - ROBOT_RADIUS*1.5 * math.sin(math.radians(robot_theta))
    canvas.create_line(cx, cy, ex, ey, fill="black", width=2)

# =================================================================
# COMUNICAÇÃO SERIAL
# =================================================================
def connect_serial():
    global ser
    if ghost_mode_var.get():
        status_label.config(text="Simulação Ativa", foreground="orange")
        return
    try:
        ser = serial.Serial(port_combobox.get(), 9600, timeout=1)
        time.sleep(2)
        status_label.config(text="Conectado!", foreground="green")
    except Exception as e: status_label.config(text=str(e), foreground="red")

def send_commands():
    cmds = command_listbox.get(0, tk.END)
    if not ghost_mode_var.get() and (not ser or not ser.is_open):
        messagebox.showerror("Erro", "Arduino desconectado!")
        return

    def _wait_ack(ack):
        if ghost_mode_var.get(): time.sleep(0.05); return True
        st = time.time()
        while time.time() - st < 3:
            if ser.in_waiting:
                try:
                    l = ser.readline().decode('utf-8', errors='ignore').strip()
                    print(f"[ARD]: {l}")
                    if ack in l: return True
                    if "ERR" in l: return False
                except: pass
            time.sleep(0.01)
        return False

    def _run():
        status_label.config(text="Enviando...")
        try:
            if not ghost_mode_var.get(): ser.reset_input_buffer(); ser.write(b"LIMPARFILA\\")
            if not _wait_ack("OK_CLR"): return

            for c in cmds:
                p = c.split()
                # --- CORREÇÃO DO CONFLITO E vs E ---
                if p[0] == "ENTREGAR":
                    # Manda 'O' de Ovo/Open para diferenciar de 'E'squerda
                    payload = "ADD(O(0))\\"
                else:
                    l = p[0][0]; v = p[1] if len(p)>1 else "0"
                    payload = f"ADD({l}({v}))\\"
                
                print(f">> {payload.strip()}")
                if not ghost_mode_var.get(): ser.write(payload.encode())
                if not _wait_ack("OK_ADD"): return
            
            status_label.config(text="Executando...")
            if not ghost_mode_var.get(): ser.write(b"EXECUTAR\\")
            if not _wait_ack("OK_RUN"): return

            # --- LOOP DE SINCRONIA ---
            idx = 0
            while idx < len(cmds):
                if ghost_mode_var.get():
                    time.sleep(0.5) # Tempo simulado
                    simular_movimento(cmds[idx], animate=True)
                    root.after(0, update_gui)
                    idx += 1
                    continue
                
                # Leitura Hardware
                if ser.in_waiting:
                    l = ser.readline().decode(errors='ignore').strip()
                    print(f"[RUN]: {l}")
                    if "STEP_DONE" in l:
                        simular_movimento(cmds[idx], animate=True)
                        root.after(0, update_gui)
                        idx += 1
                    elif "FINISH" in l: break
                    elif "ERR" in l: 
                        messagebox.showerror("Erro", "Falha no Robô!"); return
                time.sleep(0.01)
            
            status_label.config(text="Concluído!")

        except Exception as e: print(e)

    threading.Thread(target=_run, daemon=True).start()

# =================================================================
# GUI SETUP
# =================================================================
def add_cmd():
    t = cmd_type.get()
    if t == "ENTREGAR": command_listbox.insert(tk.END, t)
    elif t in ["DIREITA", "ESQUERDA"]: command_listbox.insert(tk.END, f"{t} 90")
    else: command_listbox.insert(tk.END, f"{t} {cmd_val.get()}")

def clear_commands(): command_listbox.delete(0, tk.END)
def reset_path(): 
    global path_points, robot_x, robot_y, robot_theta, ovos_entregues
    path_points.clear(); ovos_entregues.clear(); 
    robot_x=0; robot_y=0; robot_theta=0
    update_gui()

def toggle_ui(*a):
    if cmd_type.get() in ["DIREITA", "ESQUERDA", "ENTREGAR"]: cmd_val.pack_forget()
    else: cmd_val.pack(fill="x", padx=5)

init_db()
root = tk.Tk(); root.title("EGG0-1 Final")
main_frame = ttk.Frame(root, padding=5); main_frame.grid(row=0, column=0, sticky="nsew")

canvas = tk.Canvas(main_frame, width=CANVAS_WIDTH, height=CANVAS_HEIGHT, bg="white", relief="sunken")
canvas.grid(row=0, column=0, rowspan=2, padx=5, pady=5)

panel = ttk.Frame(main_frame); panel.grid(row=0, column=1, sticky="ns")
tabs = ttk.Notebook(panel); tabs.grid(row=0, column=0, sticky="nsew")

# Aba Controle
t_ctrl = ttk.Frame(tabs, padding=10); tabs.add(t_ctrl, text="Controle")
lf_con = ttk.LabelFrame(t_ctrl, text="Conexão"); lf_con.pack(fill="x", pady=5)
ghost_mode_var = tk.BooleanVar(value=False)
ttk.Checkbutton(lf_con, text="Modo Simulação", variable=ghost_mode_var).pack(anchor="w", padx=5)
try: pts = [p.device for p in serial.tools.list_ports.comports()]
except: pts = []
port_combobox = ttk.Combobox(lf_con, values=pts); 
if pts: port_combobox.current(0)
port_combobox.pack(fill="x", padx=5)
ttk.Button(lf_con, text="Conectar", command=connect_serial).pack(fill="x", padx=5)
status_label = ttk.Label(lf_con, text="Offline", foreground="blue"); status_label.pack(anchor="w", padx=5)

lf_cmd = ttk.LabelFrame(t_ctrl, text="Comandos"); lf_cmd.pack(fill="x", pady=5)
cmd_type = tk.StringVar(value="FRENTE"); cmd_type.trace_add("write", toggle_ui)
ttk.OptionMenu(lf_cmd, cmd_type, "FRENTE", "FRENTE", "DIREITA", "ESQUERDA", "ENTREGAR").pack(fill="x", padx=5)
cmd_val = ttk.Entry(lf_cmd); cmd_val.insert(0, "10"); 
ttk.Button(lf_cmd, text="Adicionar", command=add_cmd).pack(fill="x", padx=5)
toggle_ui()

lf_q = ttk.LabelFrame(t_ctrl, text="Fila"); lf_q.pack(fill="both", expand=True, pady=5)
command_listbox = tk.Listbox(lf_q, height=8); command_listbox.pack(fill="both", expand=True, padx=5)
ttk.Button(lf_q, text="Limpar", command=clear_commands).pack(side="left")
ttk.Button(lf_q, text="EXECUTAR", command=send_commands).pack(side="right")

lf_s = ttk.LabelFrame(t_ctrl, text="Salvar"); lf_s.pack(fill="x", pady=5)
ttk.Label(lf_s, text="Nome:").pack(anchor="w")
nome_rota_entry = ttk.Entry(lf_s); nome_rota_entry.pack(fill="x")
ttk.Button(lf_s, text="Salvar na Nuvem", command=salvar_rota_db).pack(fill="x")
ttk.Button(lf_s, text="Resetar Mapa", command=reset_path).pack(fill="x")

# Aba Historico
t_hist = ttk.Frame(tabs, padding=10); tabs.add(t_hist, text="Histórico")
history_listbox = tk.Listbox(t_hist, height=20); history_listbox.pack(fill="both", expand=True)
ttk.Button(t_hist, text="Atualizar", command=atualizar_historico).pack(fill="x")
ttk.Button(t_hist, text="Carregar", command=carregar_rota_selecionada).pack(fill="x")
history_status = ttk.Label(t_hist, text="..."); history_status.pack(anchor="w")

update_gui()
root.mainloop()