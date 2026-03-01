import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import re
import threading
from datetime import datetime

# Configuration 
PORT = '/dev/tty.usbmodem1203'  #Node B
BAUD = 115200
MAX_POINTS = 100

# Data
rpm_data = deque([0] * MAX_POINTS, maxlen=MAX_POINTS)
temp_data = deque([0] * MAX_POINTS, maxlen=MAX_POINTS)

# Recent events (just keep last 10)
recent_events = deque(maxlen=10)

# Counters
rpm_cmd_count = 0
temp_cmd_count = 0
ack_count = 0

# Serial
try:
    ser = serial.Serial(PORT, BAUD, timeout=1)
    print(f"Connected to {PORT}")
except Exception as e:
    print(f"Error: {e}")
    exit()

# Parse
def parse_line(line):
    global rpm_cmd_count, temp_cmd_count, ack_count
    
    if 'RPM:' in line and 'WARNING' not in line:
        rpm_match = re.search(r'RPM:\s*(\d+)', line)
        if rpm_match:
            rpm_data.append(int(rpm_match.group(1)))
            
    if 'TEMP:' in line and 'WARNING' not in line:
        temp_match = re.search(r'TEMP:\s*(-?\d+)', line)
        if temp_match:
            temp_data.append(int(temp_match.group(1)))
    
    timestamp = datetime.now().strftime("%H:%M:%S")
    
    if 'COMMAND: 1' in line:
        rpm_cmd_count += 1
        recent_events.append(f"[{timestamp}] RPM CMD #{rpm_cmd_count}")
        
    if 'COMMAND: 2' in line:
        temp_cmd_count += 1
        recent_events.append(f"[{timestamp}] TEMP CMD #{temp_cmd_count}")
    
    if 'ACK received' in line:
        ack_count += 1
        recent_events.append(f"[{timestamp}] ACK #{ack_count}")

# Background reader
def read_serial():
    while True:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                parse_line(line)
        except:
            pass

thread = threading.Thread(target=read_serial, daemon=True)
thread.start()

# Simple layout
fig = plt.figure(figsize=(16, 10))
gs = fig.add_gridspec(3, 2, height_ratios=[2, 2, 2], hspace=0.3, wspace=0.3)

ax1 = fig.add_subplot(gs[0, :])  # RPM
ax2 = fig.add_subplot(gs[1, :])  # TEMP
ax3 = fig.add_subplot(gs[2, 0])  # Stats
ax4 = fig.add_subplot(gs[2, 1])  # Recent events

fig.suptitle('STM32 CAN Bus Monitor', fontsize=18, fontweight='bold')

def animate(i):
    # RPM GRAPH
    ax1.clear()
    ax1.plot(list(rpm_data), color='#2E86DE', linewidth=3)
    ax1.axhline(y=5000, color='red', linestyle='--', linewidth=2)
    ax1.fill_between(range(MAX_POINTS), 5000, 7000, alpha=0.1, color='red')
    ax1.set_title(f'RPM (Commands sent: {rpm_cmd_count})', fontsize=14, fontweight='bold')
    ax1.set_ylabel('RPM', fontsize=12)
    ax1.set_ylim(0, 7000)
    ax1.grid(True, alpha=0.3)

    # TEMP GRAPH
    ax2.clear()
    ax2.plot(list(temp_data), color='#10AC84', linewidth=3)
    ax2.axhline(y=80, color='red', linestyle='--', linewidth=2)
    ax2.fill_between(range(MAX_POINTS), 80, 120, alpha=0.1, color='red')
    ax2.set_title(f'Temperature (Commands sent: {temp_cmd_count})', fontsize=14, fontweight='bold')
    ax2.set_ylabel('Celsius', fontsize=12)
    ax2.set_ylim(0, 120)
    ax2.grid(True, alpha=0.3)

    # STATS
    ax3.clear()
    ax3.axis('off')
    
    current_rpm = rpm_data[-1] if rpm_data else 0
    current_temp = temp_data[-1] if temp_data else 0
    
    rpm_status = "DANGER" if current_rpm > 5000 else "NORMAL"
    rpm_color = "#FF6B6B" if current_rpm > 5000 else "#90EE90"
    
    temp_status = "OVERHEAT" if current_temp > 80 else "NORMAL"
    temp_color = "#FF6B6B" if current_temp > 80 else "#90EE90"
    
    stats_text = f"""
    CURRENT STATUS
    
    RPM:  {current_rpm}  ({rpm_status})
    TEMP: {current_temp}C  ({temp_status})
    
    ------------------------
    TOTAL ACTIVITY
    
    RPM Commands:  {rpm_cmd_count}
    TEMP Commands: {temp_cmd_count}
    ACKs Received: {ack_count}
    """
    
    ax3.text(0.5, 0.5, stats_text, ha='center', va='center', fontsize=13,
             family='monospace',
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

    # RECENT EVENTS
    ax4.clear()
    ax4.axis('off')
    
    ax4.text(0.5, 0.95, 'RECENT EVENTS', ha='center', fontsize=14, fontweight='bold')
    
    y_pos = 0.85
    for event in list(recent_events)[-8:]:
        ax4.text(0.5, y_pos, event, ha='center', fontsize=11, family='monospace')
        y_pos -= 0.1
    
    if not recent_events:
        ax4.text(0.5, 0.5, 'Waiting for events...', ha='center', 
                fontsize=12, style='italic', color='gray')
    
    plt.tight_layout()

ani = animation.FuncAnimation(fig, animate, interval=500)
plt.show()
