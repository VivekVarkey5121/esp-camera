#pragma once

#include "config.h"

void handle_draw_screen();
void handle_touch_screen();
void handle_button_input();
void handle_viewfinder();
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
void change_mode();
void drawTriggerButton(uint16_t color, uint16_t textColor, const char *label);
