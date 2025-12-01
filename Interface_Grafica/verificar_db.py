import sqlite3

def ler_banco():
    try:
        conn = sqlite3.connect('rotas_carrinho.db')
        cursor = conn.cursor()
        
        print("=== ROTAS SALVAS ===")
        cursor.execute("SELECT * FROM rotas")
        rotas = cursor.fetchall()
        
        for rota in rotas:
            print(f"\nID: {rota[0]} | Nome: {rota[1]} | Data: {rota[2]}")
            print("-" * 30)
            
            # Pega os comandos desta rota
            cursor.execute("SELECT ordem, comando, valor FROM comandos WHERE rota_id = ?", (rota[0],))
            comandos = cursor.fetchall()
            for cmd in comandos:
                print(f"   {cmd[0]}. {cmd[1]} -> {cmd[2]}")
                
        conn.close()
    except Exception as e:
        print("Banco ainda não criado ou erro:", e)

if __name__ == "__main__":
    ler_banco()