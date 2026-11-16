//===================
// config
//===================

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

// ========================================================
// FORWARD DECLARATIONS 
// ========================================================
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);
static void inline print_quartet(unsigned long i, File &fd);
static esp_err_t start_avi();
static void another_save_avi();
static esp_err_t end_avi();
void make_avi();
void codeForCameraTask(void *parameter);
void codeForAviWriterTask(void *parameter);
void save_photo_to_sd_card();
void add_live_photo_frame_to_buffer(camera_fb_t *fb);
void Take_Photo();
void save_photo();
void handle_web_server();
void handle_save_photo();
void handle_viewfinder();
void handle_button_input();
void serve_dashboard(WiFiClient &client);
void serve_photo_buffer(WiFiClient &client);
void serve_rolling_frame(WiFiClient &client, int id);
void send_404(WiFiClient &client);
String getFormattedTime();
void setupTime();
void Camera_task();
void Start_Video();
void End_Video();
void drawTriggerButton(uint16_t color, uint16_t textColor, const char *label);

// ========================================================
// AVI CONFIG
// ========================================================
// Change AVIOFFSET to accommodate the second stream (Audio)
#define AVIOFFSET 308 // Increased from 240

uint8_t wb_buf[4] = {0x30, 0x31, 0x77, 0x62}; // "01wb" (Audio Chunk ID)

// Updated AVI Header with Audio Stream (PCM, 16kHz, 16-bit, Mono)
const uint8_t avi_header[AVIOFFSET] PROGMEM = {
  0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x41, 0x56, 0x49, 0x20, 0x4C, 0x49, 0x53, 0x54,
  0xD0, 0x00, 0x00, 0x00, 0x68, 0x64, 0x72, 0x6C, 0x61, 0x76, 0x69, 0x68, 0x38, 0x00, 0x00, 0x00,
  0xA0, 0x86, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x80, 0x02, 0x00, 0x00, 0xE0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x49, 0x53, 0x54, 0x84, 0x00, 0x00, 0x00,
  0x73, 0x74, 0x72, 0x6C, 0x73, 0x74, 0x72, 0x68, 0x30, 0x00, 0x00, 0x00, 0x76, 0x69, 0x64, 0x73,
  0x4D, 0x4A, 0x50, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x66, 0x28, 0x00, 0x00, 0x00,
  0x28, 0x00, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00, 0xE0, 0x01, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00,
  0x4D, 0x4A, 0x50, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x4C, 0x49, 0x53, 0x54, 0x5C, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x6C,
  0x73, 0x74, 0x72, 0x68, 0x30, 0x00, 0x00, 0x00, 0x61, 0x75, 0x64, 0x73, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x80, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x66, 0x12, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
  0x80, 0x3E, 0x00, 0x00, 0x00, 0x7D, 0x00, 0x00, 0x02, 0x00, 0x10, 0x00, 0x00, 0x00, 0x4C, 0x49,
  0x53, 0x54, 0x00, 0x00, 0x00, 0x00, 0x6D, 0x6F, 0x76, 0x69
};

unsigned long movi_size = 0;
unsigned long jpeg_size = 0;
unsigned long idx_offset = 0;

uint8_t zero_buf[4] = {0x00, 0x00, 0x00, 0x00};
uint8_t dc_buf[4] = {0x30, 0x30, 0x64, 0x63};   // "00dc"
uint8_t avi1_buf[4] = {0x41, 0x56, 0x49, 0x31}; // "AVI1"
uint8_t idx1_buf[4] = {0x69, 0x64, 0x78, 0x31}; // "idx1"

uint8_t vga_w[2] = {0x80, 0x02};  // 640
uint8_t vga_h[2] = {0xE0, 0x01};  // 480
uint8_t cif_w[2] = {0x90, 0x01};  // 400
uint8_t cif_h[2] = {0x28, 0x01};  // 296
uint8_t svga_w[2] = {0x20, 0x03}; // 800
uint8_t svga_h[2] = {0x58, 0x02}; // 600
uint8_t uxga_w[2] = {0x40, 0x06}; // 1600
uint8_t uxga_h[2] = {0xB0, 0x04}; // 1200
uint8_t qvga_w[2] = {0x40, 0x01};  // 320 in hex (Little-Endian)
uint8_t qvga_h[2] = {0xF0, 0x00};  // 240 in hex (Little-Endian)

// ========================================================
// DISPLAY CONFIGURATION
// ========================================================
// isplay in tft_espi usersetup
SPIClass touchSPI(HSPI);


#define TOUCH_CS  45
#define TOUCH_IRQ -1 

//=======================================================
// SETTINGS CONFIG
//========================================================
struct SystemSettings
{
  int video_mode = 1;        // 1 = 640x480 -> mini viewfinder, 2 = 320x240 -> vga viewfinder, 3 = 640x480 -> full viewfinder low fps
};

SystemSettings settings; // Global settings object

// =======================================================
// BUTTON DEFINITIONS (Width: 140px, Height: 50px)
// =======================================================
#define BTN_X 50
#define BTN_Y 240
#define BTN_W 140
#define BTN_H 50

#define MIN_SWIPE_DIST 50 
#define MAX_SWIPE_TIME 500 

#define DENT_X 0
#define DENT_Y 0

//========================================================
// GLOBAL VARIABLES
//========================================================

enum SwipeDir
{
  GESTURE_NONE,
  GESTURE_LEFT,
  GESTURE_RIGHT,
  GESTURE_UP,
  GESTURE_DOWN,
  GESTURE_TOUCH
};

TFT_eSPI tft = TFT_eSPI();      
XPT2046_Touchscreen ts(TOUCH_CS);

TFT_eSprite spr = TFT_eSprite(&tft);

//from avi code - in -
uint32_t startms = 0;      
int capture_interval = 83; 
uint16_t frame_cnt = 0;    

int i = 0;                      
#define BUFFSIZE 512            
uint8_t buf[BUFFSIZE];          
uint16_t remnant = 0;           
unsigned long fileposition = 0; 

TaskHandle_t CameraTask, AviWriterTask;
volatile int processed_frame = 30;

// Flags + state tracking
volatile bool flag_video = false;
volatile bool flag_photo = false; 
volatile int record_state = 0; 
volatile int camera_state = 0;
bool SD_card_mounted = false;

static int gallery_index = 0;
static int num_files = 0;



int mode = 0; // 11 - photo mode, 12 - video mode, 20 - gallery , 30 - settings

File avifile; 
File idxfile; 

const unsigned int FRAMERATE = 12;
const unsigned long FRAME_INTERVAL = 1000 / FRAMERATE;
unsigned long lastFrameTime = 0;

