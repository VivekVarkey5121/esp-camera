#pragma once

#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <TJpg_Decoder.h>
#include <FS.h>
#include <SD_MMC.h>
#include <time.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// ===========================
// AVI CONFIG
// ===========================
#define AVIOFFSET 308

extern const uint8_t avi_header[AVIOFFSET];
extern uint8_t wb_buf[4];
extern uint8_t zero_buf[4];
extern uint8_t dc_buf[4];
extern uint8_t avi1_buf[4];
extern uint8_t idx1_buf[4];
extern uint8_t vga_w[2];
extern uint8_t vga_h[2];
extern uint8_t cif_w[2];
extern uint8_t cif_h[2];
extern uint8_t svga_w[2];
extern uint8_t svga_h[2];
extern uint8_t uxga_w[2];
extern uint8_t uxga_h[2];
extern uint8_t qvga_w[2];
extern uint8_t qvga_h[2];

// ===========================
// DISPLAY / TOUCH CONFIG
// ===========================
extern SPIClass touchSPI;
#define TOUCH_CS 45
#define TOUCH_IRQ -1

#define BTN_X 50
#define BTN_Y 240
#define BTN_W 140
#define BTN_H 50

#define MIN_SWIPE_DIST 50
#define MAX_SWIPE_TIME 500

#define DENT_X 0
#define DENT_Y 0

// ===========================
// SYSTEM SETTINGS
// ===========================
struct SystemSettings
{
  int video_mode = 1;
};

extern SystemSettings settings;

enum SwipeDir
{
  GESTURE_NONE,
  GESTURE_LEFT,
  GESTURE_RIGHT,
  GESTURE_UP,
  GESTURE_DOWN,
  GESTURE_TOUCH
};

// ===========================
// GLOBAL OBJECTS / STATE
// ===========================
extern TFT_eSPI tft;
extern XPT2046_Touchscreen ts;
extern TFT_eSprite spr;
extern WiFiServer server;

extern uint32_t startms;
extern int capture_interval;
extern uint16_t frame_cnt;
extern int i;
extern uint8_t buf[];
extern uint16_t remnant;
extern unsigned long fileposition;
extern TaskHandle_t CameraTask;
extern TaskHandle_t AviWriterTask;
extern volatile int processed_frame;
extern volatile bool flag_video;
extern volatile bool flag_photo;
extern volatile int record_state;
extern volatile int camera_state;
extern bool SD_card_mounted;
extern int mode;
extern File avifile;
extern File idxfile;
extern const unsigned int FRAMERATE;
extern const unsigned long FRAME_INTERVAL;
extern unsigned long lastFrameTime;
extern const char *PHOTO_PATH;
extern const char *VIDEO_PATH;
extern const char *INDEX_FILE;
extern const char *INDEX_COUNT;
extern int quality;
extern int xlength;
extern int xspeed;
extern int framesize;
extern unsigned long bigdelta;
extern unsigned long totalp;
extern unsigned long totalw;
extern int overtime_count;
extern unsigned long uVideoLen;
extern int frames_so_far;
extern int skipping;
extern int skipped;
extern unsigned long bw;
extern unsigned long elapsedms;
extern unsigned long movi_size;
extern unsigned long idx_offset;
extern char strftime_buf[];
extern time_t now;
extern struct tm timeinfo;
extern char avi_filenames[5][100];
extern int avi_newest_index;
extern volatile int final_frame_index;
extern int gallery_index;
extern int num_files;

extern const char *ssid;
extern const char *password;
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress primaryDNS;

#define SD_MMC_CMD 38
#define SD_MMC_CLK 39
#define SD_MMC_D0 40

#define CAMERA_MODEL_FREENOVE_ESP32S3
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 15
#define SIOD_GPIO_NUM 4
#define SIOC_GPIO_NUM 5
#define Y9_GPIO_NUM 16
#define Y8_GPIO_NUM 17
#define Y7_GPIO_NUM 18
#define Y6_GPIO_NUM 12
#define Y5_GPIO_NUM 10
#define Y4_GPIO_NUM 8
#define Y3_GPIO_NUM 9
#define Y2_GPIO_NUM 11
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13

#define LIVE_PHOTO_FRAMES 30
#define MAX_FRAME_SIZE 80000
extern volatile int lp_index;
extern uint8_t *live_photo_buffer[LIVE_PHOTO_FRAMES];
extern size_t live_photo_buffer_len[LIVE_PHOTO_FRAMES];
extern uint8_t *photo_buffer;
extern volatile size_t photo_buffer_len;
extern int DISCARD_FRAMES;
extern int pers_lp_index;
extern uint8_t *pers_live_photo_buffer[LIVE_PHOTO_FRAMES];
extern size_t pers_live_photo_buffer_len[LIVE_PHOTO_FRAMES];
extern uint8_t *pers_photo_buffer;
extern size_t pers_photo_buffer_len;
extern int last_button_state;
extern int last_drawn_frame_idx;

// AUDIO CONFIG
#define I2S_WS 14
#define I2S_BCK 2
#define I2S_SD 46
#define SAMPLE_RATE 16000
#define AUDIO_BUF_SIZE (SAMPLE_RATE * 2 * 5)
extern uint8_t *audio_ring_buf;
extern size_t audio_ptr_write;
extern size_t audio_ptr_read;

// Forward declarations shared across modules
void major_fail();
void handle_draw_screen();
void handle_touch_screen();
void handle_button_input();
void handle_viewfinder();
void handle_save_photo();
void handle_web_server();
void Take_Photo();
void Start_Video();
void End_Video();
void change_mode();
void setupTime();
String getFormattedTime();
void setup_i2s();
void capture_audio_to_buffer();
void Camera_task();
void codeForCameraTask(void *parameter);
void codeForAviWriterTask(void *parameter);
void add_live_photo_frame_to_buffer(camera_fb_t *fb);
void save_photo();
void save_photo_to_sd_card();
void index_sd_card();
void add_to_index(const char *filename, const char *type);
void update_index_len(int num);
void read_and_print_index();
int get_number_of_files_in_index();
String get_file_at_index(int targetIndex);
void make_avi();
void draw_gallery();
void draw_gallery_ui(int index, int num_files);
void draw_settings_ui();
void handle_UI_overlay();
void next_image(int num_files);
void prev_image(int num_files);
bool check_button(int x, int y, int w, int h, int16_t in_x, int16_t in_y);
bool isDent(int x, int y);
SwipeDir checkSwipe(int16_t x, int16_t y, bool touched);
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);
void drawTriggerButton(uint16_t color, uint16_t textColor, const char *label);
void serve_dashboard(WiFiClient &client);
void serve_photo_buffer(WiFiClient &client);
void serve_rolling_frame(WiFiClient &client, int id);
void send_404(WiFiClient &client);
