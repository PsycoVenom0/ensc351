/**
 * @file NetworkModule.h
 * @brief Manages the Wi-Fi connection.
 * * This class provides a simple wrapper around the Arduino WiFi library to 
 * connect the ESP32 to a wireless access point. It handles the connection 
 * process and blocks execution until a connection is established.
 */

#ifndef NETWORK_MODULE_H
#define NETWORK_MODULE_H

#include <WiFi.h>

class NetworkModule {
public:
    /**
     * @brief Connects to the specified Wi-Fi network.
     * * @param ssid The name of the Wi-Fi network.
     * @param password The password for the network.
     */
    void connect(const char* ssid, const char* password) {
        WiFi.begin(ssid, password);
        
        Serial.print("Connecting to WiFi");
        // Wait loop until connected
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP()); // Print IP to Serial Monitor for debugging
    }
};

#endif