const char *PHOTO_PATH = "/photos";
const char *VIDEO_PATH = "/videos";
const char *INDEX_FILE = "/index.csv";
const char *INDEX_COUNT = "/index_count.txt";

// avi vars
int quality = 10;
int xlength = 0;
int xspeed = 1;
int framesize = 6; 
unsigned long bigdelta = 0;
unsigned long totalp = 0;
unsigned long totalw = 0;
int overtime_count = 0;
unsigned long uVideoLen = 0;
int frames_so_far = 0;
int skipping = 0;
int skipped = 0;
unsigned long bw = 0;
unsigned long elapsedms = 0;
char strftime_buf[64];
time_t now;
struct tm timeinfo;
char avi_filenames[5][100];
int avi_newest_index = 0;

volatile int final_frame_index = 0;

// ===========================
// WiFi credentials  
// ===========================
const char *ssid = "xx";
const char *password = "xx";

IPAddress local_IP(192, 168, 0, 184);  
IPAddress gateway(192, 168, 0, 1);    
IPAddress subnet(255, 255, 255, 0);   
IPAddress primaryDNS(192, 168, 0, 1);  

// Freenove SDMMC hardware pin definitions
#define SD_MMC_CMD 38 
#define SD_MMC_CLK 39 
#define SD_MMC_D0  40 

// ESP32-S3 Board Pin Maps
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

WiFiServer server(80);

// photo configs
#define LIVE_PHOTO_FRAMES 30  
#define MAX_FRAME_SIZE 80000
volatile int lp_index = 0;
uint8_t *live_photo_buffer[LIVE_PHOTO_FRAMES] = {NULL};
size_t live_photo_buffer_len[LIVE_PHOTO_FRAMES] = {0};

uint8_t* volatile photo_buffer = NULL;
volatile size_t photo_buffer_len = 0;

int DISCARD_FRAMES = 4;

int pers_lp_index = 0;
uint8_t *pers_live_photo_buffer[LIVE_PHOTO_FRAMES] = {NULL};
size_t pers_live_photo_buffer_len[LIVE_PHOTO_FRAMES] = {0};

uint8_t *pers_photo_buffer = NULL;
size_t pers_photo_buffer_len = 0;

int last_button_state = HIGH;
int last_drawn_frame_idx = -1; 


//========================================================
// AUDIO CONFIG
//========================================================
#include <driver/i2s.h>

#define I2S_WS 14
#define I2S_BCK 2
#define I2S_SD 46
#define SAMPLE_RATE 16000

// 16kHz * 2 bytes * 5 seconds of safety buffer = 160KB
#define AUDIO_BUF_SIZE (SAMPLE_RATE * 2 * 5) 
uint8_t *audio_ring_buf = NULL;
size_t audio_ptr_write = 0;
size_t audio_ptr_read = 0;

void setup_i2s() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // INMP441 L/R tied to GND
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    
    audio_ring_buf = (uint8_t*)ps_malloc(AUDIO_BUF_SIZE);
}

// ========================================================
// SETUP
// ========================================================
void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[SYSTEM] Initializing ESP32-S3 Camera System...");


  Serial.println("[MEMORY] Pre-allocating static PSRAM buffers...");
  for (int i = 0; i < LIVE_PHOTO_FRAMES; i++) {
    live_photo_buffer[i] = (uint8_t *)ps_malloc(MAX_FRAME_SIZE);
    if (live_photo_buffer[i] == NULL) {
      Serial.printf("[CRITICAL ERROR] Failed to allocate buffer slot %d\n", i);
      major_fail(); // Halt if we run out of memory at boot
    }
  }
  Serial.println("[MEMORY] Static buffers successfully initialized!");

  SPI.begin(41, 21, 42, -1); 

  // Initialize the screen first
  tft.begin(); 
  tft.invertDisplay(false);
  tft.setRotation(3); // 1 = Landscape
  tft.fillScreen(TFT_BLACK);

  
  // Initialize touch after TFT so the shared SPI bus is already active
  // Initialize touch 
  ts.begin(touchSPI);  
  SPI.begin(41, 21, 42, -1);
  
  ts.setRotation(3);
  spr.setColorDepth(16); // Use 16-bit RGB565 color depth
  spr.createSprite(320, 240);

  TJpgDec.setCallback(tft_output);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setJpgScale(1); 

