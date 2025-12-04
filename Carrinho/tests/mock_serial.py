import time

class MockSerial:
    def __init__(self, port, baudrate, timeout=2):
        self.is_open = True
        self.in_waiting = 0
        self.response_queue = []
        print(f"[MOCK] Arduino Virtual conectado em {port}")

    def write(self, data):
        msg = data.decode()
        print(f"[MOCK RX] Recebido: {msg.strip()}")
        
        if "LIMPARFILA" in msg:
            self._queue("OK_CLR")
        elif "ADD" in msg:
            self._queue("OK_ADD")
        elif "EXECUTAR" in msg:
            self._queue("OK_RUN")
            self._queue("STEP_DONE 0")
            self._queue("STEP_DONE 1")
            self._queue("FINISH")

    def _queue(self, txt):
        time.sleep(0.1) 
        self.response_queue.append(txt + "\r\n")
        self.in_waiting = 1

    def readline(self):
        if self.response_queue:
            return self.response_queue.pop(0).encode()
        return b""
    
    def reset_input_buffer(self):
        self.response_queue = []