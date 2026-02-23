# STM32 CAN Bus Distributed System

A distributed embedded system demonstrating command-based CAN bus communication between two STM32F446RE microcontrollers running FreeRTOS. Features threshold detection, explicit ACK protocol, and timeout-based fault detection.

## System Architecture
<img width="415" height="693" alt="stm32-can-based-distributed-system drawio" src="https://github.com/user-attachments/assets/1bf699ab-ea99-4137-95e2-d91561b015b4" />

## Skills Demonstrated
- **FreeRTOS** — Multiple tasks, message queues, ISR-to-task communication, semaphores
- **CAN Bus Protocol Design** — Command/data separation, explicit ACK, timeout handling
- **CAN Bus Hardware** — 500 kbit/s, STD frame format, RX interrupt, TX mailbox management
- **UART/USART** — Serial logging at 115200 baud for debugging
- **STM32 HAL** — CAN, UART, GPIO, Timer peripheral drivers
- **SWD/JTAG Debugging** — ST-Link, breakpoints, live expressions, task monitoring
- **Interrupt Handling** — CAN RX FIFO interrupt feeding FreeRTOS queue
- **Fault Detection** — ACK timeout detection with configurable retry logic

## Hardware
| Component | Quantity | Purpose |
|---|---|---|
| NUCLEO-F446RE | 2x | STM32 development boards |
| TJA1050 CAN Transceiver | 2x | CAN physical layer interface |
| 120Ω Metal Film Resistor | 2x | CAN bus termination |
| Breadboard | 1x | Circuit assembly |
| Jumper Cables (M/M, F/F, M/F) | 1 set | Interconnections |
| Mini-USB Cable | 2x | Power, programming, UART |

## Protocol Design

### Message Types

| Category | CAN ID | Description | ACK Required? |
|---|---|---|---|
| **Data Broadcasting** | | | |
| RPM | 0x100 | Engine RPM (0-6000) | ❌ No |
| Temperature | 0x101 | Temperature in °C | ❌ No |
| Heartbeat | 0x102 | Alive signal | ❌ No |
| **Command & Control** | | | |
| Command | 0x200 | Action request from Node B | ✅ Yes |
| ACK | 0x201 | Acknowledgement from Node A | N/A |

### Command Codes

| Code | Name | Description |
|---|---|---|
| 0x01 | CMD_WARNING_HIGH_RPM | RPM exceeded 5000 threshold |
| 0x02 | CMD_WARNING_HIGH_TEMP | Temperature exceeded 80°C threshold |
| 0x03 | CMD_REDUCE_POWER | Request power reduction |
| 0x04 | CMD_ACTIVATE_COOLING | Request cooling activation |

### Why Commands Need ACK But Data Doesn't

**Periodic Data (RPM/TEMP/Heartbeat):**
- Transmitted every 100ms continuously
- If one frame is lost, next arrives shortly
- Receiver eventually gets updated data
- **Loss tolerance: HIGH** ✅

**Commands:**
- Sent once when condition triggers
- If lost, Node A never knows action is required
- Critical for safety and correct operation
- **Loss tolerance: ZERO** ❌ → Requires ACK + timeout

This mirrors real automotive protocols like CANopen and J1939.

## Node A — Sensor Node (Data Broadcaster)

Simulates an ECU with sensors, broadcasting data periodically.

### FreeRTOS Tasks
- **vCANTransmitTask** — Broadcasts RPM, TEMP, HEARTBEAT every 100ms
- **vCANReceiveTask** — Listens for COMMAND frames from Node B, sends ACK, executes action
- **vHeartbeatTask** — Blinks onboard LED every 500ms (scheduler health indicator)
- **vUARTLogTask** — Handles UART logging queue

### Behavior
1. Continuously broadcasts sensor data (no ACK expected)
2. When COMMAND received:
   - Immediately sends ACK
   - Executes commanded action (log, LED, reduce power, etc.)
3. Does NOT evaluate thresholds — just reports raw data

## Node B — Controller Node (Decision Maker)

Receives data from Node A, evaluates thresholds, sends commands when limits exceeded.

### FreeRTOS Tasks
- **vCANReceiveTask** — Processes data frames, checks thresholds, sends commands with timeout
- **vCANTransmitTask** — Optional heartbeat or status reporting
- **vHeartbeatTask** — Blinks onboard LED every 500ms
- **vUARTLogTask** — Handles UART logging queue

