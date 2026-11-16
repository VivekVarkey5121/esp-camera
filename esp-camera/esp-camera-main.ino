#include "config.h"
#include "camera_helper.h"
#include "avi_recorder.h"
#include "audio_helper.h"
#include "sd_manager.h"
#include "display_touch.h"
#include "web_server_helper.h"

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[SYSTEM] Initializing ESP32-S3 Camera System...");

  Serial.println("[MEMORY] Pre-allocating static PSRAM buffers...");
  for (int frame = 0; frame < LIVE_PHOTO_FRAMES; frame++) {
    live_photo_buffer[frame] = (uint8_t *)ps_malloc(MAX_FRAME_SIZE);
    if (live_photo_buffer[frame] == NULL) {
      Serial.printf("[CRITICAL ERROR] Failed to allocate buffer slot %d\n", frame);
      major_fail();
    }
  }
  Serial.println("[MEMORY] Static buffers successfully initialized!");

  SPI.begin(41, 21, 42, -1);
  tft.begin();
  tft.invertDisplay(false);
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  ts.begin(touchSPI);
  SPI.begin(41, 21, 42, -1);
  ts.setRotation(3);
  spr.setColorDepth(16);
  spr.createSprite(320, 240);

  TJpgDec.setCallback(tft_output);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setJpgScale(1);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.xclk_freq_hz = 8000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.frame_size = FRAMESIZE_QSXGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("Camera Init Failed!");
    return;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor != NULL) {
    sensor->set_hmirror(sensor, 1);
    sensor->set_framesize(sensor, FRAMESIZE_QVGA);
    sensor->set_quality(sensor, 15);
    camera_state = 1;
  }

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("Static IP Configuration Failed!");
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("Web Server Address: http://");
  Serial.println(WiFi.localIP());

  pinMode(0, INPUT_PULLUP);
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  pinMode(45, OUTPUT);
  digitalWrite(45, HIGH);

  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if (!SD_MMC.begin("", true, true, SDMMC_FREQ_DEFAULT, 5)) {
    Serial.println("[SD ERROR] SD_MMC Card Mount Failed.");
  }

  if (SD_MMC.cardType() == CARD_NONE) {
    SD_card_mounted = false;
    Serial.println("[SD] SD card not mounted...");
  } else {
    SD_card_mounted = true;
    Serial.println("[SD] SD card was mounted !!");
  }

  if (SD_card_mounted) {
    if (!SD_MMC.exists(PHOTO_PATH)) {
      SD_MMC.mkdir(PHOTO_PATH);
    }
    if (!SD_MMC.exists(VIDEO_PATH)) {
      SD_MMC.mkdir(VIDEO_PATH);
    }
    if (!SD_MMC.exists(INDEX_FILE)) {
      index_sd_card();
    }
    read_and_print_index();
  }

  setupTime();
  setup_i2s();
  mode = 11;

  xTaskCreatePinnedToCore(codeForAviWriterTask, "AviWriterTask", 10000, NULL, 1, &AviWriterTask, 1);
  xTaskCreatePinnedToCore(codeForCameraTask, "CameraTask", 4096, NULL, 1, &CameraTask, 0);

  server.begin();
}

void loop()
{
  if (mode == 11 || mode == 12) {
    handle_viewfinder();
  } else {
    handle_draw_screen();
  }

  handle_button_input();
  handle_save_photo();
  handle_touch_screen();
}

void major_fail()
{
  Serial.println("[CRITICAL] Major failure, halting system.");
  while (true) {
    delay(1000);
  }
}
