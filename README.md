# ESP32 FreeRTOS Dual-Core Non-Blocking Framework

![Platform](https://img.shields.io/badge/Platform-ESP32-red?style=flat-square&logo=espressif)
![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS-green?style=flat-square)
![Language](https://img.shields.io/badge/Language-C%2B%2B-blue?style=flat-square&logo=cplusplus)
![IDE](https://img.shields.io/badge/IDE-Arduino_IDE-teal?style=flat-square&logo=arduino)
![License](https://img.shields.io/badge/License-MIT-lightgrey?style=flat-square)

> A lightweight, plug-and-play boilerplate that eliminates blocking delays in ESP32 IoT projects — by splitting hardware and network tasks across two physical CPU cores using FreeRTOS.

**Author:** Sugavanam M

---

## 🛑 The Problem: Single-Core Bottleneck

In standard Arduino/ESP32 development, the `loop()` function executes tasks **sequentially**. When your device tries to push data to a cloud endpoint (HTTP POST, MQTT publish, Firebase write), network latency causes the **entire microcontroller to stall** for several seconds.

During this freeze window — any incoming sensor readings, hardware interrupts, or button presses are **permanently lost**.

```
Standard Arduino Flow (Broken):

[Read Sensor] ──► [Upload to Cloud] ──► STALL (2–5 sec) ──► [Read Sensor] ──► ...
                                             ↑
                              Hardware events missed here ❌
```

This is the single biggest reliability issue in real-world IoT deployments.

---

## 💡 The Solution: Asynchronous Dual-Core Architecture

This framework solves the bottleneck by pinning tasks to the ESP32's **two independent physical cores** via FreeRTOS, with a thread-safe inter-core pipeline connecting them.

```
┌─────────────────────────────────────────────────────────┐
│                     ESP32 Chip                          │
│                                                         │
│  ┌──────────────────┐       ┌──────────────────────┐   │
│  │   CORE 0         │       │   CORE 1             │   │
│  │  Hardware Thread │       │  Telemetry Thread    │   │
│  │                  │       │                      │   │
│  │  · Sensor Poll   │──────►│  · WiFi / MQTT       │   │
│  │  · GPIO Monitor  │ Queue │  · Cloud Uploads     │   │
│  │  · Interrupts    │(RTOS) │  · Database Writes   │   │
│  │                  │       │                      │   │
│  │  Never sleeps ✅ │       │  Network lag safe ✅ │   │
│  └──────────────────┘       └──────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

| Core | Role | Characteristics |
|------|------|----------------|
| **Core 0** | Hardware Thread | Continuous sensor polling, GPIO interrupts — never blocked |
| **Core 1** | Telemetry Thread | WiFi negotiation, cloud uploads, DB writes — runs asynchronously |
| **QueueHandle_t** | Inter-Core Pipeline | Thread-safe data transfer — no race conditions, no memory corruption |

---

## ⚙️ How to Use This Template

You do **not** need to understand RTOS internals to use this. Just drop your existing code into the designated application blocks — the framework handles all the concurrency.

---

### Step 1 — Define Your Data Packet

Modify the `ProjectPacket_t` struct to match the variables your sensor produces:

```cpp
typedef struct {
    float temperature;
    float humidity;
    int   objectDetected;
    long  timestamp;
} ProjectPacket_t;
```

---

### Step 2 — Write Your Sensor Logic (Core 0)

Paste your `digitalRead()`, `analogRead()`, or module polling code inside `App_HardwareSensingLoop()`. When an event occurs, push it to the inter-core queue:

```cpp
void App_HardwareSensingLoop(void *pvParameters) {
    ProjectPacket_t packet;

    while (true) {
        // Your sensor code here
        packet.temperature   = readTemperatureSensor();
        packet.objectDetected = digitalRead(IR_PIN);

        // Push to inter-core pipeline (non-blocking, 10ms timeout)
        xQueueSend(globalDataQueue, &packet, pdMS_TO_TICKS(10));

        vTaskDelay(pdMS_TO_TICKS(100)); // Poll every 100ms
    }
}
```

---

### Step 3 — Write Your Cloud Logic (Core 1)

Paste your WiFi and cloud upload code inside `App_NetworkCloudLoop()`. This task **automatically blocks** until data arrives from Core 0 — consuming zero CPU while waiting:

```cpp
void App_NetworkCloudLoop(void *pvParameters) {
    ProjectPacket_t receivedData;

    while (true) {
        // Blocks here until Core 0 sends data — no polling needed
        if (xQueueReceive(globalDataQueue, &receivedData, portMAX_DELAY) == pdPASS) {

            // Upload to your cloud platform here:
            uploadToGoogleSheets(receivedData);
            // OR: publishMQTT(receivedData);
            // OR: firebaseWrite(receivedData);
        }
    }
}
```

---

### Step 4 — Initialize in `setup()`

```cpp
void setup() {
    Serial.begin(115200);

    // Create the inter-core queue (holds up to 10 packets)
    globalDataQueue = xQueueCreate(10, sizeof(ProjectPacket_t));

    // Pin tasks to specific physical cores
    xTaskCreatePinnedToCore(App_HardwareSensingLoop, "HW_Task",  4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(App_NetworkCloudLoop,    "Net_Task", 4096, NULL, 1, NULL, 1);

    // Kill the default Arduino loop() task (saves clock cycles)
    vTaskDelete(NULL);
}

void loop() { /* Intentionally empty — FreeRTOS tasks take over */ }
```

---

## 📊 Performance Comparison

| Metric | Standard `loop()` | This Framework |
|--------|:-----------------:|:--------------:|
| Sensor data loss during upload | ❌ Yes | ✅ Zero |
| Network stall affects hardware | ❌ Yes | ✅ Isolated |
| Code complexity | Low | Low (abstracted) |
| CPU utilization | ~100% busy wait | Optimized (task blocking) |
| Scalable to more tasks | ❌ No | ✅ Yes |

---

## 🛠️ Features

- **Zero Data Loss** — Hardware triggers are captured even under heavy network load
- **Plug & Play** — RTOS complexity is fully abstracted; just fill in your application code
- **True Parallelism** — Two tasks run simultaneously on two separate physical CPU cores
- **Thread-Safe Pipeline** — `QueueHandle_t` prevents race conditions and memory corruption
- **Power Efficient** — Default Arduino `loop()` task is killed, freeing unnecessary clock cycles
- **Cloud Agnostic** — Works with Google Sheets, Firebase, AWS IoT, MQTT, and any HTTP endpoint

---

## 🗂️ Project Structure

```
ESP32_DualCore_Framework/
├── ESP32_DualCore_Framework.ino   # Main entry point — setup() and task init
├── app_hardware.cpp               # Core 0: sensor polling & hardware logic
├── app_network.cpp                # Core 1: WiFi, cloud upload & telemetry logic
├── config.h                       # ProjectPacket_t struct & global queue handle
└── README.md
```

---

## 🧰 Tech Stack

| Category | Tool / Technology |
|----------|-----------------|
| Microcontroller | ESP32 (Dual-Core Xtensa LX6, 240 MHz) |
| Real-Time OS | FreeRTOS (built into ESP-IDF / Arduino Core) |
| IDE | Arduino IDE / VS Code + PlatformIO |
| Language | C++ (Arduino Framework) |
| Concurrency Primitives | `QueueHandle_t`, `xTaskCreatePinnedToCore()` |

---

## 🧠 Key Learning Outcomes

- Understood the root cause of blocking delays in single-threaded MCU firmware
- Implemented true hardware-software task isolation using FreeRTOS on dual-core ESP32
- Used `QueueHandle_t` as a thread-safe inter-core communication channel
- Designed a reusable, abstracted firmware template applicable to any IoT use case

---

## 🤝 Contributing

Feel free to fork this repository and use this structural blueprint as the foundation for your own IoT projects. Pull requests for improvements, additional cloud integrations, or hardware drivers are welcome!

---

## 📄 License

This project is licensed under the **MIT License** — free to use, modify, and distribute for educational and personal projects.

---

<p align="center">
  Built to never miss a sensor tick — <strong>Sugavanam M</strong>
</p>
