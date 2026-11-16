#include "web_server_helper.h"
#include "camera_helper.h"

void handle_web_server()
{
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  Serial.println("\n[WEB SERVER] Incoming connection detected.");
  String request = client.readStringUntil('\r');
  client.flush();

  if (request.indexOf("GET / ") >= 0) {
    serve_dashboard(client);
  }
  else if (request.indexOf("GET /photo") >= 0) {
    serve_photo_buffer(client);
  }
  else if (request.indexOf("GET /trigger") >= 0) {
    Take_Photo();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Live Photo Captured and Saved to RAM!");
  }
  else if (request.indexOf("GET /buffer") >= 0) {
    int idIndex = request.indexOf("id=");
    if (idIndex != -1) {
      int id = request.substring(idIndex + 3).toInt();
      serve_rolling_frame(client, id);
    }
    else {
      send_404(client);
    }
  }
  else {
    send_404(client);
  }

  client.stop();
}

void serve_photo_buffer(WiFiClient &client)
{
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

void serve_rolling_frame(WiFiClient &client, int id)
{
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

void serve_dashboard(WiFiClient &client)
{
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
      const totalFrames = 30;
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

void send_404(WiFiClient &client)
{
  client.println("HTTP/1.1 404 Not Found");
  client.println("Connection: close");
  client.println();
}
