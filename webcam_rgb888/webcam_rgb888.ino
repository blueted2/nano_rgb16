#include <HttpClient.h>
#include <WiFiNINA.h>

#define IMG_WIDTH 120
#define IMG_HEIGHT 100

uint8_t buf1[IMG_WIDTH * IMG_HEIGHT];
uint8_t buf2[IMG_WIDTH * IMG_HEIGHT];

uint8_t *curr_buf = buf1;
uint8_t *prev_buf = buf2;

struct rgb565_t {
  uint16_t data;

  uint8_t get_r() { return (data >> 11) & 0b11111; }
  uint8_t get_g() { return (data >> 5) & 0b111111; }
  uint8_t get_b() { return (data)&0b11111; }
};

struct rgb332_t {
  uint8_t data;

  uint8_t get_r() { return (data >> 5) & 0b111; }
  uint8_t get_g() { return (data >> 2) & 0b111; }
  uint8_t get_b() { return (data)&0b11; }

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
    uint8_t b_diff = abs((left.get_b() - right.get_b())) * 2; // make sure all three channels occupy the same number of bits (3)

    return r_diff * r_diff + g_diff + g_diff + b_diff * b_diff;
  }
};

void setup() {
  Serial.begin(9600);

  // check communication with WiFi comule
  while (WiFi.status() == WL_NO_MODULE) {
    Serial.println("// No communication with WiFi module");
  }
}

void loop() {

  int status = WiFi.status();

  while (status != WL_CONNECTED) {
    WiFi.begin("Pixel_6148", "12345678");
    status = WiFi.status();
    Serial.println("connecting to wifi");
  }

  WiFiClient client;

  HttpClient http(client);

  if (http.get("192.168.238.193", 3000, "/capture") == 0) {

    int err = http.skipResponseHeaders();
    int bodyLen = http.contentLength();

    char header[54];
    http.readBytes(header, 54);

    
    uint32_t width = IMG_WIDTH;
    uint32_t height = IMG_HEIGHT;

    uint32_t _width = *(uint32_t *)(header + 0x12);
    uint32_t _height = -*(int32_t *)(header + 0x16); // negative indicates "top-down" order

    if(width != _width || height != _height) {
      Serial.println("Wrong image size, aborting!");
      return;
    }
    

    read_888_as_332(http, (rgb332_t *)curr_buf);
 
    for (int j = 0; j < height; j += 4) {
      for (int i = 0; i < width; i += 2) {
        uint8_t old_y = prev_buf[j * width + i];
        rgb332_t pix = rgb332_t {old_y};

        Serial.print(set_serial_color(pix.get_r() << 5, pix.get_g() << 5, pix.get_b() << 6));
        Serial.print(" ");
      }


      for (int i = 0; i < width; i += 2) {
        uint8_t y = curr_buf[j * width + i];

        rgb332_t pix = rgb332_t {y};
        Serial.print(set_serial_color(pix.get_r() << 5, pix.get_g() << 5, pix.get_b() << 6));
        Serial.print(" ");
      }

      for (int i = 0; i < width; i += 2) {
        uint8_t y = curr_buf[j * width + i];
        uint8_t old_y = prev_buf[j * width + i];

        rgb332_t pix = rgb332_t {y};
        rgb332_t old_pix = rgb332_t {old_y};

     
        // uint8_t y_dist_sq = (y - old_y) * (y - old_y) / 256;

        uint8_t y_dist_sq = rgb332_t::sq_dist(pix, old_pix);

        // max value of sq_dist for rgb332 is 147 (3 * 7^2)
        y_dist_sq = (float(y_dist_sq) / 147 * 256);

        

        if (y_dist_sq > 200) {
          Serial.println(y_dist_sq);
        }
        Serial.print(set_serial_color(y_dist_sq, y_dist_sq, y_dist_sq));
        Serial.print(" ");
      }
      Serial.println();
    }


    // swap buffers
    uint8_t *tmp = curr_buf;
    curr_buf = prev_buf;
    prev_buf = tmp;

  }
  http.stop();
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

      stream.readBytes(bgr, 3);

      rgb332_t pixel332 = rgb332_t::from_rgb888(bgr[2], bgr[1], bgr[0]);

      *buf = pixel332;
      buf++;
    }
  }
}