//cam config 
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

  sensor_t *s = esp_camera_sensor_get(); 
  if (s != NULL) {
    s->set_hmirror(s, 1); 
    s->set_framesize(s, FRAMESIZE_QVGA);
    s->set_quality(s, 15);
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
  
  pinMode(10, OUTPUT); // TFT CS
  digitalWrite(10, HIGH);
  pinMode(45, OUTPUT); // TOUCH CS
  digitalWrite(45, HIGH);

  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);

  if (!SD_MMC.begin("", true, true, SDMMC_FREQ_DEFAULT, 5)) {
    Serial.println("[SD ERROR] SD_MMC Card Mount Failed.");
  }

  if (SD_MMC.cardType() == CARD_NONE){
    SD_card_mounted = false;
    Serial.println("[SD] SD card not mounted...");
  } else {
    SD_card_mounted = true;
    Serial.println("[SD] SD card was mounted !!");
  }

  if (SD_card_mounted) {
    if(!SD_MMC.exists(PHOTO_PATH)){
      SD_MMC.mkdir(PHOTO_PATH);
    }
    if(!SD_MMC.exists(VIDEO_PATH)){
      SD_MMC.mkdir(VIDEO_PATH);
    }
    if(!SD_MMC.exists(INDEX_FILE)){
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


// ========================================================
// MAIN LOOP
// ========================================================

void loop()
{
  if (mode == 11 || mode == 12){
    handle_viewfinder();
  }
  else{
    handle_draw_screen();
  }

  handle_button_input(); // add mode check for button meaning
  handle_save_photo(); //TODO can be deactivated when in non camera mode if check that all running processes are finished
  handle_touch_screen();
}

// loop handlers

void handle_draw_screen(){
  if (mode == 20){ //gallery
    draw_gallery();

  } 
  else if (mode == 30){ //settings
    draw_settings_ui();
  }
}

void handle_touch_screen()
{
  SwipeDir gesture = GESTURE_NONE;

  static unsigned long lastTouchPoll = 0;
  if (millis() - lastTouchPoll < 50) return; 
  lastTouchPoll = millis();

  digitalWrite(10, HIGH); 


  // if(checkSwipe() == GESTURE_RIGHT){
  //   change_mode();
  // }

  static int16_t x;
  static int16_t y;
  static bool touched;


  if (ts.touched()){
    TS_Point p = ts.getPoint();
    touched = true;
    x = map(p.x, 320, 3900, 0, 320);
    y = map(p.y, 320, 3900, 0, 240);


    Serial.printf("[TOUCH] X: %d, Y: %d\n", x, y);  
  }
  else{
    touched = false;
  }

  gesture = checkSwipe(x, y, touched);


  if (gesture == GESTURE_TOUCH){
    Serial.println("[gesture touch] pressed");
    if(check_button(290, 205, 15, 15, x, y)){
      Serial.println("[BUTTON A] pressed");
      if (mode == 11 || mode == 12){
        change_mode();
      }
      else if (mode == 20){
        next_image(num_files); // TODO next image
      }
      gesture = GESTURE_NONE;
    }
    else if(check_button(20, 205, 15, 15, x, y)){
      Serial.println("[BUTTON B] pressed");
      if (mode == 20){
        prev_image(num_files); // TODO next image
      }

      gesture = GESTURE_NONE;
    }
    else if (check_button(20, 20, 15, 15, x, y))
    {
      Serial.println("[BUTTON X] pressed");
      if (mode == 11)
      {
        num_files = get_number_of_files_in_index();
        gallery_index = num_files - 1;
        mode = 20;
        TJpgDec.setJpgScale(2);

      }
      else if (mode == 20)
      {
        mode = 11;
        TJpgDec.setJpgScale(1);
      }
      gesture = GESTURE_NONE;
    }


    else if (check_button(200,50,15,15, x, y)){
      Serial.println("[BUTTON Y] pressed");
      if (mode == 30){
        settings.video_mode++;
        if (settings.video_mode > 2){  //TODO only has 2 video settings modes atm
          settings.video_mode = 1;
        }
        draw_settings_ui();
      }
      gesture = GESTURE_NONE;
    }
  }

  else if (gesture == GESTURE_RIGHT){
    if(mode == 20){
      next_image(num_files);
    }
    Serial.println("[gesture] RIGHT");
  }
  else if (gesture == GESTURE_LEFT)
  {
    if (mode == 20)
    {
      prev_image(num_files);
    }
    Serial.println("[gesture] LEFT");
  }
  else if (gesture == GESTURE_UP)
  {
    if (mode == 30)
    {
      mode = 20;
    }
    else if (mode == 20)
    {
      mode = 30;
    }

    Serial.println("[gesture] UP");
  }
  else if (gesture == GESTURE_DOWN)
  {
    Serial.println("[gesture] DOWN");
  }
}

void next_image(int num_files){
  gallery_index ++;
  if (gallery_index >= num_files){
    gallery_index = 0;
  }
  Serial.println(gallery_index);
  Serial.println(num_files);

}
void prev_image(int num_files)
{
  gallery_index--;
  if (gallery_index < 0)
  {
    gallery_index = num_files - 1;
  }
  Serial.println(gallery_index);
  Serial.println(num_files);

}

// void settings_screen(){
//   spr.fillScreen(TFT_DARKGREY);
//   spr.setTextColor(TFT_WHITE);
//   spr.setTextSize(2);
//   spr.drawString("Settings", 10, 10, 1); // Explicitly use Font 2
//   spr.pushSprite(0, 0);
// }

bool check_button(int x, int y, int w, int h, int16_t in_x,int16_t in_y ){
  if (in_x - x < w && in_x - x > - w && in_y - y < h && in_y - y > -h){
    return true;
  }
  else{
    return false;
  }
}

void change_mode(){
  if (mode == 11){
    mode = 12;
    if (settings.video_mode == 1){
      sensor_t *s = esp_camera_sensor_get();
      if (s != NULL)
      {
        s->set_framesize(s, FRAMESIZE_VGA);
        s->set_quality(s, 20);
        camera_state = 1;
        spr.fillSprite(TFT_BLACK);
        TJpgDec.setJpgScale(4);  //mini viewfinder
      }
    }
    else if (settings.video_mode == 2){
      sensor_t *s = esp_camera_sensor_get();
      if (s != NULL)
      {
        s->set_framesize(s, FRAMESIZE_QVGA);
        s->set_quality(s, 15);
        camera_state = 1;
        TJpgDec.setJpgScale(1);  //qvga viewfinder
      }
    }
    // else if (settings.video_mode == 3){
    //   sensor_t *s = esp_camera_sensor_get();
    //   if (s != NULL)
    //   {
    //     s->set_framesize(s, FRAMESIZE_VGA);
    //     s->set_quality(s, 15);
    //     camera_state = 1;
    //   }
    // }
  }
  else if (mode == 12){
    mode = 11;

    sensor_t *s = esp_camera_sensor_get();  //TODO change cam resolution subrouting
    if (s != NULL)
    {
      s->set_framesize(s, FRAMESIZE_QVGA);
      s->set_quality(s, 15);
      camera_state = 1;
      TJpgDec.setJpgScale(1); // vga viewfinder
    }
  }
}

bool isDent(int x, int y){
  // if (x - DENT_X < 2 && x - DENT_X > - 2 && y - DENT_Y < 2 && y - DENT_Y > -2){
  //   return true;
  // }
  // else{
  //   return false;
  // }
  return false;
}

SwipeDir checkSwipe(int16_t x, int16_t y, bool touched)
{
  static bool wasTouched = false;
  static int16_t startX = 0, startY = 0;
  static int16_t lastX = 0, lastY = 0;
  static unsigned long startTime = 0;

  if (touched){
    if (!isDent(x,y)){
      if (!wasTouched){
        startX = x;
        startY = y;
        lastX = x;
        lastY = y;
        startTime = millis();
        wasTouched = true;
      }
      else if(wasTouched)
      {
        lastX = x;
        lastY = y;
      }
    }
    else{
      if (wasTouched){

        wasTouched = false;
        unsigned long duration = millis() - startTime;

        // Reject slow drags
        if (duration <= MAX_SWIPE_TIME)
        {
          int16_t diffX = lastX - startX;
          int16_t diffY = lastY - startY;

          // Determine if swipe was primarily Horizontal or Vertical
          if (abs(diffX) > abs(diffY))
          {
            if (abs(diffX) > MIN_SWIPE_DIST)
            {
              return (diffX > 0) ? GESTURE_RIGHT : GESTURE_LEFT;
            }
          }
          else
          {
            if (abs(diffY) > MIN_SWIPE_DIST)
            {
              return (diffY > 0) ? GESTURE_DOWN : GESTURE_UP;
            }
          }

        }
      }
      return GESTURE_NONE;
    }
  }
  else{
    
      if (wasTouched){

        wasTouched = false;
        unsigned long duration = millis() - startTime;

        // Reject slow drags
        if (duration <= MAX_SWIPE_TIME)
        {
          int16_t diffX = lastX - startX;
          int16_t diffY = lastY - startY;

          // Determine if swipe was primarily Horizontal or Vertical
          if (abs(diffX) > abs(diffY))
          {
            if (abs(diffX) > MIN_SWIPE_DIST)
            {
              return (diffX > 0) ? GESTURE_RIGHT : GESTURE_LEFT;
            }
          }
          else
          {
            if (abs(diffY) > MIN_SWIPE_DIST)
            {
              return (diffY > 0) ? GESTURE_DOWN : GESTURE_UP;
            }
          }
          return GESTURE_TOUCH;

        }      
    }

  }
  return GESTURE_NONE;

}

void handle_save_photo(){
  if (camera_state == 21)
  {
    save_photo();
    free(photo_buffer);
    photo_buffer = NULL;
    photo_buffer_len = 0;
    camera_state = 1;
  }
}

void handle_button_input(){
  int current_button_state = digitalRead(0); 

  if (current_button_state == LOW && last_button_state == HIGH) {
    if (mode == 11){ //photo
      Take_Photo();
    }
    else if (mode == 12){ //video //TODO dont let changemode when video is actively recording
      if (!flag_video){
        Start_Video();
      }
      else{
        End_Video();
      }
    }

    delay(150); 
  }

  last_button_state = current_button_state;
}

void handle_viewfinder(){
  int latest_idx = (lp_index > 0) ? lp_index - 1 : LIVE_PHOTO_FRAMES - 1;
    
  if (latest_idx == last_drawn_frame_idx) {
    return; 
  }

  if (live_photo_buffer[latest_idx] != NULL && live_photo_buffer_len[latest_idx] > 0)
  {
    last_drawn_frame_idx = latest_idx;

    TJpgDec.drawJpg(0, 0, live_photo_buffer[latest_idx], live_photo_buffer_len[latest_idx]);

    handle_UI_overlay();

    spr.pushSprite(0, 0); // Pushes the canvas to X:0, Y:0 on the physical screen [1] 
  }
}



//========================================================
// UI HANDLERS
//========================================================

void draw_gallery()
{
  spr.fillSprite(TFT_DARKGREY);
  draw_gallery_ui(gallery_index, num_files);
}

void draw_gallery_ui(int index, int num_files)
{
  spr.fillSprite(TFT_DARKGREY);
  static int file_res = 2; // 1 - qvga, 2 - vga

  String line = get_file_at_index(index);
  if (line.length() == 0)
    return;

  // Parse the fields by finding the single comma
  int firstComma = line.indexOf(',');

  String filename = "";
  String filetype = "";

  if (firstComma != -1)
  {
    filename = line.substring(0, firstComma);
    filetype = line.substring(firstComma + 1);

    filename.trim();
    filetype.trim();

  }
  // String baseName = filename;  -> rn im looking at directories instead !
  // if (baseName.endsWith(".jpg"))
  // {
  //   baseName = baseName.substring(0, baseName.length() - 4);
  // }

  // Build the path to the 15th frame thumbnail
  if(filetype == "PHOTO") {
    String filepath = String(PHOTO_PATH) + "/" + filename + "/15.jpg";
    if (SD_MMC.exists(filepath))
    {
      File file = SD_MMC.open(filepath, FILE_READ);
      if (file)
      {
        size_t image_buffer_len = file.size();
        uint8_t *image_buffer = (uint8_t *)ps_malloc(image_buffer_len);

        if (image_buffer != NULL)
        {
          file.read(image_buffer, image_buffer_len);

          uint16_t w = 0, h = 0;
          TJpgDec.getJpgSize(&w, &h, image_buffer, image_buffer_len);

          if (w == 640 && h == 480)
          {
            if (file_res == 1)
            {
              file_res = 2;
              TJpgDec.setJpgScale(2);
              Serial.println("[SIZE] changed to vga viewing");
            }
            else
            {

            }
          }
          else if (w == 320 && h == 240)
          {
            if (file_res == 2)
            {
              file_res = 1;
              TJpgDec.setJpgScale(1);
              Serial.println("[SIZE] changed to qvga viewing");
            }
            else
            {

            }
          }
          else
          {
            Serial.printf("[INFO] Resolution: Other (%dx%d)\n", w, h);
          }

          TJpgDec.drawJpg(0, 0, image_buffer, image_buffer_len);
          free(image_buffer);
        }
        else
        {
          spr.setTextColor(TFT_RED);
          spr.drawString("PSRAM Allocation Failed", 10, 110);
        }
        file.close();
      }
    }
    else
    {
      spr.setTextColor(TFT_RED);
      spr.drawString("File Not Found on SD Card", 10, 110);
    }
  } else if (filetype == "VIDEO") {

    spr.setTextColor(TFT_RED);
    spr.drawString("No support for video playback in galler yet :<", 10, 110);
  } else {
    Serial.println("[ERROR] Unknown file type in index: " + filetype);
    return;
  }


  // String filepath = String(PHOTO_PATH) + "/" + filename + "/15.jpg";


  spr.drawTriangle(275, 205, 285, 210, 275, 215, TFT_WHITE);
  spr.drawTriangle(45, 205, 35, 210, 45, 215, TFT_WHITE);
  spr.setTextColor(TFT_WHITE);
  spr.setTextSize(1);
  spr.drawString(filename.c_str(), 10, 10, 1); // Explicitly use Font 2  spr.setCursor(15, 215);
  // spr.printf("Photo %d / %d", index + 1, num_files);
  spr.pushSprite(0, 0);
}

void draw_settings_ui()
{
  spr.fillSprite(TFT_DARKGREY);
  spr.setTextColor(TFT_WHITE);
  spr.setTextSize(1);
  spr.drawString("Settings", 10, 10, 1);
  spr.drawString("Video Mode:", 10, 50, 1);
  if (settings.video_mode == 1)
  {
    spr.drawString("1", 200, 50, 1);
  }
  else if (settings.video_mode == 2)
  {
    spr.drawString("2", 200, 50, 1);
  }
  // else if (settings.video_mode == 3)
  // {
  //   spr.drawString("3", 200, 50, 1);
  // }
  spr.pushSprite(0, 0);
}


void handle_UI_overlay()
{

  spr.setTextColor(TFT_WHITE, TFT_TRANSPARENT); // Text with transparent background
  spr.setTextSize(1);

  // Draw record indicator if recording
  if (flag_video)
  {
    spr.fillCircle(15, 15, 6, TFT_RED); // Red recording dot
    spr.drawString("REC", 28, 7);
  }
  else
  {
    spr.fillCircle(15, 15, 6, TFT_WHITE); // white recording dot
  }

  // Draw a battery frame outline
  spr.drawRect(270, 7, 40, 15, TFT_WHITE);
  spr.fillRect(272, 9, 25, 11, TFT_GREEN); // Mock battery charge level -> TODO implement actual battery level when connecting battery

  if (mode == 11)
  {
    spr.drawRect(270, 200, 20, 20, TFT_WHITE);
    spr.drawCircle(280, 210, 5, TFT_WHITE);
  }
  else if (mode == 12)
  {
    spr.drawRect(270, 200, 20, 20, TFT_WHITE);
    spr.drawTriangle(275, 205, 285, 210, 275, 215, TFT_RED);
  }
}



//========================================================
// SD CARD OPERATIONS
// ####################

void save_photo_to_sd_card()
{
  String currentTimestamp = getFormattedTime();

  if(File sd_card_photo = SD_MMC.open((String(PHOTO_PATH) + "/" + currentTimestamp + ".jpg").c_str(), FILE_WRITE)) {
    sd_card_photo.write(photo_buffer, photo_buffer_len);
    Serial.println("[SD] Saving photo" + currentTimestamp);
    sd_card_photo.close();
  }

  if(!SD_MMC.exists((String(PHOTO_PATH) + "/" + currentTimestamp).c_str())){
    SD_MMC.mkdir((String(PHOTO_PATH) + "/" + currentTimestamp).c_str());
  }

  add_to_index((currentTimestamp).c_str(), "PHOTO");

  for (int i = 0; i < LIVE_PHOTO_FRAMES; i++) {
    if (live_photo_buffer[i] != NULL && live_photo_buffer_len[i] > 0) {
      // Fixed string concatenation here
      if (File sd_card_live_photo = SD_MMC.open((String(PHOTO_PATH) + "/" + currentTimestamp + "/" + String(i) + ".jpg").c_str(), FILE_WRITE)) {
        sd_card_live_photo.write(live_photo_buffer[i], live_photo_buffer_len[i]);
        sd_card_live_photo.close();
      }
    }
  }
}

void save_photo()
{
  if (SD_card_mounted)
  {
    save_photo_to_sd_card();
    Serial.println("[SD] Saving photo to SD Card...");
  }

  Serial.println("[MEMORY] Transferring photo to persistent RAM...");
  pers_lp_index = lp_index;
  for (int i = 0; i < LIVE_PHOTO_FRAMES; i++)
  {
    if (pers_live_photo_buffer[i] != NULL)
    {
      free(pers_live_photo_buffer[i]);
      pers_live_photo_buffer[i] = NULL;
    }
    if (live_photo_buffer[i] != NULL && live_photo_buffer_len[i] > 0)
    {
      pers_live_photo_buffer_len[i] = live_photo_buffer_len[i];
      pers_live_photo_buffer[i] = (uint8_t *)ps_malloc(live_photo_buffer_len[i]);
      if (pers_live_photo_buffer[i] != NULL)
      {
        memcpy(pers_live_photo_buffer[i], live_photo_buffer[i], live_photo_buffer_len[i]);
      }
    }
    else
    {
      pers_live_photo_buffer_len[i] = 0;
    }
  }

  if (pers_photo_buffer != NULL)
  {
    free(pers_photo_buffer);
  }
  pers_photo_buffer = (uint8_t *)ps_malloc(photo_buffer_len);

  if (pers_photo_buffer != NULL)
  {
    pers_photo_buffer_len = photo_buffer_len;
    memcpy(pers_photo_buffer, photo_buffer, photo_buffer_len);
    Serial.println("[MEMORY] Persistent copy successfully saved.");
  }
  else
  {
    Serial.println("[MEMORY ERROR] PSRAM allocation failed for persistent buffer!");
    pers_photo_buffer_len = 0;
  }
}


//========================================================
// AVI TASKS - Core 1
//========================================================


void make_avi() {
  if (record_state == 1) {                                     
    Serial.println("[VID] start recording");

    if (SD_card_mounted){
      start_avi();
    } else {
      Serial.println("[VID] No sd card detected. aborting video recording");
      record_state = 0;
    }
  } else {
    if (record_state == 3) {                                  
      end_avi();
      Serial.println("[VID] finished recording");
    } else {
      if (record_state == 2) {                            
        if (record_state == 3)  {                             
          Serial.println("[VID] finished recording");
          end_avi();
          record_state = 0;
        } else  {
          Serial.println("[VID] + frame" + String(frame_cnt));
          another_save_avi();
        }
      }
    }
  }
}


static esp_err_t start_avi() {
  Serial.println("[AVI] Starting an avi with Audio...");
  
  // --- SYNC AUDIO POINTERS ---
  // TODO finish audio implementation 
  audio_ptr_read = audio_ptr_write; 

  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%F_%H_%M_%S", &timeinfo);

  char fname[500];
  char just_filename[100];
  framesize = (settings.video_mode == 1) ? 6 : 5; // 6=VGA, 5=QVGA

  sprintf(just_filename, "%s.avi", strftime_buf);
  sprintf(fname, "%s/%s", VIDEO_PATH, just_filename);
  strcpy(avi_filenames[avi_newest_index], just_filename);

  avifile = SD_MMC.open(fname, FILE_WRITE);
  idxfile = SD_MMC.open(String(VIDEO_PATH) + "/idx.tmp", FILE_WRITE);

  if (!avifile || !idxfile) {
    Serial.println("[ERROR] Failed to open AVI or IDX file");
    return ESP_FAIL;
  }

  // Write Master Header
  for (i = 0; i < AVIOFFSET; i++) {
    buf[i] = pgm_read_byte(&avi_header[i]);
  }
  avifile.write(buf, AVIOFFSET);

  // Set Resolutions in Header
  uint8_t* w_ptr = (framesize == 6) ? vga_w : qvga_w;
  uint8_t* h_ptr = (framesize == 6) ? vga_h : qvga_h;
  avifile.seek(0x40); avifile.write(w_ptr, 2);
  avifile.seek(0xA8); avifile.write(w_ptr, 2);
  avifile.seek(0x44); avifile.write(h_ptr, 2);
  avifile.seek(0xAC); avifile.write(h_ptr, 2);

  avifile.seek(AVIOFFSET);

  // Initialize Counters
  startms = millis();
  movi_size = 0;
  idx_offset = 4; // Start offset after 'movi' tag
  frame_cnt = 0;
  processed_frame = lp_index;
  record_state = 2;

  return ESP_OK;
}

static void another_save_avi() {  
  if (processed_frame != lp_index) {
    processed_frame = (processed_frame + 1) % LIVE_PHOTO_FRAMES;
    frame_cnt++;

    // video chunk
    uint32_t jpeg_size = live_photo_buffer_len[processed_frame];
    uint8_t video_padding = (4 - (jpeg_size & 3)) & 3;

    avifile.write(dc_buf, 4);           // "00dc"
    print_quartet(jpeg_size, avifile);  // size
    avifile.write(live_photo_buffer[processed_frame], jpeg_size);
    if (video_padding > 0) avifile.write(zero_buf, video_padding);

    // add to index
    idxfile.write(dc_buf, 4);
    print_quartet(0x10, idxfile); 
    print_quartet(idx_offset, idxfile);
    print_quartet(jpeg_size, idxfile);

    idx_offset += jpeg_size + video_padding + 8;
    movi_size  += jpeg_size + video_padding + 8;

    // audio chunk
    size_t audio_chunk_size = 2666; 
    uint8_t audio_padding = (4 - (audio_chunk_size & 3)) & 3;

    // Check if ring buffer has enough data
    if (audio_ptr_write >= (audio_ptr_read + audio_chunk_size)) {
      avifile.write(wb_buf, 4);                // "01wb"
      print_quartet(audio_chunk_size, avifile); // size
      
// --- FAST RING BUFFER WRITE ---
      size_t read_idx = audio_ptr_read % AUDIO_BUF_SIZE;
      size_t bytes_to_end = AUDIO_BUF_SIZE - read_idx;

      if (bytes_to_end >= audio_chunk_size) {
          // Chunk fits perfectly before the buffer wraps around
          avifile.write(&audio_ring_buf[read_idx], audio_chunk_size);
      } else {
          // Chunk wraps around: write the end, then write the beginning
          avifile.write(&audio_ring_buf[read_idx], bytes_to_end);
          avifile.write(&audio_ring_buf[0], audio_chunk_size - bytes_to_end);
      }
      
      if (audio_padding > 0) avifile.write(zero_buf, audio_padding);

      // Add Audio to Index: [ID, Flags (0x00), Offset, Size]
      idxfile.write(wb_buf, 4);
      print_quartet(0x00, idxfile);
      print_quartet(idx_offset, idxfile);
      print_quartet(audio_chunk_size, idxfile);

      idx_offset += audio_chunk_size + audio_padding + 8;
      movi_size  += audio_chunk_size + audio_padding + 8;
      audio_ptr_read += audio_chunk_size;
    }
  }
}



static esp_err_t end_avi() {
  Serial.println("[AVI] Ending recording. Finalizing index...");
  
  while (final_frame_index != processed_frame) {
    another_save_avi();
    delay(1);
  }

  uint32_t current_end = avifile.position();
  elapsedms = millis() - startms;
  
  float fRealFPS = (1000.0f * (float)frame_cnt) / ((float)elapsedms);
  uint32_t us_per_frame = round(1000000.0f / fRealFPS);


  avifile.seek(4);
  print_quartet(current_end + (frame_cnt * 32) - 8, avifile); 

  avifile.seek(0x20);
  print_quartet(us_per_frame, avifile);

  avifile.seek(0x30); print_quartet(frame_cnt, avifile);
  avifile.seek(0x8C); print_quartet(frame_cnt, avifile);

  avifile.seek(0xE8);
  print_quartet(movi_size + 4, avifile);

  avifile.seek(current_end);
  avifile.write(idx1_buf, 4); // "idx1"
  print_quartet(frame_cnt * 32, avifile); // 16 bytes for video + 16 for audio per frame

  idxfile.close();
  idxfile = SD_MMC.open(String(VIDEO_PATH) + "/idx.tmp", FILE_READ);

  uint8_t index_transfer_buf[16];
  while (idxfile.available()) {
    idxfile.read(index_transfer_buf, 16);
    avifile.write(index_transfer_buf, 16);
  }

  idxfile.close();
  SD_MMC.remove(String(VIDEO_PATH) + "/idx.tmp");
  avifile.close();

  Serial.printf("[AVI] Saved: %s | FPS: %.2f\n", avi_filenames[avi_newest_index], fRealFPS);
  add_to_index(avi_filenames[avi_newest_index], "VIDEO");
  
  record_state = 0;
  return ESP_OK;
}



void capture_audio_to_buffer() {
    if (!flag_video) return;
    
    size_t bytes_read = 0;
    uint8_t i2s_raw[2048];
    i2s_read(I2S_NUM_0, i2s_raw, 2048, &bytes_read, portMAX_DELAY);
    
    if (bytes_read > 0) {
        for(size_t i=0; i<bytes_read; i++) {
            audio_ring_buf[(audio_ptr_write + i) % AUDIO_BUF_SIZE] = i2s_raw[i];
        }
        audio_ptr_write += bytes_read;
    }
}
//================================
// CAMERA TASK - Core 0
//================================

void Camera_task() {  
  if (camera_state == 0){
    // do nothing
  }
  else if (camera_state == 1){
    unsigned long currentMillis = millis(); 
    if (currentMillis - lastFrameTime >= FRAME_INTERVAL) {
      lastFrameTime = currentMillis; 
      camera_fb_t *fb = esp_camera_fb_get();
      if (fb) {
        add_live_photo_frame_to_buffer(fb);
        lp_index += 1;
        if (lp_index >= LIVE_PHOTO_FRAMES) {
          lp_index = 0;
        }
        esp_camera_fb_return(fb);
      }
    }
  }
  else if (camera_state == 10){
    Serial.println("\n[CAPTURE] Starting capture routine...");
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
      Serial.println("[CAMERA] Switching sensor settings to QSXGA (5MP, Quality 10)...");
      s->set_framesize(s, FRAMESIZE_QSXGA);
      s->set_quality(s, 10);
    }

    Serial.println("[CAMERA] Flushing transition frames...");
    for (int i = 0; i < DISCARD_FRAMES; i++) {
      camera_fb_t *fb = esp_camera_fb_get();
      if (fb) {
        esp_camera_fb_return(fb);
      }
      delay(10);
    }

    camera_state = 20;

    Serial.println("[CAMERA] Initiating high-resolution 5MP frame grab...");
    camera_fb_t *fb = esp_camera_fb_get();
    while (!fb) {
      Serial.println("[CAMERA WARNING] Frame grab returned NULL, retrying...");
      fb = esp_camera_fb_get();
      delay(10);
    }
    Serial.printf("[CAMERA] Frame successfully grabbed. Data Size: %u bytes\n", fb->len);

    if (photo_buffer != NULL) {
      free(photo_buffer);
    }
    photo_buffer = (uint8_t *)ps_malloc(fb->len);
    if (photo_buffer != NULL) {
      memcpy(photo_buffer, fb->buf, fb->len);
      photo_buffer_len = fb->len;
      Serial.println("[MEMORY] Copy complete.");
    } else {
      Serial.println("[MEMORY ERROR] PSRAM allocation failed for temporary buffer!");
    }
    esp_camera_fb_return(fb);


    if (s != NULL) {
      Serial.println("[CAMERA] Reverting sensor settings and activating standby...");
      s->set_framesize(s, FRAMESIZE_QVGA);
      s->set_quality(s, 15);
      delay(100);
    }

    Serial.println("[CAMERA] Flushing transition frames back to vga...");
    for (int i = 0; i < DISCARD_FRAMES; i++) {
      camera_fb_t *tmp_fb = esp_camera_fb_get();
      if (tmp_fb) {
        esp_camera_fb_return(tmp_fb);
      }
      delay(10);
    }

    camera_state = 21;


    flag_photo = false;
  }
  else if (camera_state == 21)
  {
    //photo save in progress, blank
  }
}

// void add_live_photo_frame_to_buffer(camera_fb_t *fb)
// {
//   if (live_photo_buffer[lp_index] != NULL)
//   {
//     free(live_photo_buffer[lp_index]);
//   }
//   live_photo_buffer_len[lp_index] = fb->len;
//   if (live_photo_buffer[lp_index] != NULL)
//   {
//     memcpy(live_photo_buffer[lp_index], fb->buf, fb->len);
//   }
// }

void add_live_photo_frame_to_buffer(camera_fb_t *fb)
{
  if (fb == NULL || fb->buf == NULL) {
    return; 
  }

  // if (frame_locked[lp_index]) {  -> TODO frame locks not implemented, since it should never be in a position to overrun like that/ no recovery for that atm anyways
  //   return; 
  // }

  if (live_photo_buffer[lp_index] == NULL) {
    return; 
  }

  if (fb->len > MAX_FRAME_SIZE) {
    Serial.println("[WARNING] Captured frame exceeded pre-allocated size limit!");
    return; 
  }

  live_photo_buffer_len[lp_index] = fb->len;
  memcpy(live_photo_buffer[lp_index], fb->buf, fb->len);
}




//================================
// CAMERA CONTROL FUNCTIONS
//================================


void Start_Video() 
{
  if(flag_photo){  //only start video if not photo

  }
  else{
    if (camera_state == 1){ // viewfinder is running
      Serial.println("[VID] Record flag set to true");
      flag_video = true;
      record_state = 1; 
    }
  }
}

void End_Video(){
  flag_video = false;
  final_frame_index = lp_index;
  record_state = 3;  

}

void Take_Photo(){
  if(flag_video){  //only allow photo if not video

  }
  else{
  flag_photo = true;
  camera_state = 10;
  }
}


/// ############################
// HELPER FUNCTIONS
// ###########################

void index_sd_card(){  

  Serial.println("[INDEX] inedxing sd card");
  int file_count = 0;

  File indexFile = SD_MMC.open(INDEX_FILE, FILE_WRITE);
  if (!indexFile)
  {
    Serial.println("[INDEX ERROR] Failed to open index file");
    return;
  }

  File root = SD_MMC.open(PHOTO_PATH);
  if (!root)
  {
    Serial.println("[INDEX ERROR] Failed to open directory");
    indexFile.close();
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      char record[100];
      sprintf(record, "%-19.19s,%-5.5s\n", file.name(), "PHOTO");
      indexFile.write((uint8_t *)record, 26);

      file_count++;
    }
    else
    {

    }
    file.close();       
    file = root.openNextFile(); 
  }
  root.close();

  root = SD_MMC.open(VIDEO_PATH);
  if (!root)
  {
    Serial.println("[INDEX ERROR] Failed to open directory");
    indexFile.close();

    return;
  }
  file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
    }
    else
    {
      char record[100];
      sprintf(record, "%-19.19s,%-5.5s\n", file.name(), "VIDEO");
      indexFile.write((uint8_t *)record, 26);

      file_count++;
    }
    file.close();
    file = root.openNextFile();
  }

  root.close(); 
  indexFile.close();



  //
  File countfile = SD_MMC.open(INDEX_COUNT, FILE_WRITE);
  if (!countfile)
  {
    Serial.println("[INDEX ERROR] Failed to open index for appending!");
    return;
  }
  else{
    countfile.print(file_count);
    countfile.close();
  }
}

