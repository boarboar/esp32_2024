#pragma once
#include "esp_camera.h"

// vars need to be extern
//extern const char *TAG;

esp_err_t init_camera(framesize_t fs = FRAMESIZE_VGA);
esp_err_t init_board(void);
void cam_led(int);
esp_err_t cam_set(const char*);
