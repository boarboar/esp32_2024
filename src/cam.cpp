#include <Arduino.h>
#include "cam.h"
#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include "esp_http_server.h"

#define BOARD_ESP32CAM_AITHINKER 1

// WROVER-KIT PIN Map
#ifdef BOARD_WROVER_KIT

#define CAM_PIN_PWDN -1  //power down is not used
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 21
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 19
#define CAM_PIN_D2 18
#define CAM_PIN_D1 5
#define CAM_PIN_D0 4
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif

// ESP32Cam (AiThinker) PIN Map
#ifdef BOARD_ESP32CAM_AITHINKER

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif
// ESP32S3 (WROOM) PIN Map
#ifdef BOARD_ESP32S3_WROOM
#define CAM_PIN_PWDN 38
#define CAM_PIN_RESET -1   //software reset will be performed
#define CAM_PIN_VSYNC 6
#define CAM_PIN_HREF 7
#define CAM_PIN_PCLK 13
#define CAM_PIN_XCLK 15
#define CAM_PIN_SIOD 4
#define CAM_PIN_SIOC 5
#define CAM_PIN_D0 11
#define CAM_PIN_D1 9
#define CAM_PIN_D2 8
#define CAM_PIN_D3 10
#define CAM_PIN_D4 12
#define CAM_PIN_D5 18
#define CAM_PIN_D6 17
#define CAM_PIN_D7 16
#endif

#define LED_PIN 33
#define FLASH_PIN 4

/*
static framesize_t fszmap[] = {
{
    FRAMESIZE_96X96,    // 96x96
    FRAMESIZE_QQVGA,    // 160x120
    FRAMESIZE_QCIF,     // 176x144
    FRAMESIZE_HQVGA,    // 240x176
    FRAMESIZE_240X240,  // 240x240
    FRAMESIZE_QVGA,     // 320x240
    FRAMESIZE_CIF,      // 400x296
    FRAMESIZE_HVGA,     // 480x320
    FRAMESIZE_VGA,      // 640x480
    FRAMESIZE_SVGA,     // 800x600
    FRAMESIZE_XGA,      // 1024x768
    FRAMESIZE_HD,       // 1280x720
    FRAMESIZE_SXGA,     // 1280x1024
    FRAMESIZE_UXGA,     // 1600x1200
    // 3MP Sensors
    FRAMESIZE_FHD,      // 1920x1080
    FRAMESIZE_P_HD,     //  720x1280
    FRAMESIZE_P_3MP,    //  864x1536
    FRAMESIZE_QXGA,     // 2048x1536
    // 5MP Sensors
    FRAMESIZE_QHD,      // 2560x1440
    FRAMESIZE_WQXGA,    // 2560x1600
    FRAMESIZE_P_FHD,    // 1080x1920
    FRAMESIZE_QSXGA,    // 2560x1920
    FRAMESIZE_INVALID
} framesize_t;
*/

static const char *TAG = "cam";

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental) // 10-15MHz would yield better qual? 
    //.xclk_freq_hz = 20000000,
    .xclk_freq_hz = 10000001,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality // try 6?
    .fb_count = 1,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_LATEST, //CAMERA_GRAB_WHEN_EMPTY,
    
};

void cam_led(int s) {
  digitalWrite(LED_PIN, s ? LOW : HIGH);  
}

void reportFreeRAM() {
  ESP_LOGI(TAG, "FHeep: %u, FPSRAM: %u, PSRAMFL %u", ESP.getFreeHeap(), ESP.getFreePsram(), psramFound());
} 

esp_err_t init_camera(framesize_t fs)
{
    //esp_log_level_set(TAG, ESP_LOG_INFO); 
    //initialize the camera
    
    if(psramFound()){
      camera_config.frame_size = fs;
      camera_config.jpeg_quality = 6;
      camera_config.fb_count = 2;
    } else {
      camera_config.frame_size = fs;
      camera_config.jpeg_quality = 12;
      camera_config.fb_count = 1;
    }
    
    reportFreeRAM();

    ESP_LOGI(TAG, "FS: %u, jq: %u, fb: %u", camera_config.frame_size, camera_config.jpeg_quality, camera_config.fb_count);

    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");

        periph_module_disable(PERIPH_I2C0_MODULE); // try to shut I2C down properly in case that is the problem
        periph_module_disable(PERIPH_I2C1_MODULE);
        periph_module_reset(PERIPH_I2C0_MODULE);
        periph_module_reset(PERIPH_I2C1_MODULE);
        delay(1000);
        ESP.restart();

        return err;
    }

    sensor_t * s = esp_camera_sensor_get();

    if (NULL != s) {
      s->set_brightness(s, 1);//up the blightness just a bit
      s->set_saturation(s, -1);//lower the saturation
      s->set_contrast(s, 1);
      s->set_awb_gain(s, 0);  
      s->set_lenc(s, 0);
    }
    return ESP_OK;
}

esp_err_t init_board(void)
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  //power up the camera if PWDN pin is defined
  if(CAM_PIN_PWDN != -1){
    pinMode(CAM_PIN_PWDN, OUTPUT);
    digitalWrite(CAM_PIN_PWDN, LOW);
  }

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(FLASH_PIN, OUTPUT);

  //digitalWrite(FLASH_PIN, HIGH);
  //delay(100);
  //digitalWrite(FLASH_PIN, LOW);
  digitalWrite(LED_PIN, HIGH);

  return ESP_OK;
}

// ?var1=x&var2=y...
// f = frimesize
// q = qual
// g = 0/1
esp_err_t cam_set(const char* buf) {
  if (buf == NULL) return ESP_FAIL;
  sensor_t * s = esp_camera_sensor_get();
  char value[32] = {0,};
  esp_err_t res = ESP_OK;
  ESP_LOGI(TAG, "Q: %s", buf);
  if(httpd_query_key_value(buf, "f", value, sizeof(value)) == ESP_OK) {
    int val = atoi(value);
    framesize_t fsz = (framesize_t)val;
    if (fsz != camera_config.frame_size) {
      esp_camera_deinit();
      delay(1);
      res = init_camera(fsz);
      delay(10);
      ESP_LOGI(TAG, "FRSET: F=%d RES=%d", fsz, res);
    }
  } 
  if(httpd_query_key_value(buf, "q", value, sizeof(value)) == ESP_OK) {
    int val = atoi(value);
    res = s->set_quality(s, val);
    ESP_LOGI(TAG, "FSET: Q=%d RES=%d", val, res);
  }
  if(httpd_query_key_value(buf, "g", value, sizeof(value)) == ESP_OK) {
    int val = atoi(value);
    if (val) val = 2;
    res = s->set_special_effect(s, val);
    ESP_LOGI(TAG, "FSET: G=%d RES=%d", val, res);
  }

  return res;
}
