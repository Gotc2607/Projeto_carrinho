# =================================================================
# == EGG0-1 - INTERFACE COM LOG VISUAL & HANDSHAKE ROBUSTO      ==
# =================================================================
import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
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
CANVAS_HEIGHT = 500 # Reduzi um pouco para caber o log
SCALE = 0.7
ROBOT_RADIUS = 10 * SCALE 

# --- Variáveis Globais ---
ser = None
robot_x, robot_y, robot_theta = 0, 0, 0 
path_points = []
cache_rotas_historico = []

# =================================================================
# FUNÇÃO DE LOG (NOVIDADE)
# =================================================================
def log_system(msg, tipo="INFO"):
    """Escreve no terminal da interface e no console"""
    timestamp = datetime.now().strftime("%H:%M:%S")
    full_msg = f"[{timestamp}] [{tipo}] {msg}"
    print(full_msg) # Console do Python
    
    try:
        # Insere no widget de texto da interface
        log_widget.configure(state='normal') # Destrava para escrever
        
        tag = "normal"
        if tipo == "ERRO": tag = "erro"
        elif tipo == "RX": tag = "rx"
        elif tipo == "TX": tag = "tx"
        
        log_widget.insert(tk.END, full_msg + "\n", tag)
        log_widget.see(tk.END) # Auto-scroll para o final
        log_widget.configure(state='disabled') # Trava para não editar
    except:
        pass

# =================================================================
# FUNÇÕES DE BANCO DE DADOS
# =================================================================
def init_db():
    global supabase
    try:
        supabase = create_client(SUPABASE_URL, SUPABASE_KEY)
        log_system("Supabase conectado.", "DB")
    except Exception as e:
        log_system(f"Erro Supabase: {e}", "ERRO")

def salvar_rota_db():
    nome = nome_rota_entry.get()
    cmds = command_listbox.get(0, tk.END)
    
    if not nome or not cmds:
        log_system("Erro ao salvar: Nome ou comandos vazios", "ERRO")
        return
    
    status_label.config(text="Salvando...")
    
    def _thread_save():
        try:
            res = supabase.table("rotas").insert({
                "nome": nome, 
                "data_criacao": datetime.now().isoformat()
            }).execute()
            new_id = res.data[0]['id']
            
            payload = []
            for i, c in enumerate(cmds):
                p = c.split()
                val = float(p[1]) if len(p)>1 else 0.0
                payload.append({"rota_id": new_id, "ordem": i, "comando": p[0], "valor": val})
            
            supabase.table("comandos").insert(payload).execute()
            root.after(0, lambda: log_system("Rota salva na nuvem com sucesso!", "DB"))
            root.after(0, atualizar_historico)
            root.after(0, lambda: status_label.config(text="Salvo!"))
        except Exception as e:
            root.after(0, lambda: log_system(f"Erro ao salvar: {str(e)[:50]}", "ERRO"))

    threading.Thread(target=_thread_save, daemon=True).start()

def atualizar_historico():
    history_status.config(text="Atualizando...")
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
            root.after(0, lambda: log_system(f"Erro histórico: {e}", "ERRO"))
    threading.Thread(target=_thread_list, daemon=True).start()

def carregar_rota_selecionada():
    selection = history_listbox.curselection()
    if not selection: return
    
    index = selection[0]
    rota_dados = cache_rotas_historico[index]
    
    log_system(f"Carregando rota ID {rota_dados['id']}...", "DB")
    
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
                log_system("Rota carregada e desenhada.", "INFO")
            root.after(0, _aplicar_na_tela)
        except Exception as e:
             root.after(0, lambda: log_system(f"Erro load: {e}", "ERRO"))
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
    
    # === MODO FANTASMA ===
    if ghost_mode_var.get():
        status_label.config(text="Modo Simulação Ativo", foreground="orange")
        log_system("Conectado em MODO SIMULAÇÃO (Sem Arduino).", "INFO")
        return
    # =====================

    try:
        porta = port_combobox.get()
        ser = serial.Serial(porta, 9600, timeout=1)
        time.sleep(2)
        status_label.config(text=f"Conectado em {porta}", foreground="green")
        log_system(f"Porta Serial {porta} aberta com sucesso.", "INFO")
        threading.Thread(target=read_serial_monitor, daemon=True).start()
    except Exception as e: 
        status_label.config(text="Erro Conexão", foreground="red")
        log_system(f"Falha ao abrir serial: {e}", "ERRO")

