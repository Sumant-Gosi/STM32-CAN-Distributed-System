import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import re
import threading

# ── Configuration ─────────────────────────────────
PORT     = '/dev/tty.usbmodem14203'  # Change to your port
BAUD     = 115200
MAX_POINTS = 100  # How many data points to show on graph

# ── Data Buffers ───────────────────────────────────
rpm_data  = deque([0] * MAX_POINTS, maxlen=MAX_POINTS)
temp_data = deque([0] * MAX_POINTS, maxlen=MAX_POINTS)

# ── Open Serial Port ───────────────────────────────
try:
    ser = serial.Serial(PORT, BAUD, timeout=1)
    print(f"Connected to {PORT}")
except Exception as e:
    print(f"Could not open port: {e}")
    exit()

# ── Parse Incoming UART Lines ──────────────────────
def parse_line(line):
    # Matches lines like: [CAN] TX RPM: 3000
    rpm_match  = re.search(r'TX RPM:\s*(\d+)', line)
    temp_match = re.search(r'TX TEMP:\s*(-?\d+)', line)

    if rpm_match:
        rpm_data.append(int(rpm_match.group(1)))

    if temp_match:
        temp_data.append(int(temp_match.group(1)))

# ── Background Thread — Reads Serial Continuously ──
def read_serial():
    while True:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(line)   # Also print to terminal
                parse_line(line)
        except:
            pass

thread = threading.Thread(target=read_serial, daemon=True)
thread.start()

# ── Plot Setup ─────────────────────────────────────
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))
fig.suptitle('STM32 CAN Bus - Node A Live Data', fontsize=14)

def animate(i):
    # ── RPM Plot ───────────────────────────────────
    ax1.clear()
    ax1.plot(list(rpm_data), color='blue', linewidth=1.5)
    ax1.set_title('Engine RPM')
    ax1.set_ylabel('RPM')
    ax1.set_ylim(0, 7000)
    ax1.axhline(y=5000, color='red', linestyle='--', label='Warning threshold')
    ax1.legend(loc='upper left')
    ax1.grid(True)

    # ── Temperature Plot ───────────────────────────
    ax2.clear()
    ax2.plot(list(temp_data), color='orange', linewidth=1.5)
    ax2.set_title('Temperature')
    ax2.set_ylabel('°C')
    ax2.set_ylim(0, 120)
    ax2.axhline(y=80, color='red', linestyle='--', label='Warning threshold')
    ax2.legend(loc='upper left')
    ax2.grid(True)

    plt.tight_layout()

ani = animation.FuncAnimation(fig, animate, interval=200)
plt.show()