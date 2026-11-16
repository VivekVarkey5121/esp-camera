#include "display_touch.h"
#include "camera_helper.h"
#include "sd_manager.h"

void handle_draw_screen()
{
  if (mode == 20) {
    draw_gallery();
  }
  else if (mode == 30) {
    draw_settings_ui();
  }
}

void next_image(int num_files)
{
  gallery_index++;
  if (gallery_index >= num_files) {
    gallery_index = 0;
  }
  Serial.println(gallery_index);
  Serial.println(num_files);
}

void prev_image(int num_files)
{
  gallery_index--;
  if (gallery_index < 0) {
    gallery_index = num_files - 1;
  }
  Serial.println(gallery_index);
  Serial.println(num_files);
}

bool check_button(int x, int y, int w, int h, int16_t in_x, int16_t in_y)
{
  if (in_x - x < w && in_x - x > -w && in_y - y < h && in_y - y > -h) {
    return true;
  }
  return false;
}

bool isDent(int x, int y)
{
  (void)x;
  (void)y;
  return false;
}

SwipeDir checkSwipe(int16_t x, int16_t y, bool touched)
{
  static bool wasTouched = false;
  static int16_t startX = 0, startY = 0;
  static int16_t lastX = 0, lastY = 0;
  static unsigned long startTime = 0;

  if (touched) {
    if (!isDent(x, y)) {
      if (!wasTouched) {
        startX = x;
        startY = y;
        lastX = x;
        lastY = y;
        startTime = millis();
        wasTouched = true;
      } else {
        lastX = x;
        lastY = y;
      }
    } else {
      if (wasTouched) {
        wasTouched = false;
        unsigned long duration = millis() - startTime;
        if (duration <= MAX_SWIPE_TIME) {
          int16_t diffX = lastX - startX;
          int16_t diffY = lastY - startY;
          if (abs(diffX) > abs(diffY)) {
            if (abs(diffX) > MIN_SWIPE_DIST) {
              return (diffX > 0) ? GESTURE_RIGHT : GESTURE_LEFT;
            }
          } else {
            if (abs(diffY) > MIN_SWIPE_DIST) {
              return (diffY > 0) ? GESTURE_DOWN : GESTURE_UP;
            }
          }
        }
      }
      return GESTURE_NONE;
    }
  } else {
    if (wasTouched) {
      wasTouched = false;
      unsigned long duration = millis() - startTime;
      if (duration <= MAX_SWIPE_TIME) {
        int16_t diffX = lastX - startX;
        int16_t diffY = lastY - startY;
        if (abs(diffX) > abs(diffY)) {
          if (abs(diffX) > MIN_SWIPE_DIST) {
            return (diffX > 0) ? GESTURE_RIGHT : GESTURE_LEFT;
          }
        } else {
          if (abs(diffY) > MIN_SWIPE_DIST) {
            return (diffY > 0) ? GESTURE_DOWN : GESTURE_UP;
          }
        }
        return GESTURE_TOUCH;
      }
    }
  }

  return GESTURE_NONE;
}

void change_mode()
{
  if (mode == 11) {
    mode = 12;
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
      if (settings.video_mode == 1) {
        s->set_framesize(s, FRAMESIZE_VGA);
        s->set_quality(s, 20);
        camera_state = 1;
        spr.fillSprite(TFT_BLACK);
        TJpgDec.setJpgScale(4);
      }
      else if (settings.video_mode == 2) {
        s->set_framesize(s, FRAMESIZE_QVGA);
        s->set_quality(s, 15);
        camera_state = 1;
        TJpgDec.setJpgScale(1);
      }
    }
  }
  else if (mode == 12) {
    mode = 11;
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
      s->set_framesize(s, FRAMESIZE_QVGA);
      s->set_quality(s, 15);
      camera_state = 1;
      TJpgDec.setJpgScale(1);
    }
  }
}

void draw_gallery()
{
  spr.fillSprite(TFT_DARKGREY);
  draw_gallery_ui(gallery_index, num_files);
}

