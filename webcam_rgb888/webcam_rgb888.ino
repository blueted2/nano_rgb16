#include <HttpClient.h>
#include <WiFiNINA.h>

#define IMG_WIDTH 120
#define IMG_HEIGHT 100

#define SERVER "192.168.238.193"
#define PORT 3002
#define URL "/?width=120&height=100&format=bmp"

struct rgb565_t {
  uint16_t data;

  uint8_t get_r() { return (data >> 11) & 0b11111; }
  uint8_t get_g() { return (data >> 5) & 0b111111; }
  uint8_t get_b() { return (data)&0b11111; }

  uint8_t get_r_norm() { return (data >> 8) & 0b11111000; }
  uint8_t get_g_norm() { return (data >> 3) & 0b11111100; }
  uint8_t get_b_norm() { return (data << 3) & 0b11111000; }

  static rgb565_t from_rgb888(uint8_t r, uint8_t g, uint8_t b) {
    r = r >> 3;
    g = g >> 2;
    b = b >> 3;

    return rgb565_t{(r << 11) | (g << 5) | b};
  }
};

struct rgb332_t {
  uint8_t data;

  uint8_t get_r() { return (data >> 5) & 0b111; }
  uint8_t get_g() { return (data >> 2) & 0b111; }
  uint8_t get_b() { return (data)&0b11; }

  uint8_t get_r_norm() { return (data << 0) & 0b11100000; }
  uint8_t get_g_norm() { return (data << 3) & 0b11100000; }
  uint8_t get_b_norm() { return (data << 6) & 0b11000000; }

  static rgb332_t from_rgb565(rgb565_t pixel) {
    uint8_t r = pixel.get_r() >> 2;
    uint8_t g = pixel.get_g() >> 3;
    uint8_t b = pixel.get_b() >> 3;

    return rgb332_t{(r << 5) | (g << 2) | b};
  }

  static rgb332_t from_rgb888(uint8_t r, uint8_t g, uint8_t b) {
    r = r >> 5;
    g = g >> 5;
    b = b >> 6;

    return rgb332_t{(r << 5) | (g << 2) | b};
  }

  static uint8_t sq_dist(rgb332_t left, rgb332_t right) {
    uint8_t r_diff = abs((left.get_r() - right.get_r()));
    uint8_t g_diff = abs((left.get_g() - right.get_g()));

    // make sure all three channels occupy the same number of bits (3)
    uint8_t b_diff = abs((left.get_b() - right.get_b())) * 2;

    return r_diff * r_diff + g_diff + g_diff + b_diff * b_diff;
  }
};

rgb565_t buf1[IMG_WIDTH * IMG_HEIGHT];
// uint8_t buf2[IMG_WIDTH * IMG_HEIGHT];

rgb565_t *curr_buf = buf1;
rgb565_t *prev_buf = buf1;

WiFiClient client;
HttpClient http(client);

void setup() {
  Serial.begin(9600);

  // check communication with WiFi comule
  while (WiFi.status() == WL_NO_MODULE) {
    Serial.println("// No communication with WiFi module");
  }

  client.setTimeout(100);
  client.setRetry(false);

  http = HttpClient(client);
}

