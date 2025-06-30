import tkinter as tk
from tkinter import ttk
import websocket
import threading
import time
from PIL import Image, ImageTk

# WebSocket server URL (replace with your ESP32's IP if different)
WS_URL = "ws://192.168.4.1:81/"

class ModernWebSerialGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 WebSerial Gateway")
        self.root.geometry("800x600")
        self.root.minsize(700, 500)
        
        # Custom colors
        self.bg_color = "#1e293b"  # Dark slate
        self.card_color = "#334155"  # Slightly lighter slate
        self.text_color = "#f8fafc"  # Almost white
        self.accent_color = "#3b82f6"  # Bright blue
        self.success_color = "#10b981"  # Emerald
        self.error_color = "#ef4444"  # Red
        self.warning_color = "#f59e0b"  # Amber
        
        # Configure root window
        self.root.configure(bg=self.bg_color)
        self.root.option_add("*tearOff", False)
        
        # Custom fonts
        self.title_font = ("Segoe UI", 16, "bold")
        self.subtitle_font = ("Segoe UI", 10)
        self.mono_font = ("Consolas", 10)
        self.button_font = ("Segoe UI", 10, "bold")
        
        # Create main container with shadow effect
        self.create_shadow()
        self.container = tk.Frame(root, bg=self.card_color, bd=0, 
                                highlightthickness=0, padx=20, pady=20)
        self.container.place(relx=0.5, rely=0.5, anchor="center", 
                            width=760, height=560)
        
        # Header section
        self.header_frame = tk.Frame(self.container, bg=self.card_color)
        self.header_frame.pack(fill="x", pady=(0, 10))
        
        # Title and subtitle
        tk.Label(self.header_frame, text="ESP32 WebSerial Gateway", 
                font=self.title_font, bg=self.card_color, fg=self.text_color).pack(side="left")
        
        # Connection status indicator
        self.status_indicator = tk.Canvas(self.header_frame, width=20, height=20, 
                                        bg=self.card_color, highlightthickness=0)
        self.status_indicator.pack(side="right", padx=5)
        self.status_circle = self.status_indicator.create_oval(5, 5, 15, 15, 
                                                             fill=self.error_color)
        
        self.status_var = tk.StringVar(value="Disconnected")
        tk.Label(self.header_frame, textvariable=self.status_var, 
                font=self.subtitle_font, bg=self.card_color, 
                fg=self.text_color).pack(side="right")
        
        # Connection controls
        self.control_frame = tk.Frame(self.container, bg=self.card_color)
        self.control_frame.pack(fill="x", pady=(0, 15))
        
        self.create_buttons()
        
        # Message display area
        self.message_frame = tk.Frame(self.container, bg="#475569", bd=0)
        self.message_frame.pack(fill="both", expand=True)
        
        self.scrollbar = ttk.Scrollbar(self.message_frame)
        self.scrollbar.pack(side="right", fill="y")
        
        self.message_area = tk.Text(self.message_frame, height=15, 
                                  font=self.mono_font, bg="#475569", 
                                  fg=self.text_color, wrap="word", 
                                  state="disabled", padx=15, pady=15,
                                  yscrollcommand=self.scrollbar.set,
                                  insertbackground=self.accent_color,
                                  selectbackground=self.accent_color)
        self.message_area.pack(fill="both", expand=True)
        self.scrollbar.config(command=self.message_area.yview)
        
        # Input area
        self.input_frame = tk.Frame(self.container, bg=self.card_color)
        self.input_frame.pack(fill="x", pady=(15, 0))
        
        self.input_entry = ttk.Entry(self.input_frame, font=self.mono_font)
        self.input_entry.pack(side="left", fill="x", expand=True, padx=(0, 10))
        self.input_entry.bind("<Return>", lambda e: self.send_message())
        
        self.send_button = ttk.Button(self.input_frame, text="Send", 
                                    command=self.send_message, 
                                    style="Accent.TButton")
        self.send_button.pack(side="left")
        
        # Configure message tags
        self.configure_tags()
        
        # WebSocket connection
        self.ws = None
        self.running = True
        self.connect()  # Initial connection attempt
        
        # Configure styles
        self.configure_styles()
        
        self.input_entry.focus_set()
    
    def create_shadow(self):
        shadow = tk.Frame(self.root, bg="#000000", bd=0)
        shadow.place(relx=0.5, rely=0.5, anchor="center", width=770, height=570, relwidth=1)
        shadow.lower()
    
    def create_buttons(self):
        button_style = {"style": "TButton", "width": 10}
        
        # Reconnect button (hidden by default)
        self.reconnect_button = ttk.Button(self.control_frame, text="Reconnect", 
                                        command=self.connect, **button_style)
        # Don't pack it initially - only show when disconnected
        
        self.clear_button = ttk.Button(self.control_frame, text="Clear", 
                                     command=self.clear_messages, **button_style)
        self.clear_button.pack(side="right", padx=5)
        
        # Statistics labels
        self.stats_frame = tk.Frame(self.control_frame, bg=self.card_color)
        self.stats_frame.pack(side="left")
        
        self.sent_var = tk.StringVar(value="Sent: 0")
        self.received_var = tk.StringVar(value="Received: 0")
        
        tk.Label(self.stats_frame, textvariable=self.sent_var, 
                font=self.subtitle_font, bg=self.card_color, 
                fg=self.text_color).pack(side="left", padx=10)
        tk.Label(self.stats_frame, textvariable=self.received_var, 
                font=self.subtitle_font, bg=self.card_color, 
                fg=self.text_color).pack(side="left", padx=10)
        
        self.sent_count = 0
        self.received_count = 0
    
    def configure_tags(self):
        self.message_area.tag_configure("serial", foreground="#86efac", background="#064e3b")
        self.message_area.tag_configure("user", foreground="#93c5fd", background="#1e40af")
        self.message_area.tag_configure("system", foreground="#d1d5db", background="#4b5563")
        self.message_area.tag_configure("error", foreground="#fca5a5", background="#7f1d1d")
        self.message_area.tag_configure("warning", foreground="#fde68a", background="#92400e")
    
    def configure_styles(self):
        style = ttk.Style()
        style.theme_use("clam")
        
        style.configure("TFrame", background=self.bg_color)
        
        style.configure("TButton", font=self.button_font, 
                       foreground=self.text_color, background=self.card_color,
                       bordercolor=self.card_color, relief="flat", 
                       padding=6)
        style.map("TButton", 
                 background=[("active", "#475569")],
                 bordercolor=[("active", "#64748b")])
        
        style.configure("Accent.TButton", font=self.button_font, 
                       foreground="#ffffff", background=self.accent_color,
                       bordercolor=self.accent_color)
        style.map("Accent.TButton", 
                 background=[("active", "#2563eb")],
                 bordercolor=[("active", "#2563eb")])
        
        style.configure("TEntry", font=self.mono_font, 
                       fieldbackground="#475569", foreground=self.text_color,
                       bordercolor="#64748b", lightcolor="#64748b", 
                       darkcolor="#64748b", padding=8)
        style.map("TEntry", 
                 bordercolor=[("focus", self.accent_color)],
                 lightcolor=[("focus", self.accent_color)],
                 darkcolor=[("focus", self.accent_color)])
        
        style.configure("Vertical.TScrollbar", background=self.card_color,
                       troughcolor=self.bg_color, bordercolor=self.bg_color,
                       arrowcolor=self.text_color, gripcount=0)
        style.map("Vertical.TScrollbar", 
                 background=[("active", "#64748b")])
    
    def connect(self):
        """Manual connection method (called by reconnect button)"""
        if self.ws and hasattr(self.ws, 'sock') and self.ws.sock:
            self.ws.close()
        
        self.add_message("[System] Attempting to connect...", "system")
        self.reconnect_button.pack_forget()  # Hide while attempting
        
        try:
            self.ws = websocket.WebSocketApp(
                WS_URL,
                on_open=self.on_open,
                on_message=self.on_message,
                on_close=self.on_close,
                on_error=self.on_error
            )
            
            # Run in a separate thread to avoid blocking the GUI
            self.ws_thread = threading.Thread(target=self.ws.run_forever)
            self.ws_thread.daemon = True
            self.ws_thread.start()
            
        except Exception as e:
            self.add_message(f"[System] Connection error: {str(e)}", "error")
            self.status_var.set("Disconnected")
            self.status_indicator.itemconfig(self.status_circle, fill=self.error_color)
            self.reconnect_button.pack(side="right", padx=5)  # Show reconnect button
    
    def on_open(self, ws):
        self.root.after(0, lambda: self.status_var.set("Connected"))
        self.root.after(0, lambda: self.status_indicator.itemconfig(
            self.status_circle, fill=self.success_color))
        self.root.after(0, lambda: self.add_message(
            "[System] WebSocket connection established", "system"))
        # Hide reconnect button when connected
        self.root.after(0, lambda: self.reconnect_button.pack_forget())
    
    def on_message(self, ws, message):
        self.received_count += 1
        self.root.after(0, lambda: self.received_var.set(f"Received: {self.received_count}"))
        
        if message.startswith("[Serial]"):
            self.root.after(0, lambda: self.add_message(message, "serial"))
        elif message.startswith("[Error]"):
            self.root.after(0, lambda: self.add_message(message, "error"))
        elif message.startswith("[Warning]"):
            self.root.after(0, lambda: self.add_message(message, "warning"))
    
    def on_close(self, ws, close_status_code, close_msg):
        self.root.after(0, lambda: self.status_var.set("Disconnected"))
        self.root.after(0, lambda: self.status_indicator.itemconfig(
            self.status_circle, fill=self.error_color))
        self.root.after(0, lambda: self.add_message(
            f"[System] Connection closed (Code: {close_status_code}, Message: {close_msg})", 
            "system"))
        # Show reconnect button when disconnected
        self.root.after(0, lambda: self.reconnect_button.pack(side="right", padx=5))
    
    def on_error(self, ws, error):
        self.root.after(0, lambda: self.add_message(
            f"[System] WebSocket error: {str(error)}", "error"))
        self.root.after(0, lambda: self.status_var.set("Disconnected"))
        self.root.after(0, lambda: self.status_indicator.itemconfig(
            self.status_circle, fill=self.error_color))
        # Show reconnect button on error
        self.root.after(0, lambda: self.reconnect_button.pack(side="right", padx=5))
    
    def send_message(self):
        msg = self.input_entry.get().strip()
        if msg:
            if self.ws and hasattr(self.ws, 'sock') and self.ws.sock and self.ws.sock.connected:
                try:
                    self.ws.send(msg)
                    self.sent_count += 1
                    self.root.after(0, lambda: self.sent_var.set(f"Sent: {self.sent_count}"))
                    self.add_message(f"[You] {msg}", "user")
                    self.input_entry.delete(0, tk.END)
                except Exception as e:
                    self.add_message(f"[System] Failed to send message: {str(e)}", "error")
            else:
                self.add_message("[System] Not connected - message not sent", "warning")
    
    def add_message(self, msg, tag):
        self.message_area.configure(state="normal")
        self.message_area.insert(tk.END, msg + "\n", tag)
        self.message_area.configure(state="disabled")
        self.message_area.see(tk.END)
    
    def clear_messages(self):
        self.message_area.configure(state="normal")
        self.message_area.delete(1.0, tk.END)
        self.message_area.configure(state="disabled")
        
        self.sent_count = 0
        self.received_count = 0
        self.sent_var.set("Sent: 0")
        self.received_var.set("Received: 0")
    
    def destroy(self):
        self.running = False
        if self.ws and hasattr(self.ws, 'sock') and self.ws.sock:
            self.ws.close()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    try:
        photo = ImageTk.PhotoImage(img)
        root.iconphoto(False, photo)
    except:
        pass
    app = ModernWebSerialGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.destroy)
    root.mainloop()