def read_serial_monitor():
    """Lê mensagens soltas do Arduino (Logs de erro, Debug)"""
    while ser and ser.is_open:
        try: 
            if ser.in_waiting > 0:
                # Apenas lê se não estivermos num envio de lote (para não roubar a resposta)
                # Mas como o _wait_for_ack consome, aqui pegamos so o 'lixo' ou logs espontaneos
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    log_system(f"Arduino diz: {line}", "RX")
        except: pass
        time.sleep(0.1)

def send_commands():
    """ENVIA LOTE COM LOG DETALHADO"""
    cmds = command_listbox.get(0, tk.END)
    
    if not ghost_mode_var.get() and (not ser or not ser.is_open):
        messagebox.showerror("Erro", "Arduino não conectado!")
        return

    def _wait_for_ack(expected_ack, timeout=3.0):
        if ghost_mode_var.get():
            time.sleep(0.05) 
            return True, "GHOST_OK"

        start_time = time.time()
        while (time.time() - start_time) < timeout:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Loga o que recebeu
                        log_system(f"Resp: {line}", "RX")
                        
                        if "ERR_FULL" in line: return False, "Memória Cheia!"
                        if "ERR_FMT" in line: return False, "Erro Formato!"
                        if "LOG:" in line: continue # Ignora logs de debug na verificação de ACK
                        if expected_ack in line: return True, "OK"
                except: pass
            time.sleep(0.01)
        return False, "Timeout (Sem resposta)"

    def _run_batch():
        log_system("=== INICIANDO ENVIO DE LOTE ===", "INFO")
        status_label.config(text="Iniciando Envio...")
        
        try:
            if not ghost_mode_var.get(): ser.reset_input_buffer() 
            
            # 1. LIMPAR
            log_system("Enviando comando: LIMPARFILA", "TX")
            if not ghost_mode_var.get(): ser.write(b"LIMPARFILA\\")
            
            success, msg = _wait_for_ack("OK_CLR")
            if not success:
                log_system(f"Falha ao limpar: {msg}", "ERRO")
                messagebox.showerror("Erro", f"Falha ao limpar:\n{msg}")
                return

            # 2. ENVIAR
            total = len(cmds)
            for i, c in enumerate(cmds):
                status_label.config(text=f"Enviando {i+1}/{total}...")
                p = c.split()
                
                cmd_interno = ""
                if p[0] == "ENTREGAR":
                    cmd_interno = "S(90)"
                else:
                    letra = p[0][0] 
                    valor = p[1] if len(p) > 1 else "0"
                    cmd_interno = f"{letra}({valor})"
                
                payload = f"ADD({cmd_interno})\\"
                
                log_system(f"Enviando ({i+1}/{total}): {payload.strip()}", "TX")
                if not ghost_mode_var.get(): ser.write(payload.encode())
                
                success, msg = _wait_for_ack("OK_ADD")
                if not success:
                    log_system(f"Falha no comando {c}: {msg}", "ERRO")
                    messagebox.showerror("Erro", f"Falha no comando {c}:\n{msg}")
                    return

            # 3. EXECUTAR
            status_label.config(text="Executando...")
            log_system("Enviando comando: EXECUTAR", "TX")
            if not ghost_mode_var.get(): ser.write(b"EXECUTAR\\")
            
            success, msg = _wait_for_ack("OK_RUN")
            if success:
                status_label.config(text="Carrinho em movimento!")
                log_system("Confirmação de execução recebida (OK_RUN)", "RX")
            else:
                log_system(f"Erro ao iniciar: {msg}", "ERRO")
                status_label.config(text=f"Erro Start: {msg}")

            # ANIMAÇÃO
            log_system("Iniciando animação visual...", "INFO")
            for c in cmds:
                simular_movimento(c)
                root.after(0, update_gui)
                time.sleep(0.5)

            log_system("=== LOTE CONCLUÍDO ===", "INFO")
            status_label.config(text="Concluído.")

        except Exception as e:
            log_system(f"Exceção fatal: {e}", "ERRO")
            root.after(0, lambda: status_label.config(text="Erro fatal"))

    threading.Thread(target=_run_batch, daemon=True).start()

def add_cmd():
    t = cmd_type.get()
    if t == "ENTREGAR":
        command_listbox.insert(tk.END, t)
    elif t in ["DIREITA", "ESQUERDA"]:
        command_listbox.insert(tk.END, f"{t} 90")
    else: 
        v = cmd_val.get()
        if not v or not v.replace('.', '', 1).isdigit():
            messagebox.showwarning("Aviso", "Valor inválido.")
            return
        command_listbox.insert(tk.END, f"{t} {v}")