void add_to_index(const char *filename, const char *type)
{
  File file = SD_MMC.open(INDEX_FILE, FILE_APPEND);
  if (!file)
  {
    Serial.println("[INDEX ERROR] Failed to open index for appending!");
    return;
  }

  // Format: filename,type
  // Example: IMG_0001.jpg,PHOTO,2026-06-18_12-30-00


  char record[100];

  sprintf(record, "%-19.19s,%-5.5s\n", filename, type);
  file.write((uint8_t *)record, 26); // Always writes exactly 26 bytes
  file.close();

  update_index_len(1);

  Serial.println("[INDEX] Added new file to index.");
}

void update_index_len(int num){
  int num_files = get_number_of_files_in_index();
  num_files = num_files + num;
  if (num_files < 0) num_files = 0;

  File file = SD_MMC.open(INDEX_COUNT, FILE_WRITE);
  if (file)
  {
    file.print(num_files);
    file.close();
  }
  else{
    Serial.println("[iNDEX ERROR] failed to write to index count");
  }


}
void read_and_print_index()
{
  File file = SD_MMC.open(INDEX_FILE, FILE_READ);
  if (!file)
  {
    Serial.println("[INDEX ERROR] Index file not found or corrupted.");
    return;
  }

  Serial.println("\n--- START OF SD INDEX ---");

  while (file.available())
  {
    String line = file.readStringUntil('\n'); // Read line-by-line
    line.trim();                              // Clean up hidden carriage returns (\r)

    if (line.length() == 0)
      continue; // Skip empty lines

    // Parse the fields by finding the single comma
    int firstComma = line.indexOf(',');

    if (firstComma != -1)
    {
      String filename = line.substring(0, firstComma);
      String filetype = line.substring(firstComma + 1);

      filename.trim();
      filetype.trim();

      // Print parsed metadata to Monitor
      Serial.printf("[FILE] Name: %s | Type: %s\n", filename.c_str(), filetype.c_str());
    }
  }

  file.close();
  Serial.println("--- END OF SD INDEX ---\n");
}