void draw_gallery_ui(int index, int num_files)
{
  (void)num_files;
  spr.fillSprite(TFT_DARKGREY);
  static int file_res = 2;

  String line = get_file_at_index(index);
  if (line.length() == 0) {
    return;
  }

  int firstComma = line.indexOf(',');
  String filename = "";
  String filetype = "";

  if (firstComma != -1) {
    filename = line.substring(0, firstComma);
    filetype = line.substring(firstComma + 1);
    filename.trim();
    filetype.trim();
  }

  if (filetype == "PHOTO") {
    String filepath = String(PHOTO_PATH) + "/" + filename + "/15.jpg";
    if (SD_MMC.exists(filepath)) {
      File file = SD_MMC.open(filepath, FILE_READ);
      if (file) {
        size_t image_buffer_len = file.size();
        uint8_t *image_buffer = (uint8_t *)ps_malloc(image_buffer_len);

        if (image_buffer != NULL) {
          file.read(image_buffer, image_buffer_len);
          uint16_t w = 0, h = 0;
          TJpgDec.getJpgSize(&w, &h, image_buffer, image_buffer_len);

          if (w == 640 && h == 480) {
            if (file_res == 1) {
              file_res = 2;
              TJpgDec.setJpgScale(2);
              Serial.println("[SIZE] changed to vga viewing");
            }
          }
          else if (w == 320 && h == 240) {
            if (file_res == 2) {
              file_res = 1;
              TJpgDec.setJpgScale(1);
              Serial.println("[SIZE] changed to qvga viewing");
            }
          }
          else {
            Serial.printf("[INFO] Resolution: Other (%dx%d)\n", w, h);
          }

          TJpgDec.drawJpg(0, 0, image_buffer, image_buffer_len);
          free(image_buffer);
        }
        else {
          spr.setTextColor(TFT_RED);
          spr.drawString("PSRAM Allocation Failed", 10, 110);
        }
        file.close();
      }
    }
    else {
      spr.setTextColor(TFT_RED);
      spr.drawString("File Not Found on SD Card", 10, 110);
    }
  }
  else if (filetype == "VIDEO") {
    spr.setTextColor(TFT_RED);
    spr.drawString("No support for video playback in galler yet :<", 10, 110);
  }
  else {
    Serial.println("[ERROR] Unknown file type in index: " + filetype);
    return;
  }

  spr.drawTriangle(275, 205, 285, 210, 275, 215, TFT_WHITE);
  spr.drawTriangle(45, 205, 35, 210, 45, 215, TFT_WHITE);
  spr.setTextColor(TFT_WHITE);
  spr.setTextSize(1);
  spr.drawString(filename.c_str(), 10, 10, 1);
  spr.pushSprite(0, 0);
}

void draw_settings_ui()
{
  spr.fillSprite(TFT_DARKGREY);
  spr.setTextColor(TFT_WHITE);
  spr.setTextSize(1);
  spr.drawString("Settings", 10, 10, 1);
  spr.drawString("Video Mode:", 10, 50, 1);
  if (settings.video_mode == 1) {
    spr.drawString("1", 200, 50, 1);
  }
  else if (settings.video_mode == 2) {
    spr.drawString("2", 200, 50, 1);
  }
  spr.pushSprite(0, 0);
}

void handle_UI_overlay()
{
  spr.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
  spr.setTextSize(1);

  if (flag_video) {
    spr.fillCircle(15, 15, 6, TFT_RED);
    spr.drawString("REC", 28, 7);
  }
  else {
    spr.fillCircle(15, 15, 6, TFT_WHITE);
  }

  spr.drawRect(270, 7, 40, 15, TFT_WHITE);
  spr.fillRect(272, 9, 25, 11, TFT_GREEN);

  if (mode == 11) {
    spr.drawRect(270, 200, 20, 20, TFT_WHITE);
    spr.drawCircle(280, 210, 5, TFT_WHITE);
  }
  else if (mode == 12) {
    spr.drawRect(270, 200, 20, 20, TFT_WHITE);
    spr.drawTriangle(275, 205, 285, 210, 275, 215, TFT_RED);
  }
}

