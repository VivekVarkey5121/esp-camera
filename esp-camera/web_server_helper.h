#pragma once

#include "config.h"

void handle_web_server();
void serve_dashboard(WiFiClient &client);
void serve_photo_buffer(WiFiClient &client);
void serve_rolling_frame(WiFiClient &client, int id);
void send_404(WiFiClient &client);
