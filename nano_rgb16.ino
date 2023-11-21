#include "rick.c"
#include "rick2.c"
#include <HttpClient.h>
#include <WiFiNINA.h>

void setup() {
  Serial.begin(115200);

  // check communication with WiFi comule
  while (WiFi.status() == WL_NO_MODULE) {
    Serial.println("// No communication with WiFi module");
  }
}

uint8_t buf1[120 * 100];
uint8_t buf2[120 * 100];

uint8_t *curr_buf = buf1;
uint8_t *prev_buf = buf2;

void loop() {
  int status = WiFi.status();

  while (status != WL_CONNECTED) {
    WiFi.begin("Pixel_6148", "12345678");
    status = WiFi.status();
  }

  // Serial.println("Connected to wifi");

  WiFiClient client;

  HttpClient http(client);

  if (http.get("192.168.196.193", 3000, "/capture") == 0) {
    // Serial.print("Got status code: ");
    // Serial.println(http.responseStatusCode());

    int err = http.skipResponseHeaders();
    int bodyLen = http.contentLength();
    // Serial.print("Content length is: ");
    // Serial.println(bodyLen);

    char header[54];
    http.readBytes(header, 54);

    uint32_t *width = (uint32_t *)(header + 0x12);
    int32_t *height = (int32_t *)(header + 0x16);

    // Serial.println(*width);
    // Serial.println(*height);

    for (int j = 0; j < -*height; j++) {
      uint8_t line[3 * *width];

      http.readBytes(line, 3 * *width);
      for (int i = 0; i < *width; i++) {
        uint8_t r = line[3 * i + 2];
        uint8_t g = line[3 * i + 1];
        uint8_t b = line[3 * i + 0];

        uint8_t y = 0.3 * r + 0.6 * g + 0.1 * b;

        curr_buf[j * *width + i] = y;
      }
    }

    // Serial.println("got image");

    for (int j = 0; j < -*height; j += 4) {
      for (int i = 0; i < *width; i += 3) {
        // uint8_t y = curr_buf[j * *width + i];

        uint8_t old_y = prev_buf[j * *width + i];
        // uint8_t y_dist_sq = (y - old_y) * (y - old_y) / 256;
        Serial.print(set_serial_color(old_y, old_y, old_y));
        Serial.print(" ");

        // Serial.println(bgr[2]);
        // Serial.println(bgr[1]);
        // Serial.println(bgr[0]);
      }

      for (int i = 0; i < *width; i += 3) {
        uint8_t y = curr_buf[j * *width + i];

        // uint8_t old_y = prev_buf[j * *width + i];
        // uint8_t y_dist_sq = (y - old_y) * (y - old_y) / 256;
        Serial.print(set_serial_color(y, y, y));
        Serial.print(" ");

        // Serial.println(bgr[2]);
        // Serial.println(bgr[1]);
        // Serial.println(bgr[0]);
      }
      for (int i = 0; i < *width; i += 3) {
        uint8_t y = curr_buf[j * *width + i];

        uint8_t old_y = prev_buf[j * *width + i];
        uint8_t y_dist_sq = (y - old_y) * (y - old_y) / 256;
        Serial.print(set_serial_color(y_dist_sq, y_dist_sq, y_dist_sq));
        Serial.print(" ");

        // Serial.println(bgr[2]);
        // Serial.println(bgr[1]);
        // Serial.println(bgr[0]);
      }
      Serial.println();
    }

    uint8_t *tmp = curr_buf;
    curr_buf = prev_buf;
    prev_buf = tmp;

    // char buffer[16];
    // http.readBytes(buffer, 14);

    // Serial.println(buffer);

    // int offset = buffer[10];
    // int offset1 = buffer[11];

    // Serial.println(offset);
    // Serial.println(offset1);
  }

  http.stop();

  // if (client.connect("192.168.196.193", 3000)) {
  //   Serial.println("connected to server");

  //   client.println("GET /capture HTTP/1.1");
  //   client.println("Host: localhost");
  //   client.println("Connection: close");

  //   client.println();

  //   while(client.connected()) {
  //     if(client.available()){
  //       // read an incoming byte from the server and print it to serial
  //       monitor: char c = client.read(); Serial.print(c);
  //     }
  //   }

  // }
  // delay(1000);
}

// void loop() {
//   uint16_t prev_pixel = 12;

//   gimp_image.pixel_data[0] = 0;
//   // rick2_bmp_data.pixel_data[0] = 0;

//   for (int y = 0; y < gimp_image.height; y++) {
//     String output_buffer = "";
//     for (int x = 0; x < gimp_image.width; x++) {
//       int index = (x + y * gimp_image.width) * gimp_image.bytes_per_pixel;
//       uint16_t pixel = gimp_image.pixel_data[index] |
//                        (gimp_image.pixel_data[index + 1] << 8);

//       if (prev_pixel != pixel) {
//         prev_pixel = pixel;

//         // Serial.print(pixel);

//         uint16_t r_mask = 0b1111100000000000;
//         uint16_t g_mask = 0b0000011111100000;
//         uint16_t b_mask = 0b0000000000011111;

//         uint8_t r = (pixel & r_mask) >> 8;
//         uint8_t g = (pixel & g_mask) >> 3;
//         uint8_t b = (pixel & b_mask) << 3;

