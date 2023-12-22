
struct rgb565_t
{
  uint16_t data;

  uint8_t get_r() { return (data >> 11) & 0b11111; }
  uint8_t get_g() { return (data >> 5) & 0b111111; }
  uint8_t get_b() { return (data) & 0b11111; }

  uint8_t get_r_norm() { return (data >> 8) & 0b11111000; }
  uint8_t get_g_norm() { return (data >> 3) & 0b11111100; }
  uint8_t get_b_norm() { return (data << 3) & 0b11111000; }

  static uint32_t sq_dist(rgb565_t left, rgb565_t right)
  {
    uint8_t r_diff = abs((left.get_r_norm() - right.get_r_norm())) / 4;
    uint8_t g_diff = abs((left.get_g_norm() - right.get_g_norm())) / 4;
    uint8_t b_diff = abs((left.get_b_norm() - right.get_b_norm())) / 4;

    return r_diff * r_diff + g_diff + g_diff + b_diff * b_diff;
  }
};

struct rgb332_t
{
  uint8_t data;

  uint8_t get_r() { return (data >> 5) & 0b111; }
  uint8_t get_g() { return (data >> 2) & 0b111; }
  uint8_t get_b() { return (data) & 0b11; }

  static rgb332_t from_rgb565(rgb565_t pixel)
  {
    uint8_t r = pixel.get_r() >> 2;
    uint8_t g = pixel.get_g() >> 3;
    uint8_t b = pixel.get_b() >> 3;

    uint8_t data = (r << 5) | (g << 2) | b;

    return rgb332_t{data};
  }

  static rgb332_t from_rgb888(uint8_t r, uint8_t g, uint8_t b)
  {
    r = r >> 5;
    g = g >> 5;
    b = b >> 6;

    uint8_t data = (r << 5) | (g << 2) | b;

    return rgb332_t{data};
  }

  static uint8_t sq_dist(rgb332_t left, rgb332_t right)
  {
    uint8_t r_diff = abs((left.get_r() - right.get_r()));
    uint8_t g_diff = abs((left.get_g() - right.get_g()));

    // make sure all three channels occupy the same number of bits (3)
    uint8_t b_diff = abs((left.get_b() - right.get_b())) * 2;

    return r_diff * r_diff + g_diff + g_diff + b_diff * b_diff;
  }
};

void print_two_pixels(rgb565_t top, rgb565_t bottom) {

  uint8_t r_top = top.get_r_norm();
  uint8_t g_top = top.get_g_norm();
  uint8_t b_top = top.get_b_norm();

  uint8_t r_bottom = bottom.get_r_norm();
  uint8_t g_bottom = bottom.get_g_norm();
  uint8_t b_bottom = bottom.get_b_norm();

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


void reset_color() {
  Serial.print("\033[0m");
}