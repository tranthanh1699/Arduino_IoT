/*
ESP32-CAM 2.4 inch TFT LCD Display Module (ILI9341, SPI, 240x320)
Author : ChungYi Fu (Kaohsiung, Taiwan)  2020-12-13 23:00
https://www.facebook.com/francefu

Circuit
TFT_MOSI --> IO13
TFT_MISO --> IO12
TFT_SCLK --> IO14
TFT_CS   --> IO15
TFT_DC   --> IO2
TFT_RST  --> IO16 (Don't use IO4)
TFT_VCC  --> 3.3V
TFT_LED  --> 3.3V
TFT_GND  --> GND

Introduction
http://www.lcdwiki.com/2.8inch_SPI_Module_ILI9341_SKU:MSP2807

Refer to
http://fabacademy.org/2020/labs/seoulinnovation/students/seokmin-park/week9.html

Libraries：
https://www.arduinolibraries.info/libraries/adafruit-gfx-library
https://www.arduinolibraries.info/libraries/adafruit-ili9341
*/

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include "SPI.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

//CAMERA_MODEL_AI_THINKER
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

#define TFT_WHITE   ILI9341_WHITE
#define TFT_BLACK   ILI9341_BLACK
#define TFT_RED     ILI9341_RED
#define TFT_ORANGE  ILI9341_ORANGE
#define TFT_YELLOW  ILI9341_YELLOW
#define TFT_GREEN   ILI9341_GREEN
#define TFT_CYAN    ILI9341_CYAN
#define TFT_BLUE    ILI9341_BLUE
#define TFT_MAGENTA ILI9341_MAGENTA

#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  16

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST, TFT_MISO);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Initialise the TFT
  tft.begin();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3);

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
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1; 

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

void loop() {
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  uint8_t buffer;
  for( int i = 0; i < (320*240); i++) {
    buffer = fb->buf[i*2];
    fb->buf[i*2] = fb->buf[i*2+1];
    fb->buf[i*2+1] = buffer;
  }
 
  tft.drawRGBBitmap(0,0,(uint16_t*)fb->buf, 320,240);
  esp_camera_fb_return(fb);    
}
