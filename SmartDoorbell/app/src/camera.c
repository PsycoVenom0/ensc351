#include "camera.h"
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <string.h>
#include <math.h>

#define IMG_PATH "/tmp/visitor.jpg" // RAM disk for speed
#define MOTION_THRESH 0.02          // 2% pixel change triggers motion
#define PIXEL_THRESH 40             // Sensitivity

static unsigned char* bg_buffer = NULL;
static int img_w = 0, img_h = 0;

// Helper: Decode JPEG to RGB
unsigned char* load_jpeg(const char* filename, int* w, int* h) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE* infile = fopen(filename, "rb");
    if (!infile) return NULL;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    *w = cinfo.output_width;
    *h = cinfo.output_height;
    unsigned char* buf = malloc((*w) * (*h) * cinfo.output_components);
    unsigned char* rowptr[1];

    while (cinfo.output_scanline < cinfo.output_height) {
        rowptr[0] = &buf[cinfo.output_scanline * (*w) * cinfo.output_components];
        jpeg_read_scanlines(&cinfo, rowptr, 1);
    }
    
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return buf;
}

void camera_init(void) { system("rm -f " IMG_PATH); }

int camera_capture(const char* ip) {
    char cmd[256];
    // wget with 1s timeout to prevent blocking logic
    snprintf(cmd, sizeof(cmd), "wget -q -O %s -T 1 http://%s/capture", IMG_PATH, ip);
    return system(cmd);
}

bool camera_check_motion(void) {
    int w, h;
    unsigned char* curr = load_jpeg(IMG_PATH, &w, &h);
    if (!curr) return false;

    if (!bg_buffer || w != img_w || h != img_h) {
        if (bg_buffer) free(bg_buffer);
        bg_buffer = curr;
        img_w = w; img_h = h;
        return false; // First frame is background
    }

    long diff_count = 0;
    long total_bytes = w * h * 3;

    for (long i = 0; i < total_bytes; i+=3) { // Check RGB
        int diff = abs(curr[i] - bg_buffer[i]);
        if (diff > PIXEL_THRESH) diff_count++;
        // Update background (Running Average)
        bg_buffer[i] = (unsigned char)((bg_buffer[i]*0.8) + (curr[i]*0.2));
    }
    
    free(curr);
    return ((float)diff_count / (float)(w*h)) > MOTION_THRESH;
}

void camera_cleanup(void) { if(bg_buffer) free(bg_buffer); }