//         // Serial.print(r);
//         // Serial.print(";");
//         // Serial.print(g);
//         // Serial.print(";");
//         // Serial.print(b);
//         // Serial.print(";");

//         // if(x == 0 and y == 0) {
//         //   Serial.println(pixel, HEX);
//         //   Serial.println(r, HEX);
//         //   Serial.println(g, HEX);
//         //   Serial.println(b, HEX);
//         // }
//         output_buffer += set_serial_color(r, g, b);
//       }
//       // Serial.print(" ");
//       output_buffer += " ";
//     }
//     Serial.println(output_buffer);
//   }

//   // prev_pixel = 12;

//   for (int y = 0; y < rick2_bmp_data.height; y++) {
//     String output_buffer = "";
//     for (int x = 0; x < rick2_bmp_data.width; x++) {
//       int index =
//           (x + y * rick2_bmp_data.width) * rick2_bmp_data.bytes_per_pixel;
//       uint16_t pixel = rick2_bmp_data.pixel_data[index] |
//                        (rick2_bmp_data.pixel_data[index + 1] << 8);

//       if (prev_pixel != pixel) {
//         prev_pixel = pixel;

//         // Serial.print(pixel);

//         uint16_t r_mask = 0b1111100000000000;
//         uint16_t g_mask = 0b0000011111100000;
//         uint16_t b_mask = 0b0000000000011111;

//         uint8_t r = (pixel & r_mask) >> 8;
//         uint8_t g = (pixel & g_mask) >> 3;
//         uint8_t b = (pixel & b_mask) << 3;

//         // Serial.print(r);
//         // Serial.print(";");
//         // Serial.print(g);
//         // Serial.print(";");
//         // Serial.print(b);
//         // Serial.print(";");

//         // if(x == 0 and y == 0) {
//         //   Serial.println(pixel, HEX);
//         //   Serial.println(r, HEX);
//         //   Serial.println(g, HEX);
//         //   Serial.println(b, HEX);
//         // }
//         output_buffer += set_serial_color(r, g, b);
//       }
//       // Serial.print(" ");
//       output_buffer += " ";
//     }
//     Serial.println(output_buffer);
//   }
//   // Serial.println("ding!!");
//   uint32_t max_r_r = 0;
//   uint32_t max_g_g = 0;
//   uint32_t max_b_b = 0;

//   for (int y = 0; y < rick2_bmp_data.height; y++) {
//     String output_buffer = "";
//     for (int x = 0; x < rick2_bmp_data.width; x++) {
//       int index =
//           (x + y * rick2_bmp_data.width) * rick2_bmp_data.bytes_per_pixel;
//       uint16_t pixel1 = rick2_bmp_data.pixel_data[index] |
//                         (rick2_bmp_data.pixel_data[index + 1] << 8);

//       uint16_t pixel2 = gimp_image.pixel_data[index] |
//                         (gimp_image.pixel_data[index + 1] << 8);

//       uint16_t r_mask = 0b1111100000000000;
//       uint16_t g_mask = 0b0000011111100000;
//       uint16_t b_mask = 0b0000000000011111;

//       uint8_t r1 = (pixel1 & r_mask) >> 8;
//       uint8_t g1 = (pixel1 & g_mask) >> 3;
//       uint8_t b1 = (pixel1 & b_mask) << 3;

//       uint8_t r2 = (pixel2 & r_mask) >> 8;
//       uint8_t g2 = (pixel2 & g_mask) >> 3;
//       uint8_t b2 = (pixel2 & b_mask) << 3;

//       // uint32_t diff = (r1 - r2)*(r1-r2) + (g1 - g2)*(g1-g2) + (b1 -
//       b2)*(b1-b2);

//       uint32_t r_r = (r1-r2)*(r1-r2);
//       uint32_t g_g = (g1-g2)*(g1-g2);
//       uint32_t b_b = (b1-b2)*(b1-b2);

//       max_r_r = max(max_r_r, r_r);
//       max_g_g = max(max_g_g, g_g);
//       max_b_b = max(max_b_b, b_b);
//       // Serial.print(r);
//       // Serial.print(";");
//       // Serial.print(g);
//       // Serial.print(";");
//       // Serial.print(b);
//       // Serial.print(";");

//       // if(x == 0 and y == 0) {
//       //   Serial.println(pixel, HEX);
//       //   Serial.println(r, HEX);
//       //   Serial.println(g, HEX);
//       //   Serial.println(b, HEX);
//       // }

//       output_buffer += set_serial_color(r_r / (25600 / 256), g_g / (26896 /
//       256), b_b / (25600 / 256)); output_buffer += " ";
//     }
//     Serial.println(output_buffer);
//     // Serial.print(" ");
//   }
//   // Serial.println(max_r_r);
//   // Serial.println(max_g_g);
//   // Serial.println(max_b_b);
// }

String set_serial_color(uint8_t r, uint8_t g, uint8_t b) {
  return "\033[48;2;" + String(r) + ";" + String(g) + ";" + String(b) + "m";

  // Serial.print(output);
  // Serial.print("\033[48;2;");
  // Serial.print(r);
  // Serial.print(";");
  // Serial.print(g);
  // Serial.print(";");
  // Serial.print(b);
  // Serial.print("m");
  // Serial.print(";249m");
}
