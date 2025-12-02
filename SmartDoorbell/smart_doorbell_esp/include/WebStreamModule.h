/**
 * @file WebStreamModule.h
 * @brief Sets up an HTTP server to stream video.
 * * This module creates a simple web server on port 80. When a client accesses
 * the root URL ("/"), it responds with a continuous stream of JPEG images 
 * (MJPEG format). This allows browsers or the BeagleY-AI to view the live feed.
 */

#ifndef WEB_STREAM_MODULE_H
#define WEB_STREAM_MODULE_H

#include "esp_http_server.h"
#include "esp_camera.h"

// Define the multipart boundary for MJPEG streaming
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

class WebStreamModule {
public:
    /**
     * @brief Starts the web server.
     * * Registers the URI handler for the root path ("/") which serves the video stream.
     */
    void startServer() {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        httpd_handle_t stream_httpd = NULL;

        // Configuration for the streaming endpoint
        httpd_uri_t stream_uri = {
            .uri       = "/",           // Access via http://<IP>/
            .method    = HTTP_GET,
            .handler   = stream_handler, // The function that does the work
            .user_ctx  = NULL
        };

        // Start the server instance
        if (httpd_start(&stream_httpd, &config) == ESP_OK) {
            httpd_register_uri_handler(stream_httpd, &stream_uri);
        }
    }

private:
    /**
     * @brief HTTP Handler for streaming video.
     * * This function runs in a loop, capturing frames from the camera and sending 
     * them as chunks to the connected HTTP client. It continues until the 
     * connection is closed.
     */
    static esp_err_t stream_handler(httpd_req_t *req) {
        camera_fb_t * fb = NULL;
        esp_err_t res = ESP_OK;
        size_t _jpg_buf_len = 0;
        uint8_t * _jpg_buf = NULL;
        char * part_buf[64];

        // Send the initial HTTP header for MJPEG stream
        res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
        if(res != ESP_OK){
            return res;
        }

        // Streaming Loop
        while(true){
            // 1. Capture a frame from the camera
            fb = esp_camera_fb_get();
            if (!fb) {
                Serial.println("Camera capture failed");
                res = ESP_FAIL;
            } else {
                // If the frame is valid (and already JPEG), prepare to send
                if(fb->format != PIXFORMAT_JPEG){
                    // If hardware didn't output JPEG, we would need to convert here
                    // But our config ensures JPEG output.
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if(!jpeg_converted){
                        Serial.println("JPEG compression failed");
                        res = ESP_FAIL;
                    }
                } else {
                    // Frame is already JPEG, use it directly
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
            }
            
            // 2. Send the frame over HTTP
            if(res == ESP_OK){
                size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
                // Send Boundary
                res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
                // Send Content-Type/Length Header
                if(res == ESP_OK){
                    res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
                }
                // Send actual Image Data
                if(res == ESP_OK){
                    res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
                }
            }

            // 3. Clean up frame buffer to free memory for next frame
            if(fb){
                esp_camera_fb_return(fb);
                fb = NULL;
                _jpg_buf = NULL;
            } else if(_jpg_buf){
                free(_jpg_buf);
                _jpg_buf = NULL;
            }

            // If client disconnected or error occurred, break loop
            if(res != ESP_OK){
                break;
            }
        }
        return res;
    }
};

#endif