void handle_viewfinder()
{
  int latest_idx = (lp_index > 0) ? lp_index - 1 : LIVE_PHOTO_FRAMES - 1;
  if (latest_idx == last_drawn_frame_idx) {
    return;
  }

  if (live_photo_buffer[latest_idx] != NULL && live_photo_buffer_len[latest_idx] > 0) {
    last_drawn_frame_idx = latest_idx;
    TJpgDec.drawJpg(0, 0, live_photo_buffer[latest_idx], live_photo_buffer_len[latest_idx]);
    handle_UI_overlay();
    spr.pushSprite(0, 0);
  }
}

void handle_button_input()
{
  int current_button_state = digitalRead(0);

  if (current_button_state == LOW && last_button_state == HIGH) {
    if (mode == 11) {
      Take_Photo();
    }
    else if (mode == 12) {
      if (!flag_video) {
        Start_Video();
      }
      else {
        End_Video();
      }
    }

    delay(150);
  }

  last_button_state = current_button_state;
}

void handle_touch_screen()
{
  SwipeDir gesture = GESTURE_NONE;

  static unsigned long lastTouchPoll = 0;
  if (millis() - lastTouchPoll < 50) {
    return;
  }
  lastTouchPoll = millis();

  digitalWrite(10, HIGH);

  static int16_t x;
  static int16_t y;
  static bool touched;

  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    touched = true;
    x = map(p.x, 320, 3900, 0, 320);
    y = map(p.y, 320, 3900, 0, 240);
    Serial.printf("[TOUCH] X: %d, Y: %d\n", x, y);
  }
  else {
    touched = false;
  }

  gesture = checkSwipe(x, y, touched);

  if (gesture == GESTURE_TOUCH) {
    Serial.println("[gesture touch] pressed");
    if (check_button(290, 205, 15, 15, x, y)) {
      Serial.println("[BUTTON A] pressed");
      if (mode == 11 || mode == 12) {
        change_mode();
      }
      else if (mode == 20) {
        next_image(num_files);
      }
      gesture = GESTURE_NONE;
    }
    else if (check_button(20, 205, 15, 15, x, y)) {
      Serial.println("[BUTTON B] pressed");
      if (mode == 20) {
        prev_image(num_files);
      }
      gesture = GESTURE_NONE;
    }
    else if (check_button(20, 20, 15, 15, x, y)) {
      Serial.println("[BUTTON X] pressed");
      if (mode == 11) {
        num_files = get_number_of_files_in_index();
        gallery_index = num_files - 1;
        mode = 20;
        TJpgDec.setJpgScale(2);
      }
      else if (mode == 20) {
        mode = 11;
        TJpgDec.setJpgScale(1);
      }
      gesture = GESTURE_NONE;
    }
    else if (check_button(200, 50, 15, 15, x, y)) {
      Serial.println("[BUTTON Y] pressed");
      if (mode == 30) {
        settings.video_mode++;
        if (settings.video_mode > 2) {
          settings.video_mode = 1;
        }
        draw_settings_ui();
      }
      gesture = GESTURE_NONE;
    }
  }
  else if (gesture == GESTURE_RIGHT) {
    if (mode == 20) {
      next_image(num_files);
    }
    Serial.println("[gesture] RIGHT");
  }
  else if (gesture == GESTURE_LEFT) {
    if (mode == 20) {
      prev_image(num_files);
    }
    Serial.println("[gesture] LEFT");
  }
  else if (gesture == GESTURE_UP) {
    if (mode == 30) {
      mode = 20;
    }
    else if (mode == 20) {
      mode = 30;
    }
    Serial.println("[gesture] UP");
  }
  else if (gesture == GESTURE_DOWN) {
    Serial.println("[gesture] DOWN");
  }
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
  spr.pushImage(x, y, w, h, bitmap);
  return true;
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
