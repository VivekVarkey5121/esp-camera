#include "avi_recorder.h"
#include "audio_helper.h"
#include "sd_manager.h"

static void inline print_quartet(unsigned long value, File &fd)
{
  uint8_t quartet[4];
  quartet[0] = value % 0x100;
  quartet[1] = (value >> 8) % 0x100;
  quartet[2] = (value >> 16) % 0x100;
  quartet[3] = (value >> 24) % 0x100;
  fd.write(quartet, 4);
}

static esp_err_t start_avi()
{
  Serial.println("[AVI] Starting an avi with Audio...");

  audio_ptr_read = audio_ptr_write;

  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%F_%H_%M_%S", &timeinfo);

  char fname[500];
  char just_filename[100];
  framesize = (settings.video_mode == 1) ? 6 : 5;

  sprintf(just_filename, "%s.avi", strftime_buf);
  sprintf(fname, "%s/%s", VIDEO_PATH, just_filename);
  strcpy(avi_filenames[avi_newest_index], just_filename);

  avifile = SD_MMC.open(fname, FILE_WRITE);
  idxfile = SD_MMC.open(String(VIDEO_PATH) + "/idx.tmp", FILE_WRITE);

  if (!avifile || !idxfile) {
    Serial.println("[ERROR] Failed to open AVI or IDX file");
    return ESP_FAIL;
  }

  for (i = 0; i < AVIOFFSET; i++) {
    buf[i] = pgm_read_byte(&avi_header[i]);
  }
  avifile.write(buf, AVIOFFSET);

  uint8_t *w_ptr = (framesize == 6) ? vga_w : qvga_w;
  uint8_t *h_ptr = (framesize == 6) ? vga_h : qvga_h;
  avifile.seek(0x40); avifile.write(w_ptr, 2);
  avifile.seek(0xA8); avifile.write(w_ptr, 2);
  avifile.seek(0x44); avifile.write(h_ptr, 2);
  avifile.seek(0xAC); avifile.write(h_ptr, 2);

  avifile.seek(AVIOFFSET);

  startms = millis();
  movi_size = 0;
  idx_offset = 4;
  frame_cnt = 0;
  processed_frame = lp_index;
  record_state = 2;

  return ESP_OK;
}

static void another_save_avi()
{
  if (processed_frame != lp_index) {
    processed_frame = (processed_frame + 1) % LIVE_PHOTO_FRAMES;
    frame_cnt++;

    uint32_t jpeg_size = live_photo_buffer_len[processed_frame];
    uint8_t video_padding = (4 - (jpeg_size & 3)) & 3;

    avifile.write(dc_buf, 4);
    print_quartet(jpeg_size, avifile);
    avifile.write(live_photo_buffer[processed_frame], jpeg_size);
    if (video_padding > 0) {
      avifile.write(zero_buf, video_padding);
    }

    idxfile.write(dc_buf, 4);
    print_quartet(0x10, idxfile);
    print_quartet(idx_offset, idxfile);
    print_quartet(jpeg_size, idxfile);

    idx_offset += jpeg_size + video_padding + 8;
    movi_size += jpeg_size + video_padding + 8;

    size_t audio_chunk_size = 2666;
    uint8_t audio_padding = (4 - (audio_chunk_size & 3)) & 3;

    if (audio_ptr_write >= (audio_ptr_read + audio_chunk_size)) {
      avifile.write(wb_buf, 4);
      print_quartet(audio_chunk_size, avifile);

      size_t read_idx = audio_ptr_read % AUDIO_BUF_SIZE;
      size_t bytes_to_end = AUDIO_BUF_SIZE - read_idx;

      if (bytes_to_end >= audio_chunk_size) {
        avifile.write(&audio_ring_buf[read_idx], audio_chunk_size);
      } else {
        avifile.write(&audio_ring_buf[read_idx], bytes_to_end);
        avifile.write(&audio_ring_buf[0], audio_chunk_size - bytes_to_end);
      }

      if (audio_padding > 0) {
        avifile.write(zero_buf, audio_padding);
      }

      idxfile.write(wb_buf, 4);
      print_quartet(0x00, idxfile);
      print_quartet(idx_offset, idxfile);
      print_quartet(audio_chunk_size, idxfile);

      idx_offset += audio_chunk_size + audio_padding + 8;
      movi_size += audio_chunk_size + audio_padding + 8;
      audio_ptr_read += audio_chunk_size;
    }
  }
}

static esp_err_t end_avi()
{
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

  avifile.seek(0x30);
  print_quartet(frame_cnt, avifile);
  avifile.seek(0x8C);
  print_quartet(frame_cnt, avifile);

  avifile.seek(0xE8);
  print_quartet(movi_size + 4, avifile);

  avifile.seek(current_end);
  avifile.write(idx1_buf, 4);
  print_quartet(frame_cnt * 32, avifile);

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

void make_avi()
{
  if (record_state == 1) {
    Serial.println("[VID] start recording");

    if (SD_card_mounted) {
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
        if (record_state == 3) {
          Serial.println("[VID] finished recording");
          end_avi();
          record_state = 0;
        } else {
          Serial.println("[VID] + frame" + String(frame_cnt));
          another_save_avi();
        }
      }
    }
  }
}
