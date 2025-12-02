/**
 * @file main.cpp
 * @brief Entry point for the ESP32-CAM Smart Doorbell Firmware.
 * * This firmware transforms the ESP32-CAM into a video streaming server.
 * It connects to a configured Wi-Fi network and streams video upon request.
 * The BeagleY-AI board consumes this stream to perform motion detection
 * and capture images.
 */

#include <Arduino.h>
#include "CameraModule.h"
#include "NetworkModule.h"
#include "WebStreamModule.h"

// --- Wi-Fi Credentials ---
// NOTE: These must match the hotspot created by your BeagleY-AI or your home router.
const char* ssid = "MyBeagleNet";
const char* password = "password123";

// --- Module Instances ---
CameraModule camera;
NetworkModule network;
WebStreamModule webStream;

/**
 * @brief Arduino Setup Function
 * Runs once at startup. Initializes all subsystems.
 */
void setup() {
  // 1. Initialize Serial Communication for debugging logs
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // 2. Initialize Camera
  // Configures pins and starts the camera driver
  if (!camera.init()) {
    Serial.println("Camera Init Failed");
    return; // Stop if camera fails
  }

  // 3. Connect to Wi-Fi
  // Blocks until connection is successful
  network.connect(ssid, password);

  // 4. Start the Web Server
  // Begins listening on port 80 for stream requests
  webStream.startServer();
  
  Serial.println("Smart Doorbell Camera Ready!");
  Serial.print("Stream Link: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

/**
 * @brief Arduino Main Loop
 * * Since the web server runs asynchronously (using ESP-IDF's httpd task),
 * the main loop does not need to handle the streaming logic.
 * We can add a small delay to prevent the watchdog from barking.
 */
void loop() {
  delay(10000); 
}