int get_number_of_files_in_index(){
  File file = SD_MMC.open(INDEX_COUNT, FILE_READ);
  
  if (!file || file.size() == 0)
  {
    if (file) file.close();
    return 0;
  }

  char buf[12]; 
  size_t len = file.readBytes(buf, sizeof(buf) - 1);
  buf[len] = '\0'; // null-terminate 
  
  file.close();
  return atoi(buf);
}

String get_file_at_index(int targetIndex)
{
  File file = SD_MMC.open(INDEX_FILE, FILE_READ);
  if (!file)
    return "";

  unsigned long targetByteOffset = (unsigned long)targetIndex * 26;

  if (targetByteOffset < file.size())
  {
    file.seek(targetByteOffset); // hardware jump
    String line = file.readStringUntil('\n');
    file.close();
    line.trim();
    return line;
  }

  file.close();
  return ""; // Out of bounds
}

void changeMode(int new_mode){
  if (mode == 11 || mode == 12){
    if (new_mode != 11 || new_mode != 12){  //changing from a camera mode to a non camera mode, stop viewfinder
      camera_state = 0;
    }
  }
  else{
    if (mode == 11 || mode == 12) // changing to a camera mode, start viewfinder
    {
      camera_state = 1;  ///TODO sleep / wakeup camera
    }
  }
}