### Behavior
1. Receives RPM/TEMP data continuously
2. Evaluates thresholds:
   - RPM > 5000 → Send CMD_WARNING_HIGH_RPM
   - TEMP > 80°C → Send CMD_WARNING_HIGH_TEMP
3. Waits for ACK with 200ms timeout
4. If timeout → Logs error, can retry or escalate

### ACK Timeout Handling
```c
CAN_App_TransmitCommand(CMD_WARNING_HIGH_RPM);

// Wait for ACK with 200ms timeout
ackReceived = false;
uint32_t startTime = osKernelGetTickCount();

while(!ackReceived && (osKernelGetTickCount() - startTime) < 200) {
    osDelay(10);
}

if(!ackReceived) {
    UART_Log("ERROR", "Node A did not ACK command!");
    // Could retry, increment error counter, trigger fault handler
}
```

## Wiring Guide

### TJA1050 Module to NUCLEO

| TJA1050 Pin | NUCLEO Pin | Description |
|---|---|---|
| TxD | PA12 (CN10 Pin 12) | CAN TX |
| RxD | PA11 (CN10 Pin 14) | CAN RX |
| VCC | 3.3V (CN6 Pin 4) | Power supply |
| GND | GND (CN6 Pin 6) | Ground |

**Wire both modules identically to their respective NUCLEO boards.**

### CAN Bus Connection

Connect between the two TJA1050 modules:
```
Module A CANH ──────────────── Module B CANH
Module A CANL ──────────────── Module B CANL
```

### Termination Resistors

Critical for signal integrity:
- **120Ω resistor** between CANH and CANL on **Module A**
- **120Ω resistor** between CANH and CANL on **Module B**

Without proper termination, expect communication errors or complete failure.

### Physical Layout
```
Breadboard Setup:
┌──────────────────────────────────────┐
│  TJA1050 A          TJA1050 B        │
│  ┌──────┐           ┌──────┐        │
│  │ Pins │           │ Pins │        │
│  └──┬───┘           └──┬───┘        │
│     │                  │             │
│   120Ω               120Ω           │
│  (CANH-CANL)      (CANH-CANL)      │
│     │                  │             │
│  ───┴──────────────────┴───         │
│    CANH ←───────────→ CANH          │
│    CANL ←───────────→ CANL          │
└──────────────────────────────────────┘
        │                │
     NUCLEO A        NUCLEO B
```

## How To Build and Flash

### Build
1. Open NodeA project in STM32CubeIDE
2. Press `Ctrl+B` to build
3. Verify `0 errors, 0 warnings`
4. Repeat for NodeB project

### Flash
1. Connect NUCLEO-F446RE via Mini-USB
2. Press `F11` to flash and debug
3. Press `F8` to resume execution
4. Repeat for second board

### View UART Output (macOS)

Find port:
```bash
ls /dev/tty.*
```

Connect:
```bash
screen /dev/tty.usbmodemXXXXX 115200
```

Exit: `Ctrl+A` then `K` then `Y`

## Expected Output

### Node A Terminal (Normal Operation)
```
[SYSTEM] Node A starting...
[CAN] Initialized OK
[CAN_TX] Task started
[CAN_TX] RPM: 900
[CAN_TX] TEMP: 26
[CAN_TX] Heartbeat
[HEARTBEAT] Alive
[CAN_TX] RPM: 1000
[CAN_TX] TEMP: 27
```

### Node B Terminal (Normal Operation)
```
[SYSTEM] Node B starting...
[CAN] Initialized OK
[CAN_RX] Task started
[CAN_RX] RPM: 900
[CAN_RX] TEMP: 26
[CAN_RX] Heartbeat from Node A
[CAN_RX] RPM: 1000
[CAN_RX] TEMP: 27
```

### When Threshold Violated (RPM > 5000)

**Node B detects and commands:**
```
[CAN_RX] RPM: 5100
[WARNING] RPM threshold exceeded!
[CAN_TX] COMMAND: 0x01
[CAN_RX] ACK received for command: 0x01
```

**Node A receives and acknowledges:**
```
[CAN_TX] RPM: 5100
[CAN_RX] Command received
[COMMAND] Node B detected HIGH RPM!
[CAN_TX] ACK: 0x01
```

