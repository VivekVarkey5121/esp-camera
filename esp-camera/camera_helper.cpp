#include "camera_helper.h"

void add_live_photo_frame_to_buffer(camera_fb_t *fb)
{
  if (fb == NULL || fb->buf == NULL) {
    return;
  }

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

void Camera_task()
{
  if (camera_state == 0) {
    return;
  }
  else if (camera_state == 1) {
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
  else if (camera_state == 10) {
    Serial.println("\n[CAPTURE] Starting capture routine...");
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
      Serial.println("[CAMERA] Switching sensor settings to QSXGA (5MP, Quality 10)...");
      s->set_framesize(s, FRAMESIZE_QSXGA);
      s->set_quality(s, 10);
    }

    Serial.println("[CAMERA] Flushing transition frames...");
    for (int frame = 0; frame < DISCARD_FRAMES; frame++) {
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
    for (int frame = 0; frame < DISCARD_FRAMES; frame++) {
      camera_fb_t *tmp_fb = esp_camera_fb_get();
      if (tmp_fb) {
        esp_camera_fb_return(tmp_fb);
      }
      delay(10);
    }

    camera_state = 21;
    flag_photo = false;
  }
  else if (camera_state == 21) {
    return;
  }
}

void codeForCameraTask(void *parameter)
{
  for (;;) {
    Camera_task();
    delay(1);
  }
}

void Start_Video()
{
  if (!flag_photo && camera_state == 1) {
    Serial.println("[VID] Record flag set to true");
    flag_video = true;
    record_state = 1;
  }
}

void End_Video()
{
  flag_video = false;
  final_frame_index = lp_index;
  record_state = 3;
}

void Take_Photo()
{
  if (!flag_video) {
    flag_photo = true;
    camera_state = 10;
  }
}
