/*
ESP32-CAM Face detection (offline)
Author : ChungYi Fu (Kaohsiung, Taiwan)  2020-5-5 01:00
https://www.facebook.com/francefu

Please change the esp32 core of Arduino IDE to version 1.0.4.
The code can't run on version 1.0.5 .
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"             //用於電源不穩不重開機 
#include "soc/rtc_cntl_reg.h"    //用於電源不穩不重開機 
#include "esp_camera.h"          //視訊函式
#include "fd_forward.h"          //人臉偵測函式

//
// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled
//

//安可信ESP32-CAM模組腳位設定
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
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

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //關閉電源不穩就重開機的設定
    
  Serial.begin(115200);
  Serial.setDebugOutput(true);  //開啟診斷輸出
  Serial.println();

  //視訊組態設定
  camera_config_t config;
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  //視訊初始化
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  //可動態改變視訊框架大小(解析度大小)
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);  //UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA

  //閃光燈(GPIO4)
  ledcAttachPin(4, 4);  
  ledcSetup(4, 5000, 8);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);          
}

void loop() {
  dl_matrix3du_t *image_matrix = NULL;
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
      Serial.println("Camera capture failed");
  } else {
      image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);  //分配內部記憶體
      if (!image_matrix) {
          Serial.println("dl_matrix3du_alloc failed");
      } else {
          //臉部偵測參數設定  https://github.com/espressif/esp-face/blob/master/face_detection/README.md
          static mtmn_config_t mtmn_config = {0};
          mtmn_config.type = FAST;
          mtmn_config.min_face = 80;
          mtmn_config.pyramid = 0.707;
          mtmn_config.pyramid_times = 4;
          mtmn_config.p_threshold.score = 0.6;
          mtmn_config.p_threshold.nms = 0.7;
          mtmn_config.p_threshold.candidate_number = 20;
          mtmn_config.r_threshold.score = 0.7;
          mtmn_config.r_threshold.nms = 0.7;
          mtmn_config.r_threshold.candidate_number = 10;
          mtmn_config.o_threshold.score = 0.7;
          mtmn_config.o_threshold.nms = 0.7;
          mtmn_config.o_threshold.candidate_number = 1;
          
          fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item);  //影像格式轉換RGB格式
          box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);  //偵測人臉取得臉框數據
          if (net_boxes){
            Serial.println("faces = " + String(net_boxes->len));  //偵測到的人臉數
            Serial.println();
            for (int i = 0; i < net_boxes->len; i++){  //列舉人臉位置與大小
                Serial.println("index = " + String(i));
                int x = (int)net_boxes->box[i].box_p[0];
                Serial.println("x = " + String(x));
                int y = (int)net_boxes->box[i].box_p[1];
                Serial.println("y = " + String(y));
                int w = (int)net_boxes->box[i].box_p[2] - x + 1;
                Serial.println("width = " + String(w));
                int h = (int)net_boxes->box[i].box_p[3] - y + 1;
                Serial.println("height = " + String(h));
                Serial.println();
            } 
            free(net_boxes->score);
            free(net_boxes->box);
            free(net_boxes->landmark);
            free(net_boxes);
          }
          else {
            Serial.println("No Face");    //未偵測到的人臉
            Serial.println();
          }
          dl_matrix3du_free(image_matrix);
      }
      esp_camera_fb_return(fb);
  }
  
  delay(1000);
}
