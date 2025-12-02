/**
 * @file CameraModule.h
 * @brief Handles the initialization and configuration of the ESP32-CAM module.
 * * This class encapsulates the complex configuration structure required by the 
 * ESP32-CAM driver. It sets up the specific GPIO pins for the AI-Thinker 
 * ESP32-CAM board model and configures image settings like resolution and format.
 */

#ifndef CAMERA_MODULE_H
#define CAMERA_MODULE_H

#include "esp_camera.h"
#include <Arduino.h>

// --- Pin Definitions for AI-Thinker ESP32-CAM ---
// These specific pins map to the physical connections on the board.
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1 // Software reset
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

class CameraModule {
public:
    /**
     * @brief Initializes the camera hardware.
     * * Configures the camera structure with the defined pins, sets the output
     * format to JPEG (for streaming), and attempts to initialize the driver.
     * * @return true if initialization was successful, false otherwise.
     */
    bool init() {
        camera_config_t config;
        
        // Assign GPIO pins
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer = LEDC_TIMER_0;
        config.pin_d0 = Y2_GPIO_NUM;
        config.pin_d1 = Y3_GPIO_NUM;
        config.pin_d2 = Y4_GPIO_NUM;
        config.pin_d3 = Y5_GPIO_NUM;
        config.pin_d4 = Y6_GPIO_NUM;
        config.pin_d5 = Y7_GPIO_NUM;
        config.pin_d6 = Y8_GPIO_NUM;
        config.pin_d7 = Y9_GPIO_NUM;
        config.pin_xclk = XCLK_GPIO_NUM;
        config.pin_pclk = PCLK_GPIO_NUM;
        config.pin_vsync = VSYNC_GPIO_NUM;
        config.pin_href = HREF_GPIO_NUM;
        config.pin_sscb_sda = SIOD_GPIO_NUM;
        config.pin_sscb_scl = SIOC_GPIO_NUM;
        config.pin_pwdn = PWDN_GPIO_NUM;
        config.pin_reset = RESET_GPIO_NUM;
        
        // Configure Clock
        config.xclk_freq_hz = 20000000;
        
        // Configure Pixel Format
        config.pixel_format = PIXFORMAT_JPEG; // JPEG is required for streaming video
        
        // Select Frame Size
        // FRAMESIZE_SVGA (800x600) offers a good balance between quality and framerate.
        // Higher resolutions (UXGA) may reduce framerate significantly.
        if(psramFound()){
            config.frame_size = FRAMESIZE_SVGA; 
            config.jpeg_quality = 10; // 0-63 lower number means higher quality
            config.fb_count = 2;      // Use 2 frame buffers for smoother streaming if PSRAM is available
        } else {
            config.frame_size = FRAMESIZE_SVGA;
            config.jpeg_quality = 12;
            config.fb_count = 1;
        }

        // Initialize the Camera Driver
        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            Serial.printf("Camera init failed with error 0x%x", err);
            return false;
        }
        return true;
    }
};

#endif