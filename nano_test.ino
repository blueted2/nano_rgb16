#include "rick.c"
#include <WiFiNINA.h>

void setup() {
  Serial.begin(9600);

  // check communication with WiFi comule
  while (WiFi.status() == WL_NO_MODULE) {
    Serial.println("// No communication with WiFi module");
  }
}

void loop() {
  for (int y = 0; y < gimp_image.height; y++) {
    for (int x = 0; x < gimp_image.width; x++) {
      int index = (x + y * gimp_image.width) * gimp_image.bytes_per_pixel;      
      uint16_t pixel = gimp_image.pixel_data[index] | (gimp_image.pixel_data[index + 1] << 8);

      // Serial.print(pixel);

      uint16_t r_mask = 0b1111100000000000;
      uint16_t g_mask = 0b0000011111100000;
      uint16_t b_mask = 0b0000000000011111;

      uint8_t r = (pixel & r_mask) >> 8;
      uint8_t g = (pixel & g_mask) >> 3;
      uint8_t b = (pixel & b_mask) << 3;

      // Serial.print(r);
      // Serial.print(";");
      // Serial.print(g);
      // Serial.print(";");
      // Serial.print(b);
      // Serial.print(";");

      // if(x == 0 and y == 0) {
      //   Serial.println(pixel, HEX);
      //   Serial.println(r, HEX);
      //   Serial.println(g, HEX);
      //   Serial.println(b, HEX);
      // }


      set_serial_color(r, g, b);
      Serial.print("█");
    }
    Serial.println();
  }

  delay(1000);
}

void set_serial_color(uint8_t r, uint8_t g, uint8_t b) {
  Serial.print("\x1b[38;2;");
  Serial.print(r);
  Serial.print(";");
  Serial.print(g);
  Serial.print(";");
  Serial.print(b);
  Serial.print(";249m");
}
