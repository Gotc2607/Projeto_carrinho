# =================================================================
# == EGG0-1 - INTERFACE BLINDADA (SYNC & TIMEOUT & GHOST)       ==
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
cache_rotas_historico = []

# =================================================================
# FUNÇÕES DE BANCO DE DADOS
# =================================================================
def init_db():
    global supabase
    try:
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
            res = supabase.table("rotas").insert({
                "nome": nome, "data_criacao": datetime.now().isoformat()
            }).execute()
            new_id = res.data[0]['id']
            payload = []
            for i, c in enumerate(cmds):
                p = c.split()
                val = float(p[1]) if len(p)>1 else 0.0
                payload.append({"rota_id": new_id, "ordem": i, "comando": p[0], "valor": val})
            supabase.table("comandos").insert(payload).execute()
            root.after(0, lambda: status_label.config(text="Salvo com sucesso!"))
            root.after(0, atualizar_historico)
        except Exception as e:
            root.after(0, lambda: status_label.config(text=f"Erro: {str(e)[:30]}"))
    threading.Thread(target=_thread_save, daemon=True).start()

def atualizar_historico():
    history_status.config(text="Atualizando lista...")
    def _thread_list():
        global cache_rotas_historico
        try:
            res = supabase.table("rotas").select("*").order("data_criacao", desc=True).limit(50).execute()
            cache_rotas_historico = res.data
            def _update_ui_list():
                history_listbox.delete(0, tk.END)
                for r in cache_rotas_historico:
                    data_raw = r['data_criacao'].split('.')[0].replace('T', ' ')
                    history_listbox.insert(tk.END, f"{r['nome']}  |  {data_raw}")
                history_status.config(text="Lista atualizada.")
            root.after(0, _update_ui_list)
        except Exception as e:
            root.after(0, lambda: history_status.config(text=f"Erro: {e}"))
    threading.Thread(target=_thread_list, daemon=True).start()

def carregar_rota_selecionada():
    selection = history_listbox.curselection()
    if not selection: return
    index = selection[0]
    rota_dados = cache_rotas_historico[index]
    history_status.config(text=f"Carregando ID {rota_dados['id']}...")
    def _thread_load():
        try:
            res = supabase.table("comandos").select("*").eq("rota_id", rota_dados['id']).order("ordem").execute()
            cmds_db = res.data
            def _aplicar_na_tela():
                reset_path()
                clear_commands()
                nome_rota_entry.delete(0, tk.END)
                nome_rota_entry.insert(0, rota_dados['nome'])
                for item in cmds_db:
                    cmd_str = item['comando']
                    if item['comando'] != "ENTREGAR":
                        cmd_str += f" {item['valor']}"
                    command_listbox.insert(tk.END, cmd_str)
                    simular_movimento(cmd_str)
                    update_gui()
                tabs.select(tab_controle)
                history_status.config(text="Rota carregada!")
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
    if ghost_mode_var.get():
        status_label.config(text="Modo Simulação Ativo", foreground="orange")
        messagebox.showinfo("Modo Fantasma", "Conexão Simulada!\nComandos visuais apenas.")
        return
    try:
        ser = serial.Serial(port_combobox.get(), 9600, timeout=1)
        time.sleep(2)
        status_label.config(text="Conectado (Hardware)!", foreground="green")
    except Exception as e: status_label.config(text=str(e), foreground="red")

