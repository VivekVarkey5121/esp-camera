#pragma once

#include "config.h"

void add_live_photo_frame_to_buffer(camera_fb_t *fb);
void Camera_task();
void codeForCameraTask(void *parameter);
void Take_Photo();
void Start_Video();
void End_Video();