void drawTriggerButton(uint16_t color, uint16_t textColor, const char *label)
{
  tft.fillRect(BTN_X, BTN_Y, BTN_W, BTN_H, color);
  tft.drawRect(BTN_X, BTN_Y, BTN_W, BTN_H, TFT_WHITE);

  tft.setTextColor(textColor);
  tft.setTextSize(1);
  tft.setCursor(BTN_X + 28, BTN_Y + 18);
  tft.print(label);
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
// TODO DMA next
  spr.pushImage(x, y, w, h, bitmap);
  return true;
}

static void inline print_quartet(unsigned long i, File &fd)
{
  uint8_t y[4];
  y[0] = i % 0x100;
  y[1] = (i >> 8) % 0x100;
  y[2] = (i >> 16) % 0x100;
  y[3] = (i >> 24) % 0x100;
  fd.write(y, 4);
}

void codeForCameraTask(void *parameter)
{
  for (;;)
  {
    Camera_task();
    delay(1);
  }
}

void codeForAviWriterTask(void *parameter)
{
  for (;;)
  {
    capture_audio_to_buffer();
    make_avi();
    delay(1);
  }
}

String getFormattedTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("[TIME ERROR] Could not read local time.");
    return "1970-01-01_00-00-00";
  }
  char timeStringBuff[32];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d_%H-%M-%S", &timeinfo);
  return String(timeStringBuff);
}

