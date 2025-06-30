import tkinter as tk
from tkinter import ttk
import websocket
import threading
import time

# WebSocket server URL (replace with your ESP32's IP if different)
WS_URL = "ws://192.168.4.1:81/"

class WebSerialGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 WebSerial Gateway")
        self.root.geometry("600x500")
        self.root.configure(bg="#e0e7ff")

        # Style configuration
        self.style = ttk.Style()
        self.style.configure("TButton", font=("Arial", 10), padding=5)
        self.style.configure("TEntry", font=("Arial", 10), padding=5)

        # Main container
        self.container = tk.Frame(root, bg="#ffffff", bd=2, relief="flat")
        self.container.place(relx=0.5, rely=0.5, anchor="center", width=560, height=460)
        self.container.configure(highlightbackground="#d1d5db", highlightcolor="#d1d5db", highlightthickness=1)

        # Title
        tk.Label(self.container, text="ESP32 WebSerial Gateway", font=("Arial", 14, "bold"), bg="#ffffff", fg="#1f2937").pack(pady=10)

        # Status
        self.status_var = tk.StringVar(value="Disconnected")
        self.status_label = tk.Label(self.container, textvariable=self.status_var, font=("Arial", 10, "bold"), bg="#ffffff", fg="#dc2626")
        self.status_label.pack(anchor="w", padx=10)

        # Message area
        self.message_frame = tk.Frame(self.container, bg="#f9fafb", bd=1, relief="solid")
        self.message_frame.pack(fill="both", expand=True, padx=10, pady=5)
        self.message_area = tk.Text(self.message_frame, height=15, font=("Arial", 10), bg="#f9fafb", fg="#1f2937", wrap="word", state="disabled")
        self.message_area.pack(fill="both", expand=True, padx=5, pady=5)
        scrollbar = ttk.Scrollbar(self.message_frame, orient="vertical", command=self.message_area.yview)
        scrollbar.pack(side="right", fill="y")
        self.message_area.configure(yscrollcommand=scrollbar.set)

        # Input and buttons
        self.input_frame = tk.Frame(self.container, bg="#ffffff")
        self.input_frame.pack(fill="x", padx=10, pady=5)
        self.input_entry = ttk.Entry(self.input_frame)
        self.input_entry.pack(side="left", fill="x", expand=True, padx=(0, 5))
        self.input_entry.bind("<Return>", lambda e: self.send_message())
        self.send_button = ttk.Button(self.input_frame, text="Send", command=self.send_message)
        self.send_button.pack(side="left", padx=5)
        self.clear_button = ttk.Button(self.input_frame, text="Clear", command=self.clear_messages)
        self.clear_button.pack(side="left")

        # WebSocket
        self.ws = None
        self.running = True
        self.ws_thread = threading.Thread(target=self.websocket_thread)
        self.ws_thread.daemon = True
        self.ws_thread.start()

        # Styling for messages
        self.message_area.tag_configure("serial", foreground="#16a34a", background="#ecfdf5")
        self.message_area.tag_configure("user", foreground="#2563eb", background="#eff6ff")
        self.message_area.tag_configure("system", foreground="#6b7280")

    def websocket_thread(self):
        while self.running:
            try:
                self.ws = websocket.WebSocketApp(
                    WS_URL,
                    on_open=self.on_open,
                    on_message=self.on_message,
                    on_close=self.on_close,
                    on_error=self.on_error
                )
                self.ws.run_forever()
            except Exception as e:
                self.add_message(f"[System] Error: {str(e)}", "system")
                time.sleep(5)  # Retry after 5 seconds
            if not self.running:
                break

    def on_open(self, ws):
        self.status_var.set("Connected")
        self.status_label.configure(fg="#16a34a")
        self.add_message("[System] WebSocket connected", "system")

    def on_message(self, ws, message):
        if message.startswith("[Serial]"):
            self.add_message(message, "serial")

    def on_close(self, ws, close_status_code, close_msg):
        self.status_var.set("Disconnected")
        self.status_label.configure(fg="#dc2626")
        self.add_message("[System] Connection lost. Retrying...", "system")

    def on_error(self, ws, error):
        self.add_message(f"[System] Error: {str(error)}", "system")

    def send_message(self):
        msg = self.input_entry.get().strip()
        if msg and self.ws and self.ws.sock and self.ws.sock.connected:
            self.ws.send(msg)
            self.add_message(f"[You] {msg}", "user")
            self.input_entry.delete(0, tk.END)

    def add_message(self, msg, tag):
        self.message_area.configure(state="normal")
        self.message_area.insert(tk.END, msg + "\n", tag)
        self.message_area.configure(state="disabled")
        self.message_area.see(tk.END)

    def clear_messages(self):
        self.message_area.configure(state="normal")
        self.message_area.delete(1.0, tk.END)
        self.message_area.configure(state="disabled")

    def destroy(self):
        self.running = False
        if self.ws and self.ws.sock:
            self.ws.close()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = WebSerialGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.destroy)
    root.mainloop()