def clear_commands(): 
    command_listbox.delete(0, tk.END)
    log_system("Fila de comandos limpa.", "INFO")

def reset_path(): 
    global path_points, robot_x, robot_y, robot_theta
    path_points.clear(); robot_x=0; robot_y=0; robot_theta=0
    update_gui()
    log_system("Mapa resetado.", "INFO")

def toggle_val_entry(*args):
    current_type = cmd_type.get()
    if current_type in ["DIREITA", "ESQUERDA", "ENTREGAR"]:
        cmd_val.pack_forget()
        cmd_val.delete(0, tk.END)
        cmd_val.insert(0, "90")
    else:
        cmd_val.pack(fill="x", padx=5)
        if cmd_val.get() == "0" or not cmd_val.get():
             cmd_val.delete(0, tk.END)
             cmd_val.insert(0, "10")

# =================================================================
# SETUP GUI
# =================================================================
init_db()
root = tk.Tk()
root.title("EGG0-1 Control Station")

# Layout Principal
main_frame = ttk.Frame(root, padding=5)
main_frame.grid(row=0, column=0, sticky="nsew")

# --- ÁREA SUPERIOR: MAPA E CONTROLES ---
top_frame = ttk.Frame(main_frame)
top_frame.grid(row=0, column=0, sticky="nsew")

# MAPA
canvas = tk.Canvas(top_frame, width=CANVAS_WIDTH, height=CANVAS_HEIGHT, bg="white", relief="sunken")
canvas.grid(row=0, column=0, padx=5, pady=5)

# PAINEL CONTROLES
right_panel = ttk.Frame(top_frame)
right_panel.grid(row=0, column=1, sticky="ns")

tabs = ttk.Notebook(right_panel)
tabs.pack(fill="both", expand=True)

# ABA 1 - Controle
tab_controle = ttk.Frame(tabs, padding=10)
tabs.add(tab_controle, text="Controle")

lf_conn = ttk.LabelFrame(tab_controle, text="Conexão")
lf_conn.pack(fill="x", pady=5)
ghost_mode_var = tk.BooleanVar(value=False)
ttk.Checkbutton(lf_conn, text="Modo Simulação", variable=ghost_mode_var).pack(anchor="w", padx=5)
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
command_listbox = tk.Listbox(lf_queue, height=6)
command_listbox.pack(fill="both", expand=True, padx=5)
ttk.Button(lf_queue, text="Limpar", command=clear_commands).pack(side="left")
ttk.Button(lf_queue, text="EXECUTAR LOTE", command=send_commands).pack(side="right")

lf_save = ttk.LabelFrame(tab_controle, text="Salvar")
lf_save.pack(fill="x", pady=5)
ttk.Label(lf_save, text="Nome:").pack(anchor="w")
nome_rota_entry = ttk.Entry(lf_save); nome_rota_entry.pack(fill="x")
ttk.Button(lf_save, text="Salvar na Nuvem", command=salvar_rota_db).pack(fill="x")
ttk.Button(lf_save, text="Resetar Mapa", command=reset_path).pack(fill="x")

# ABA 2 - Histórico
tab_historico = ttk.Frame(tabs, padding=10)
tabs.add(tab_historico, text="Histórico")
history_listbox = tk.Listbox(tab_historico, height=15)
history_listbox.pack(fill="both", expand=True)
ttk.Button(tab_historico, text="Atualizar Lista", command=atualizar_historico).pack(fill="x")
ttk.Button(tab_historico, text="CARREGAR ROTA", command=carregar_rota_selecionada).pack(fill="x")
history_status = ttk.Label(tab_historico, text="...")
history_status.pack(anchor="w")

# --- ÁREA INFERIOR: MONITOR DE LOGS (NOVO) ---
log_frame = ttk.LabelFrame(main_frame, text="Monitor Serial / Logs")
log_frame.grid(row=1, column=0, sticky="nsew", padx=5, pady=5)

log_widget = scrolledtext.ScrolledText(log_frame, height=8, state='disabled', font=("Consolas", 9))
log_widget.pack(fill="both", expand=True)

# Configurando cores para as tags do log
log_widget.tag_config("erro", foreground="red")
log_widget.tag_config("tx", foreground="green") # Enviado (Verde)
log_widget.tag_config("rx", foreground="blue")  # Recebido (Azul)
log_widget.tag_config("normal", foreground="black")

update_gui()
log_system("Sistema iniciado.", "INFO")
root.mainloop()