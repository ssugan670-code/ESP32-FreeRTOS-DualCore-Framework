if (xQueueReceive(globalDataQueue, &receivedData, portMAX_DELAY) == pdPASS) {
    // Upload receivedData to Google Sheets, AWS, or Firebase here!
}