def send_commands():
    """ENVIA E SINCRONIZA COM ARDUINO (COM FEEDBACK)"""
    cmds = command_listbox.get(0, tk.END)
    
    if not ghost_mode_var.get() and (not ser or not ser.is_open):
        messagebox.showerror("Erro", "Arduino não conectado!")
        return

    def _wait_for_ack(expected_ack, timeout=3.0):
        if ghost_mode_var.get():
            time.sleep(0.05); return True, "GHOST_OK"
        start = time.time()
        while (time.time() - start) < timeout:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    print(f"[Arduino ACK]: {line}") 
                    if "ERR_FULL" in line: return False, "Memória Cheia!"
                    if "ERR_FMT" in line: return False, "Erro Formato!"
                    if expected_ack in line: return True, "OK"
                except: pass
            time.sleep(0.01)
        return False, "Timeout (Sem resposta)"

    def _run_batch():
        status_label.config(text="Iniciando Envio...")
        try:
            if not ghost_mode_var.get(): ser.reset_input_buffer() 
            
            # 1. LIMPAR E ENVIAR (Protocolo de Handshake)
            print(">> [CMD] LIMPARFILA")
            if not ghost_mode_var.get(): ser.write(b"LIMPARFILA\\")
            if not _wait_for_ack("OK_CLR")[0]: 
                status_label.config(text="Erro ao limpar."); return

            total = len(cmds)
            for i, c in enumerate(cmds):
                status_label.config(text=f"Enviando {i+1}/{total}...")
                p = c.split(); letra = p[0][0]; valor = p[1] if len(p)>1 else "0"
                payload = f"ADD({letra}({valor}))\\"
                print(f">> [CMD] {payload.strip()}")
                
                if not ghost_mode_var.get(): ser.write(payload.encode())
                if not _wait_for_ack("OK_ADD")[0]:
                    status_label.config(text=f"Erro no cmd {i}"); return

            # 2. START EXECUÇÃO
            status_label.config(text="Executando Percurso...")
            print(">> [CMD] EXECUTAR")
            if not ghost_mode_var.get(): ser.write(b"EXECUTAR\\")
            
            if not _wait_for_ack("OK_RUN")[0]:
                status_label.config(text="Erro ao iniciar."); return

            # 3. LOOP DE SINCRONIA (A GRANDE MUDANÇA)
            print("--- Aguardando Passos do Robô ---")
            passo_atual = 0
            
            while passo_atual < total:
                
                # A. MODO FANTASMA (Simulação pura)
                if ghost_mode_var.get():
                    time.sleep(0.5) # Simula tempo do robô
                    simular_movimento(cmds[passo_atual])
                    root.after(0, update_gui)
                    passo_atual += 1
                    continue

                # B. MODO REAL (Hardware Feedback)
                if ser.in_waiting > 0:
                    try:
                        line = ser.readline().decode('utf-8', errors='ignore').strip()
                        print(f"[Arduino Run]: {line}")
                        
                        if "STEP_DONE" in line:
                            # Robô terminou um passo! Atualiza mapa.
                            simular_movimento(cmds[passo_atual])
                            root.after(0, update_gui)
                            passo_atual += 1
                            
                        elif "ERR_TIMEOUT" in line:
                            messagebox.showerror("ALERTA", "O robô parou por segurança (Timeout)!\nVerifique se bateu ou travou.")
                            status_label.config(text="Travado/Timeout")
                            return # Encerra tudo
                            
                        elif "FINISH" in line:
                            break # Acabou tudo
                    except: pass
                
                time.sleep(0.01)

            status_label.config(text="Percurso Concluído.")

        except Exception as e:
            print(f"Erro: {e}")
            root.after(0, lambda: status_label.config(text=f"Erro: {e}"))

    threading.Thread(target=_run_batch, daemon=True).start()

def add_cmd():
    t = cmd_type.get()
    if t == "ENTREGAR":
        command_listbox.insert(tk.END, t)
    elif t in ["DIREITA", "ESQUERDA"]:
        command_listbox.insert(tk.END, f"{t} 90")
    else: # FRENTE, TRAS
        v = cmd_val.get()
        if not v or not v.replace('.', '', 1).isdigit(): return
        command_listbox.insert(tk.END, f"{t} {v}")

def clear_commands(): command_listbox.delete(0, tk.END)
def reset_path(): 
    global path_points, robot_x, robot_y, robot_theta
    path_points.clear(); robot_x=0; robot_y=0; robot_theta=0
    update_gui()

def toggle_val_entry(*args):
    if cmd_type.get() in ["DIREITA", "ESQUERDA", "ENTREGAR"]:
        cmd_val.pack_forget() 
    else:
        cmd_val.pack(fill="x", padx=5) 
        if not cmd_val.get(): cmd_val.insert(0, "10")

