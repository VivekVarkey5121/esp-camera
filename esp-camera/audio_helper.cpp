#include "audio_helper.h"
#include "avi_recorder.h"
#include <driver/i2s.h>

void setup_i2s()
{
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
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
  audio_ring_buf = (uint8_t *)ps_malloc(AUDIO_BUF_SIZE);
}

void capture_audio_to_buffer()
{
  if (!flag_video || audio_ring_buf == NULL) {
    return;
  }

  size_t bytes_read = 0;
  uint8_t i2s_raw[2048];
  i2s_read(I2S_NUM_0, i2s_raw, 2048, &bytes_read, portMAX_DELAY);

  if (bytes_read > 0) {
    for (size_t index = 0; index < bytes_read; index++) {
      audio_ring_buf[(audio_ptr_write + index) % AUDIO_BUF_SIZE] = i2s_raw[index];
    }
    audio_ptr_write += bytes_read;
  }
}

void codeForAviWriterTask(void *parameter)
{
  for (;;) {
    capture_audio_to_buffer();
    make_avi();
    delay(1);
  }
}
