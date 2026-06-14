/**
 * Repository: ESP32 FreeRTOS Dual-Core Non-Blocking Framework
 * Description: A lightweight boilerplate template to separate hardware sensing 
 * and network telemetries across ESP32 cores, preventing data loss.
 * Author: Sugavanam M
 * License: MIT
 */

#include <Arduino.h>

// =========================================================================
// 🛡️ FRAMEWORK ENGINE: DATA STRUCTURE & QUEUE
// =========================================================================

// STEP 1: Define your project's data packet here.
// Users can modify this structure to send their own sensor values (e.g., float temperature, int gasValue)
typedef struct {
    int sensorValue;
    char deviceID[16];
    bool triggerState;
} ProjectPacket_t;

// The Inter-Core Pipeline Bridge
QueueHandle_t globalDataQueue = NULL;

// Forward Declarations for User Logic
void App_HardwareSensingLoop();
void App_NetworkCloudLoop();


// =========================================================================
// ⚙️ FRAMEWORK ENGINE: CORE TASK WRAPPERS
// =========================================================================

// Core 0 Thread Wrapper (Dedicated to Hardware & Sensors)
void Framework_Core0_Engine(void *pvParameters) {
    while(1) {
        App_HardwareSensingLoop(); // Triggers user's custom hardware loop
        vTaskDelay(pdMS_TO_TICKS(10)); // Watchdog reset & context switch
    }
}

// Core 1 Thread Wrapper (Dedicated to WiFi, Delays & Telemetry)
void Framework_Core1_Engine(void *pvParameters) {
    while(1) {
        App_NetworkCloudLoop(); // Triggers user's custom network loop
        vTaskDelay(pdMS_TO_TICKS(10)); // Watchdog reset & context switch
    }
}

// Bootstrapper to initialize the queue and assign physical cores
void Initialize_FreeRTOS_Framework() {
    // Create a thread-safe buffer for 20 packets
    globalDataQueue = xQueueCreate(20, sizeof(ProjectPacket_t));
    
    // Pin tasks to physical processor cores
    xTaskCreatePinnedToCore(Framework_Core0_Engine, "Core0_SensorTask", 4096, NULL, 2, NULL, 0); 
    xTaskCreatePinnedToCore(Framework_Core1_Engine, "Core1_CloudTask", 8192, NULL, 1, NULL, 1);  
}


// =========================================================================
// 🎯 USER APPLICATION LAYER (Users write their specific code here)
// =========================================================================

// --- CORE 0: Fast Sensing (No Delays Allowed Here) ---
void App_HardwareSensingLoop() {
    // USER CODE: Read sensors, buttons, or modules here.
    // Example:
    /*
    bool detected = digitalRead(SENSOR_PIN);
    if (detected) {
        ProjectPacket_t newData;
        newData.sensorValue = analogRead(A0);
        strcpy(newData.deviceID, "NODE_1");
        
        // Push data to Core 1 without blocking Core 0
        xQueueSend(globalDataQueue, &newData, pdMS_TO_TICKS(10));
    }
    */
}

// --- CORE 1: Slow Network/Cloud Processing (Delays are Safe Here) ---
void App_NetworkCloudLoop() {
    ProjectPacket_t receivedData;
    
    // Wait for data. If the network takes 5 seconds to upload, Core 0 is unaffected.
    if (xQueueReceive(globalDataQueue, &receivedData, portMAX_DELAY) == pdPASS) {
        // USER CODE: Handle WiFi, HTTP POST, MQTT, or Database connections here.
        // Example:
        /*
        Serial.printf("Uploading Data: %d from %s\n", receivedData.sensorValue, receivedData.deviceID);
        HTTPClient http;
        http.begin("http://your-cloud-api.com/upload");
        http.POST(String(receivedData.sensorValue));
        http.end();
        */
    }
}


// =========================================================================
// 🚀 MAIN BOOTSTRAP (DO NOT MODIFY)
// =========================================================================
void setup() {
    Serial.begin(115200);
    
    // USER CODE: Initialize pinModes and hardware configurations here
    // pinMode(SENSOR_PIN, INPUT);

    Serial.println("Booting ESP32 Dual-Core Framework...");
    Initialize_FreeRTOS_Framework(); // Launch the RTOS engines
}

void loop() {
    // Framework forces the destruction of the traditional single-threaded loop
    vTaskDelete(NULL); 
}