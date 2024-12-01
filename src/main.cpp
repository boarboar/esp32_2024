#include <Arduino.h>
#include "WiFi.h"
#include "esp_http_server.h"

#include "cam.h"
#include "stream.h"

static const char *TAG = "APP";
static const char* ssid = "NETGEAR";
static const char* password = "boarboar";

static const char* index_html = "<!DOCTYPE html><html><head><title>ESP32 CAM</title></head>"\
  "<body>Links:<br>"\
  "<a href=\"stream\">MJPEG stream</a><br>"\
  "<a href=\"capture\">JPEG capture</a>"\
  "</body></html>";

httpd_handle_t stream_server = NULL;

esp_err_t index_httpd_handler(httpd_req_t *req)
{
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}

void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_httpd_handler,
    .user_ctx  = NULL
  }; 
  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = jpg_stream_httpd_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t capture_uri = {
    .uri       = "/capture",
    .method    = HTTP_GET,
    .handler   = jpg_httpd_handler,
    .user_ctx  = NULL
  };
  
  //Serial.printf("Starting streaming server on port: '%d'\n", config.server_port);
  if (httpd_start(&stream_server, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_server, &index_uri);
    httpd_register_uri_handler(stream_server, &stream_uri);
    httpd_register_uri_handler(stream_server, &capture_uri);
    Serial.print("Camera Stream Ready! Go to: http://");
    Serial.print(WiFi.localIP());
    Serial.println(stream_uri.uri);
    Serial.print("Camera Capture Ready! Go to: http://");
    Serial.print(WiFi.localIP());
    Serial.println(capture_uri.uri);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  esp_log_level_set(TAG, ESP_LOG_INFO); 
  init_board();
  esp_err_t err = init_camera();
  Serial.println(err);
  ESP_LOGI(TAG, "SETUP: %u", err);

  // Wi-Fi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start streaming web server
  startCameraServer();
}

void loop() {
  delay(1);
}

