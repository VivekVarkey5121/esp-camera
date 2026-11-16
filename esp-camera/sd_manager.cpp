#include "sd_manager.h"

String getFormattedTime()
{
  struct tm currentTimeInfo;
  if (!getLocalTime(&currentTimeInfo)) {
    Serial.println("[TIME ERROR] Could not read local time.");
    return "1970-01-01_00-00-00";
  }

  char timeStringBuff[32];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d_%H-%M-%S", &currentTimeInfo);
  return String(timeStringBuff);
}

void setupTime()
{
  long gmtOffset_sec = 0;
  int daylightOffset_sec = 0;
  Serial.println("[TIME] Initializing NTP time synchronization...");
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
}

void save_photo_to_sd_card()
{
  String currentTimestamp = getFormattedTime();

  if (File sd_card_photo = SD_MMC.open((String(PHOTO_PATH) + "/" + currentTimestamp + ".jpg").c_str(), FILE_WRITE)) {
    if (photo_buffer != NULL && photo_buffer_len > 0) {
      sd_card_photo.write(photo_buffer, photo_buffer_len);
    }
    Serial.println("[SD] Saving photo" + currentTimestamp);
    sd_card_photo.close();
  }

  if (!SD_MMC.exists((String(PHOTO_PATH) + "/" + currentTimestamp).c_str())) {
    SD_MMC.mkdir((String(PHOTO_PATH) + "/" + currentTimestamp).c_str());
  }

  add_to_index(currentTimestamp.c_str(), "PHOTO");

  for (int frame = 0; frame < LIVE_PHOTO_FRAMES; frame++) {
    if (live_photo_buffer[frame] != NULL && live_photo_buffer_len[frame] > 0) {
      if (File sd_card_live_photo = SD_MMC.open((String(PHOTO_PATH) + "/" + currentTimestamp + "/" + String(frame) + ".jpg").c_str(), FILE_WRITE)) {
        sd_card_live_photo.write(live_photo_buffer[frame], live_photo_buffer_len[frame]);
        sd_card_live_photo.close();
      }
    }
  }
}

void save_photo()
{
  if (SD_card_mounted) {
    save_photo_to_sd_card();
    Serial.println("[SD] Saving photo to SD Card...");
  }

  Serial.println("[MEMORY] Transferring photo to persistent RAM...");
  pers_lp_index = lp_index;
  for (int frame = 0; frame < LIVE_PHOTO_FRAMES; frame++) {
    if (pers_live_photo_buffer[frame] != NULL) {
      free(pers_live_photo_buffer[frame]);
      pers_live_photo_buffer[frame] = NULL;
    }
    if (live_photo_buffer[frame] != NULL && live_photo_buffer_len[frame] > 0) {
      pers_live_photo_buffer_len[frame] = live_photo_buffer_len[frame];
      pers_live_photo_buffer[frame] = (uint8_t *)ps_malloc(live_photo_buffer_len[frame]);
      if (pers_live_photo_buffer[frame] != NULL) {
        memcpy(pers_live_photo_buffer[frame], live_photo_buffer[frame], live_photo_buffer_len[frame]);
      }
    } else {
      pers_live_photo_buffer_len[frame] = 0;
    }
  }

  if (pers_photo_buffer != NULL) {
    free(pers_photo_buffer);
  }
  pers_photo_buffer = (uint8_t *)ps_malloc(photo_buffer_len);

  if (pers_photo_buffer != NULL) {
    pers_photo_buffer_len = photo_buffer_len;
    memcpy(pers_photo_buffer, photo_buffer, photo_buffer_len);
    Serial.println("[MEMORY] Persistent copy successfully saved.");
  } else {
    Serial.println("[MEMORY ERROR] PSRAM allocation failed for persistent buffer!");
    pers_photo_buffer_len = 0;
  }
}

void handle_save_photo()
{
  if (camera_state == 21) {
    save_photo();
    free(photo_buffer);
    photo_buffer = NULL;
    photo_buffer_len = 0;
    camera_state = 1;
  }
}

void index_sd_card()
{
  Serial.println("[INDEX] inedxing sd card");
  int file_count = 0;

  File indexFile = SD_MMC.open(INDEX_FILE, FILE_WRITE);
  if (!indexFile) {
    Serial.println("[INDEX ERROR] Failed to open index file");
    return;
  }

  File root = SD_MMC.open(PHOTO_PATH);
  if (!root) {
    Serial.println("[INDEX ERROR] Failed to open directory");
    indexFile.close();
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      char record[100];
      sprintf(record, "%-19.19s,%-5.5s\n", file.name(), "PHOTO");
      indexFile.write((uint8_t *)record, 26);
      file_count++;
    }
    file.close();
    file = root.openNextFile();
  }
  root.close();

  root = SD_MMC.open(VIDEO_PATH);
  if (!root) {
    Serial.println("[INDEX ERROR] Failed to open directory");
    indexFile.close();
    return;
  }
  file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
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

  File countfile = SD_MMC.open(INDEX_COUNT, FILE_WRITE);
  if (!countfile) {
    Serial.println("[INDEX ERROR] Failed to open index for appending!");
    return;
  }
  countfile.print(file_count);
  countfile.close();
}

void add_to_index(const char *filename, const char *type)
{
  File file = SD_MMC.open(INDEX_FILE, FILE_APPEND);
  if (!file) {
    Serial.println("[INDEX ERROR] Failed to open index for appending!");
    return;
  }

  char record[100];
  sprintf(record, "%-19.19s,%-5.5s\n", filename, type);
  file.write((uint8_t *)record, 26);
  file.close();

  update_index_len(1);
  Serial.println("[INDEX] Added new file to index.");
}

void update_index_len(int num)
{
  int num_files = get_number_of_files_in_index();
  num_files = num_files + num;
  if (num_files < 0) {
    num_files = 0;
  }

  File file = SD_MMC.open(INDEX_COUNT, FILE_WRITE);
  if (file) {
    file.print(num_files);
    file.close();
  } else {
    Serial.println("[iNDEX ERROR] failed to write to index count");
  }
}

void read_and_print_index()
{
  File file = SD_MMC.open(INDEX_FILE, FILE_READ);
  if (!file) {
    Serial.println("[INDEX ERROR] Index file not found or corrupted.");
    return;
  }

  Serial.println("\n--- START OF SD INDEX ---");

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) {
      continue;
    }

    int firstComma = line.indexOf(',');
    if (firstComma != -1) {
      String filename = line.substring(0, firstComma);
      String filetype = line.substring(firstComma + 1);
      filename.trim();
      filetype.trim();
      Serial.printf("[FILE] Name: %s | Type: %s\n", filename.c_str(), filetype.c_str());
    }
  }

  file.close();
  Serial.println("--- END OF SD INDEX ---\n");
}

int get_number_of_files_in_index()
{
  File file = SD_MMC.open(INDEX_COUNT, FILE_READ);
  if (!file || file.size() == 0) {
    if (file) {
      file.close();
    }
    return 0;
  }

  char count_buf[12];
  size_t len = file.readBytes(count_buf, sizeof(count_buf) - 1);
  count_buf[len] = '\0';
  file.close();
  return atoi(count_buf);
}

String get_file_at_index(int targetIndex)
{
  File file = SD_MMC.open(INDEX_FILE, FILE_READ);
  if (!file) {
    return "";
  }

  unsigned long targetByteOffset = (unsigned long)targetIndex * 26;
  if (targetByteOffset < file.size()) {
    file.seek(targetByteOffset);
    String line = file.readStringUntil('\n');
    file.close();
    line.trim();
    return line;
  }

  file.close();
  return "";
}
