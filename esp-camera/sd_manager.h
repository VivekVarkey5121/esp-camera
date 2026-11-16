#pragma once

#include "config.h"

void save_photo_to_sd_card();
void save_photo();
void index_sd_card();
void add_to_index(const char *filename, const char *type);
void update_index_len(int num);
void read_and_print_index();
int get_number_of_files_in_index();
String get_file_at_index(int targetIndex);
String getFormattedTime();
void setupTime();