# =================================================================
# SETUP GUI
# =================================================================
init_db()
root = tk.Tk()
root.title("EGG0-1 Control (Sync + Safety)")

main_frame = ttk.Frame(root, padding=5)
main_frame.grid(row=0, column=0, sticky="nsew")

canvas = tk.Canvas(main_frame, width=CANVAS_WIDTH, height=CANVAS_HEIGHT, bg="white", relief="sunken")
canvas.grid(row=0, column=0, rowspan=2, padx=5, pady=5)

right_panel = ttk.Frame(main_frame)
right_panel.grid(row=0, column=1, sticky="ns")

tabs = ttk.Notebook(right_panel)
tabs.grid(row=0, column=0, sticky="nsew")

tab_controle = ttk.Frame(tabs, padding=10)
tabs.add(tab_controle, text="Controle")

lf_conn = ttk.LabelFrame(tab_controle, text="Conexão")
lf_conn.pack(fill="x", pady=5)

ghost_mode_var = tk.BooleanVar(value=False)
chk_ghost = ttk.Checkbutton(lf_conn, text="Modo Simulação (Sem Arduino)", variable=ghost_mode_var)
chk_ghost.pack(anchor="w", padx=5)

try: ports = [p.device for p in serial.tools.list_ports.comports()]
except: ports = []
port_combobox = ttk.Combobox(lf_conn, values=ports); 
if ports: port_combobox.current(0)
port_combobox.pack(fill="x", padx=5, pady=2)
ttk.Button(lf_conn, text="Conectar", command=connect_serial).pack(fill="x", padx=5)
status_label = ttk.Label(lf_conn, text="Offline", foreground="blue")
status_label.pack(anchor="w", padx=5)

lf_cmds = ttk.LabelFrame(tab_controle, text="Comandos")
lf_cmds.pack(fill="x", pady=5)
cmd_type = tk.StringVar(value="FRENTE")
cmd_type.trace_add("write", toggle_val_entry) 
ttk.OptionMenu(lf_cmds, cmd_type, "FRENTE", "FRENTE", "DIREITA", "ESQUERDA", "ENTREGAR").pack(fill="x", padx=5)
cmd_val = ttk.Entry(lf_cmds); cmd_val.insert(0, "10"); 
ttk.Button(lf_cmds, text="Adicionar", command=add_cmd).pack(fill="x", padx=5)
toggle_val_entry()

lf_queue = ttk.LabelFrame(tab_controle, text="Fila")
lf_queue.pack(fill="both", expand=True, pady=5)
command_listbox = tk.Listbox(lf_queue, height=8)
command_listbox.pack(fill="both", expand=True, padx=5)
ttk.Button(lf_queue, text="Limpar", command=clear_commands).pack(side="left")
ttk.Button(lf_queue, text="EXECUTAR LOTE", command=send_commands).pack(side="right")

lf_save = ttk.LabelFrame(tab_controle, text="Salvar")
lf_save.pack(fill="x", pady=5)
ttk.Label(lf_save, text="Nome:").pack(anchor="w")
nome_rota_entry = ttk.Entry(lf_save); nome_rota_entry.pack(fill="x")
ttk.Button(lf_save, text="Salvar na Nuvem", command=salvar_rota_db).pack(fill="x")
ttk.Button(lf_save, text="Resetar Mapa", command=reset_path).pack(fill="x")

tab_historico = ttk.Frame(tabs, padding=10)
tabs.add(tab_historico, text="Histórico")
history_listbox = tk.Listbox(tab_historico, height=20)
history_listbox.pack(fill="both", expand=True)
ttk.Button(tab_historico, text="Atualizar Lista", command=atualizar_historico).pack(fill="x")
ttk.Button(tab_historico, text="CARREGAR ROTA", command=carregar_rota_selecionada).pack(fill="x")
history_status = ttk.Label(tab_historico, text="...")
history_status.pack(anchor="w")

update_gui()
root.mainloop()