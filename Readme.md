# ESP32 Smart Alarm Clock

A network-connected smart alarm clock built on the **ESP32 TTGO** using the **ESP-IDF** framework.
The system synchronizes real-world time using **SNTP**, receives remote alarm schedules through **MQTT**, and drives a graphical display with a custom embedded UI.

Project characteristics:

* Embedded systems programming in C
* Event-driven firmware architecture
* ESP32 networking (WiFi + TCP/IP)
* MQTT communication
* SNTP time synchronization
* GPIO interrupts
* Real-time state machines
* Low-level hardware interaction
* Embedded graphics rendering

---

## Features

* WiFi connectivity using ESP-IDF networking stack
* Automatic internet time synchronization via SNTP
* Remote alarm scheduling over MQTT
* Real-time clock display with date/time formatting
* Visual alarm notification with animated blinking screen
* GPIO interrupt handling for alarm dismissal
* Event-driven initialization sequence
* Embedded UI rendering pipeline
* Finite state machine architecture
* Modular firmware structure

---

## System Architecture

```text
                 +-------------------+
                 |   MQTT Publisher  |
                 |  (Remote Device)  |
                 +---------+---------+
                           |
                           v
+------------+     +---------------+      +----------------+
|   WiFi AP  +----->   ESP32 STA   +----->  MQTT Broker    |
+------------+     +---------------+      +----------------+
                           |
                           v
                 +-------------------+
                 |  Alarm Firmware   |
                 |-------------------|
                 | SNTP Time Sync    |
                 | Alarm Scheduler   |
                 | Graphics Renderer |
                 | GPIO Interrupts   |
                 +---------+---------+
                           |
                           v
                 +-------------------+
                 | Display + Alarm   |
                 +-------------------+
```

---

# Firmware Design

## State Machine

The firmware operates using a finite state machine.

```text
        +------+
        | INIT |
        +--+---+
           |
           v
   Connect WiFi/SNTP/MQTT
           |
           v
      +----+-----+
      | PROCESS  |
      +----+-----+
           |
   Alarm time reached
           |
           v
     +-----+------+
     | ALARM_ON   |
     +-----+------+
           |
   Button interrupt
           |
           v
      +----+-----+
      | PROCESS  |
      +----------+
```

---

# Key Technical Insights

## WiFi Networking

The project configures the ESP32 in **station mode** and connects to a wireless access point using the ESP-IDF WiFi APIs.

Implemented features include:

* Event-based connection handling
* Authentication failure detection
* Dynamic reconnection logic
* Event groups for synchronization
* TCP/IP stack initialization

Relevant module:

```text
wifi.c
```

---

## SNTP Time Synchronization

The alarm clock retrieves accurate real-world time from NTP servers.

```c
esp_sntp_setservername(0, "pool.ntp.org");
```

Also New Zealand timezone is configured with daylight savings support.

---

## MQTT Remote Alarm Scheduling

The ESP32 subscribes to:

```text
esp32/alarm
```

Incoming MQTT messages use the format:

```text
YYYY-MM-DD HH:MM
```

Example:

```text
2026-05-20 07:30
```

The firmware parses the payload and updates the alarm schedule dynamically.

---

## Graphics Rendering

The UI is rendered directly into a framebuffer.

Features include:

* Custom background rendering
* Dynamic text rendering
* Time/date formatting
* Animated alarm flashing effect
* Double buffering via `flip_frame()`

The alarm screen alternates between red and green backgrounds during activation.

---

## GPIO Interrupt Handling

Hardware interrupts are used for responsive button input.

```c
gpio_isr_handler_add(0, gpio_isr_handler, (void*) 0);
```

Features:

* Interrupt-driven alarm dismissal
* Edge-triggered GPIO handling
* ISR-safe timing logic

---

# Technologies Used

* Language: C
* Framework: ESP-IDF
* RTOS: FreeRTOS
* Networking: WiFi + TCP/IP
* Messaging Protocol: MQTT
* Time Sync: SNTP/NTP
* Hardware Platform: ESP32

---

# Example Alarm Workflow

## 1. Device Boots

The ESP32 initializes:

* NVS
* WiFi
* Event handlers
* Graphics subsystem

---

## 2. Network Connection

The firmware:

1. Connects to WiFi
2. Synchronizes time using SNTP
3. Connects to the MQTT broker

---

## 3. Alarm Scheduling

A remote device publishes:

```text
2026-05-20 07:30
```

to:

```text
esp32/alarm
```

The ESP32 stores the alarm configuration internally.

---

## 4. Alarm Activation

When the current synchronized time matches the scheduled alarm:

* Alarm state changes to `ALARM_ON`
* GPIO output is activated
* Display begins flashing

---

## 5. User Dismissal

A GPIO interrupt is triggered when the user presses a button.

Then it:

* Clears the alarm output
* Invalidates the active alarm
* Returns to normal processing state

---

# What I learned

* Embedded firmware engineering
* Real-time systems
* Event-driven programming
* ESP32 development
* Embedded networking
* MQTT systems
* Time synchronization protocols
* Interrupt programming
* Hardware register manipulation
* State machine architecture
* Modular C design
* Embedded UI rendering

---

# Future Improvements

Potential future enhancements:

* Persistent alarm storage using NVS
* Multiple simultaneous alarms
* OLED or TFT touch UI
* Secure MQTT using TLS
* Audio buzzer integration
* Power-saving deep sleep mode

---

# Build Instructions

## Requirements

* ESP-IDF
* ESP32 toolchain
* CMake
* Python environment for ESP-IDF
* Or PlatformIO

---

## Build

```bash
idf.py build
```

---

## Flash

```bash
idf.py -p /dev/ttyUSB0 flash
```

---

## Monitor

```bash
idf.py monitor
```

---

# Hardware Requirements

* ESP32 Tdisplay (TTGO) Development Board
* Display module
* Push buttons
* WiFi network access
* Optional external alarm/buzzer hardware

---

# Project Dimensions

This project combines:

* networking,
* real-time systems,
* communication protocols,
* interrupt-driven input,
* and graphical rendering

---

# Demo



https://github.com/user-attachments/assets/7c834a3a-ac2a-4a7c-bdce-c946a7322cae


