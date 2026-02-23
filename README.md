# STM32 CAN Bus Distributed System

A distributed embedded system using two STM32F446RE microcontrollers 
communicating over CAN bus, both running FreeRTOS.

## System Architecture
<img width="415" height="693" alt="stm32-can-based-distributed-system drawio" src="https://github.com/user-attachments/assets/1bf699ab-ea99-4137-95e2-d91561b015b4" />


## Skills Demonstrated
- **FreeRTOS** — Multiple tasks, queues, ISR-to-task communication
- **CAN Bus** — 500 kbit/s, STD frame format, RX interrupt, TX mailbox
- **UART/USART** — Serial logging at 115200 baud
- **STM32 HAL** — CAN, UART, GPIO, Timer drivers
- **SWD/JTAG** — ST-Link debugging, breakpoints, live expressions
- **Interrupts** — CAN RX FIFO interrupt feeding FreeRTOS queue

## Hardware
| Component | Quantity |
|---|---|
| NUCLEO-F446RE | 2x |
| TJA1050 CAN Transceiver | 2x |
| 120Ω Termination Resistor | 2x |
| Breadboard | 1x |
| Dupont Jumper Cables | As needed |

## CAN Frame Definitions
| Message | CAN ID | Bytes | Description |
|---|---|---|---|
| RPM | 0x100 | 2 | Engine RPM (0-6000) |
| Temperature | 0x101 | 2 | Temperature in °C |
| Status | 0x102 | 1 | Node status byte |
| ACK | 0x200 | 1 | Acknowledgement from Node B |

## Node A — Commander
Simulates an ECU transmitting sensor data over CAN every 100ms and listening for ACK frames from Node B.

FreeRTOS Tasks:
- `vCANTransmitTask` — Sends RPM, TEMP, STATUS every 100ms
- `vCANReceiveTask` — Receives STATUS and ACK responses from Node B
- `vHeartbeatTask` — Blinks LED every 500ms
- `vUARTLogTask` — Handles UART log queue

## Node B — Responder
Receives CAN frames, checks thresholds, sends dedicated ACK frames back to Node A.

FreeRTOS Tasks:
- `vCANReceiveTask` — Processes incoming frames from ISR queue
- `vCANTransmitTask` — Sends periodic status keepalive responses
- `vHeartbeatTask` — Blinks LED every 500ms
- `vUARTLogTask` — Handles UART log queue

## Wiring Guide
### TJA1050 to STM32
| TJA1050 Pin | STM32 Pin | Description |
|---|---|---|
| CTX | PA12 | CAN TX |
| CRX | PA11 | CAN RX |
| VCC | 3.3V | Power |
| GND | GND | Ground |

### CAN Bus Termination
120Ω resistor between CANH and CANL on each end of the bus.

## How To Build and Flash
1. Open `NodeA` project in STM32CubeIDE
2. Press `Ctrl+B` to build
3. Connect NUCLEO-F446RE via USB
4. Press `F11` to flash and debug
5. Repeat for `NodeB` on second NUCLEO

## UART Output Example
### Node A
```
[SYSTEM] Node A starting...
[CAN] Initialized OK
[HEARTBEAT] Task started
[CAN_TX] Task started
[CAN] TX RPM: 900
[CAN] TX TEMP: 26
[CAN] TX STATUS: 1
```
### Node B
```
[SYSTEM] Node B starting...
[CAN] Initialized OK
[CAN_RX] RPM: 900
[CAN_RX] TEMP: 26
[CAN_RX] STATUS: 1
```

## Debug Setup
- Debugger: ST-Link SWD
- IDE: STM32CubeIDE
- FreeRTOS Task List view used to monitor CPU usage per task
- SWV/ITM used for zero-overhead profiling


## Author
Sumanth Gosi