void setupTime()
{
  long gmtOffset_sec = 0;
  int daylightOffset_sec = 0;
  Serial.println("[TIME] Initializing NTP time synchronization...");
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
}

void major_fail()
{
  Serial.println("[CRITICAL] Major failure, halting system.");
  while (true)
    delay(1000);
}


///######################################
//WEB SERVER TODO - optional file upload/download endpoint later (note: currently uses outdate logic for live photo upload)
///#######################################

void handle_web_server() {
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("\n[WEB SERVER] Incoming connection detected.");
  String request = client.readStringUntil('\r');
  client.flush();

  if (request.indexOf("GET / ") >= 0) {
    serve_dashboard(client);
  } else if (request.indexOf("GET /photo") >= 0) {
    serve_photo_buffer(client);
  } else if (request.indexOf("GET /trigger") >= 0) {
    Take_Photo();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Live Photo Captured and Saved to RAM!");
  } else if (request.indexOf("GET /buffer") >= 0) {
    int idIndex = request.indexOf("id=");
    if (idIndex != -1) {
      // Changed parsing logic to read dynamically regardless of string length
      int id = request.substring(idIndex + 3).toInt(); 
      serve_rolling_frame(client, id);
    } else {
      send_404(client);
    }
  } else {
    send_404(client);
  }
  client.stop(); 
}