### When ACK Times Out (Fault Condition)
```
[Node B Output]
[CAN_TX] COMMAND: 0x02
[ERROR] Node A did not ACK command!
```

This demonstrates the fault detection capability.

## Python Live Dashboard

A real-time plotting tool for visualizing CAN data:

### Run Dashboard
```bash
python3 python/dashboard.py
```

### Features
- Live scrolling graphs for RPM and Temperature
- Visual threshold indicators (red dashed lines)
- Simultaneous UART terminal output
- Configurable data retention window

### Requirements
```bash
pip3 install pyserial matplotlib
```

## Debug Setup

### Tools Used
- **Debugger:** ST-Link SWD
- **IDE:** STM32CubeIDE
- **RTOS Debugging:** FreeRTOS Task List view for CPU usage monitoring
- **Protocol Analysis:** UART logs + optional CAN bus analyzer

### Debug Workflow Examples

**Set breakpoint in CAN RX ISR:**
```c
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    // Breakpoint here to inspect incoming frames
}
```

**Watch FreeRTOS queue depth:**
```
Add to Live Expressions: canRxQueueHandle
```

**Trigger HardFault intentionally (for learning):**
```c
uint32_t *badPtr = (uint32_t*)0xFFFFFFFF;
*badPtr = 0x1234;  // Causes HardFault
```

Then use stack trace to diagnose.

## Project Structure
```
stm32-can-distributed-system/
├── NodeA/
│   ├── Core/
│   │   ├── Inc/
│   │   │   ├── can_app.h       # CAN protocol definitions
│   │   │   ├── uart_log.h      # Logging interface
│   │   │   └── tasks.h         # FreeRTOS task declarations
│   │   └── Src/
│   │       ├── can_app.c       # CAN TX/RX implementation
│   │       ├── uart_log.c      # UART wrapper
│   │       ├── tasks.c         # Sensor node tasks
│   │       └── main.c          # Init and scheduler start
│   └── NodeA.ioc               # CubeMX configuration
├── NodeB/                      # Same structure as NodeA
├── python/
│   └── dashboard.py            # Live data visualization
├── docs/
│   └── architecture.png        # System diagram
└── README.md
```

### When To Use ACK

| Scenario | ACK? | Rationale |
|---|---|---|
| Periodic sensor data | ❌ | Loss acceptable, next frame arrives soon |
| Heartbeat/alive signals | ❌ | Informational, not critical |
| Command to take action | ✅ | Critical, must confirm receipt |
| Configuration change | ✅ | One-time, must succeed |
| Bootloader entry | ✅ | Safety-critical state change |


## Future Enhancements

- [ ] Implement retry logic on ACK timeout
- [ ] Add CAN bus-off detection and recovery
- [ ] Implement bootloader over CAN
- [ ] Add DBC file for message definitions
- [ ] Expand command set (shutdown, reconfigure, etc.)
- [ ] Add encryption/authentication for commands
- [ ] Implement CANopen-lite PDO/SDO
- [ ] Add SD card logging of all CAN traffic

## What I Learned

- **Protocol Design:** Understanding when ACKs are necessary vs. wasteful overhead
- **Real-Time Constraints:** Why periodic data doesn't need confirmation but commands do
- **Fault Tolerance:** Timeout-based error detection and recovery strategies
- **FreeRTOS Patterns:** ISR-to-task communication via queues, volatile flag usage
- **Debugging Embedded Systems:** Using UART logs, breakpoints, and live variable inspection

## References

- [STM32F446RE Datasheet](https://www.st.com/resource/en/datasheet/stm32f446re.pdf)
- [TJA1050 CAN Transceiver Datasheet](https://www.nxp.com/docs/en/data-sheet/TJA1050.pdf)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- [CAN Bus Specification (ISO 11898)](https://www.iso.org/standard/63648.html)
- [CANopen Protocol](https://www.can-cia.org/canopen/)


## Author

**Sumanth Gosi**  
Embedded Systems Engineer  
[GitHub Repository](https://github.com/yourusername/stm32-can-distributed-system)

---

*This project demonstrates production-ready embedded system design principles including protocol architecture, fault tolerance, and real-time operating system integration.*
=======
Sumanth Gosi