void loop() {

  int status = WiFi.status();

  while (status != WL_CONNECTED) {
    WiFi.begin("Pixel_6148", "12345678");
    status = WiFi.status();
    Serial.println("connecting to wifi");
  }

  if (http.get(SERVER, PORT, URL) == 0) {

    int err = http.skipResponseHeaders();
    int bodyLen = http.contentLength();

    char header[54];
    http.readBytes(header, 54);

    uint32_t width = IMG_WIDTH;
    uint32_t height = IMG_HEIGHT;

    uint32_t _width = *(uint32_t *)(header + 0x12);
    int32_t _height = *(int32_t *)(header + 0x16);

    if (width != _width || height != abs(_height)) {
      Serial.println("Wrong image size, aborting!");
      Serial.println(_width);
      Serial.println(_height);
      http.stop();
      return;
    }

    if (!read_888_as_565(http, curr_buf, _height > 0)) {
      Serial.println("Failed to retrieve image");
      http.stop();
      return;
    }

    for (int j = 0; j < height; j += 2) {
      // for (int i = 0; i < width; i += 2) {
      //   uint8_t old_y = prev_buf[j * width + i];
      //   rgb332_t pix = rgb332_t {old_y};

      //   Serial.print(set_serial_color(pix.get_r() << 5, pix.get_g() << 5,
      //   pix.get_b() << 6)); Serial.print(" ");
      // }

      for (int i = 0; i < width; i += 1) {
        rgb565_t pix_top = curr_buf[j * width + i];
        rgb565_t pix_bottom = curr_buf[(j + 1) * width + i];

        // rgb332_t pix_top = rgb332_t{y};
        // rgb332_t pix_bottom = rgb332_t{y2};

        uint8_t r_top = pix_top.get_r_norm();
        uint8_t g_top = pix_top.get_g_norm();
        uint8_t b_top = pix_top.get_b_norm();

        uint8_t r_bottom = pix_bottom.get_r_norm();
        uint8_t g_bottom = pix_bottom.get_g_norm();
        uint8_t b_bottom = pix_bottom.get_b_norm();

        print_two_pixels(r_top, g_top, b_top, r_bottom, g_bottom, b_bottom);

        // Serial.print(set_serial_color(pix.get_r() << 5, pix.get_g() << 5,
        // pix.get_b() << 6)); Serial.print(" ");
      }

      // for (int i = 0; i < width; i += 2) {
      //   uint8_t y = curr_buf[j * width + i];
      //   uint8_t old_y = prev_buf[j * width + i];

      //   rgb332_t pix = rgb332_t {y};
      //   rgb332_t old_pix = rgb332_t {old_y};

      //   // uint8_t y_dist_sq = (y - old_y) * (y - old_y) / 256;

      //   uint8_t y_dist_sq = rgb332_t::sq_dist(pix, old_pix);

      //   // max value of sq_dist for rgb332 is 147 (3 * 7^2)
      //   y_dist_sq = (float(y_dist_sq) / 147 * 256);

      //   // if (y_dist_sq > 200) {
      //   //   Serial.println(y_dist_sq);
      //   // }
      //   // Serial.print(set_serial_color(y_dist_sq, y_dist_sq, y_dist_sq));
      //   // Serial.print(" ");
      // }
      Serial.println();
    }

    // swap buffers
    rgb565_t *tmp = curr_buf;
    curr_buf = prev_buf;
    prev_buf = tmp;

  } else {
    Serial.println("http error");
  }
  http.stop();
}

void print_two_pixels(uint8_t r_top, uint8_t g_top, uint8_t b_top,
                      uint8_t r_bottom, uint8_t g_bottom, uint8_t b_bottom) {
  Serial.print("\033[38;2;");
  Serial.print(r_top);
  Serial.print(";");
  Serial.print(g_top);
  Serial.print(";");
  Serial.print(b_top);
  Serial.print("m");
  Serial.print("\033[48;2;");
  Serial.print(r_bottom);
  Serial.print(";");
  Serial.print(g_bottom);
  Serial.print(";");
  Serial.print(b_bottom);
  Serial.print("m▀");
}

String set_serial_color(uint8_t r, uint8_t g, uint8_t b) {
  return "\033[48;2;" + String(r) + ";" + String(g) + ";" + String(b) + "m";
}

void read_565_as_332(Stream &stream, rgb332_t *buf) {
  for (int line = 0; line < IMG_HEIGHT; line++) {
    for (int x = 0; x < IMG_WIDTH; x++) {

      uint8_t pix_buf[2];
      stream.readBytes(pix_buf, 2);

      rgb565_t pixel = rgb565_t{pix_buf[0] | (pix_buf[1] << 8)};
      rgb332_t pixel332 = rgb332_t::from_rgb565(pixel);

      *buf = pixel332;
      buf++;
    }
  }
}

void read_888_as_332(Stream &stream, rgb332_t *buf) {
  for (int line = 0; line < IMG_HEIGHT; line++) {
    for (int x = 0; x < IMG_WIDTH; x++) {
      uint8_t bgr[3];

      if (stream.readBytes(bgr, 3) != 3) {
        return;
      }

      rgb332_t pixel332 = rgb332_t::from_rgb888(bgr[2], bgr[1], bgr[0]);

      *buf = pixel332;
      buf++;
    }
    print_two_pixels(255, 255, 255, 255, 255, 255);
  }
  Serial.println();
}

bool read_888_as_565(Stream &stream, rgb565_t *buf, bool backwards) {

  for (int line = 0; line < IMG_HEIGHT; line++) {
    for (int x = 0; x < IMG_WIDTH; x++) {

      int actual_line = line;
      if (backwards) {
        actual_line = IMG_HEIGHT - line - 1;
      }

      int index = actual_line * IMG_WIDTH + x;
      
      uint8_t bgr[3];

      if (stream.readBytes(bgr, 3) != 3) {
        return false;
      }

      rgb565_t pixel565 = rgb565_t::from_rgb888(bgr[2], bgr[1], bgr[0]);

      buf[index] = pixel565;

    }
  }
  return true;
}