void serve_photo_buffer(WiFiClient &client) {
  if (pers_photo_buffer == NULL) {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    return;
  }
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: image/jpeg");
  client.printf("Content-Length: %d\n", pers_photo_buffer_len);
  client.println("Connection: close");
  client.println();
  client.write(pers_photo_buffer, pers_photo_buffer_len);
}

void serve_rolling_frame(WiFiClient &client, int id) {
  if (id < 0 || id >= LIVE_PHOTO_FRAMES || pers_live_photo_buffer[id] == NULL) {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Connection: close");
    client.println();
    return;
  }
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: image/jpeg");
  client.printf("Content-Length: %d\n", pers_live_photo_buffer_len[id]);
  client.println("Connection: close");
  client.println();
  client.write(pers_live_photo_buffer[id], pers_live_photo_buffer_len[id]);
}

void serve_dashboard(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  client.println(R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head><title>ESP32-S3 Camera</title></head>
    <body>
    <h1>ESP32-S3 Camera</h1>
    <button onclick="fetch('/trigger').then(r=>location.reload())">Take Photo</button><br><br>
  )rawliteral");

  if (pers_photo_buffer != NULL) {
    client.println("<img id=\"latestPhotoView\" src=\"\" style=\"max-width:100%; border:2px solid #333;\">");
  }

  client.println(R"rawliteral(
    <br><br><h2>Live Buffer Playback</h2>
    <img id="videoPlayer" src="/buffer?id=0" width="640" height="480" style="border:2px solid #000;">
    <script>
      const cacheBuster = Date.now();
      const highResView = document.getElementById('latestPhotoView');
      if (highResView) { highResView.src = "/photo?t=" + cacheBuster; }

      let currentFrame = 0;
      const totalFrames = 120;
      const frames = [];
      for (let i = 0; i < totalFrames; i++) {
        frames[i] = new Image();
        frames[i].src = "/buffer?id=" + i + "&t=" + cacheBuster;
      }
      setInterval(() => {
        if (frames[currentFrame] && frames[currentFrame].complete) {
          document.getElementById('videoPlayer').src = frames[currentFrame].src;
        }
        currentFrame = (currentFrame + 1) % totalFrames;
      }, 1000 / 12);
    </script>
    </body></html>
  )rawliteral");
}

void send_404(WiFiClient &client) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Connection: close");
  client.println();
}

