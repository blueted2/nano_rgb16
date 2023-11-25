#include <ArduCAM.h>
#include <SPI.h>
#include <Wire.h>

#include <HttpClient.h>
#include <WiFiNINA.h>

#define IMG_WIDTH 120
#define IMG_HEIGHT 100

#define CS 10

uint8_t buf1[IMG_WIDTH * IMG_HEIGHT];
uint8_t buf2[IMG_WIDTH * IMG_HEIGHT];

uint8_t *curr_buf = buf1;
uint8_t *prev_buf = buf2;

ArduCAM myCAM(OV2640, CS);

struct rgb565_t
{
  uint16_t data;

  uint8_t get_r() { return (data >> 11) & 0b11111; }
  uint8_t get_g() { return (data >> 5) & 0b111111; }
  uint8_t get_b() { return (data) & 0b11111; }
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

    return rgb332_t{(r << 5) | (g << 2) | b};
  }

  static rgb332_t from_rgb888(uint8_t r, uint8_t g, uint8_t b)
  {
    r = r >> 5;
    g = g >> 5;
    b = b >> 6;

    return rgb332_t{(r << 5) | (g << 2) | b};
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

void setup()
{
  Serial.begin(9600);

  Wire.begin();

  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

  // initialize SPI:
  SPI.begin();

  // Reset the CPLD
  myCAM.write_reg(0x07, 0x80);
  delay(100);
  myCAM.write_reg(0x07, 0x00);
  delay(100);

  while (1)
  {
    uint8_t temp;
    // Check if the ArduCAM SPI bus is OK
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = myCAM.read_reg(ARDUCHIP_TEST1);
    if (temp != 0x55)
    {
      Serial.println(F("ACK CMD SPI interface Error!END"));
      delay(1000);
      continue;
    }
    else
    {
      Serial.println(F("ACK CMD SPI interface OK.END"));
      break;
    }
  }

  while (1)
  {
    uint8_t vid, pid;
    // Check if the camera module type is OV2640
    myCAM.wrSensorReg8_8(0xff, 0x01);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);

    if ((vid != 0x26) && ((pid != 0x41) || (pid != 0x42)))
    {
      Serial.println(F("ACK CMD Can't find OV2640 module!"));
      delay(1000);
      continue;
    }
    else
    {
      Serial.println(F("ACK CMD OV2640 detected.END"));
      break;
    }
  }

  set_bmp();
}

void set_bmp()
{
  myCAM.set_format(BMP);
  myCAM.InitCAM();

  myCAM.wrSensorReg8_8(0xff, 0x00);
  myCAM.wrSensorReg8_8(0x5a, IMG_WIDTH / 4);
  myCAM.wrSensorReg8_8(0x5b, IMG_HEIGHT / 4);
}

void set_jpeg()
{
  myCAM.set_format(JPEG);
  myCAM.InitCAM();

  myCAM.OV2640_set_JPEG_size(OV2640_640x480);
  delay(1000);
}

void loop()
{

  uint32_t width = IMG_WIDTH;
  uint32_t height = IMG_HEIGHT;

  capture_bmp((rgb332_t *)curr_buf);

  int status = WiFi.status();

  while (status != WL_CONNECTED)
  {
    WiFi.begin("Votre Box Mobile", "Grenier.c.16");
    status = WiFi.status();
    Serial.println("connecting to wifi");
  }

  uint32_t delta = 0;
  for (int j = 0; j < height; j++)
  {
    for (int i = 0; i < width; i++)
    {
      uint8_t y = curr_buf[j * width + i];
      uint8_t old_y = prev_buf[j * width + i];

      rgb332_t pix = rgb332_t{y};
      rgb332_t old_pix = rgb332_t{old_y};

      // uint8_t y_dist_sq = (y - old_y) * (y - old_y) / 256;

      uint8_t y_dist_sq = rgb332_t::sq_dist(pix, old_pix);

      // max value of sq_dist for rgb332 is 147 (3 * 7^2)
      y_dist_sq = (float(y_dist_sq) / 147 * 256);

      delta += y_dist_sq;
    }
  }

  Serial.println(delta);

  if (delta > 50000)
  {
    Serial.println("---MOUVEMENT---");
    set_jpeg();
    delay(1000);

    capture_jpeg();
    set_bmp();
    delay(1000);
  }

  // draw_images();

  // swap buffers
  uint8_t *tmp = curr_buf;
  curr_buf = prev_buf;
  prev_buf = tmp;
}

void draw_images()
{
  uint32_t width = IMG_WIDTH;
  uint32_t height = IMG_HEIGHT;
  for (int j = 0; j < height; j += 2)
  {
    for (int i = 0; i < width; i++)
    {
      uint8_t old_y = prev_buf[j * width + i];
      rgb332_t pix = rgb332_t{old_y};

      Serial.print(set_serial_color(pix.get_r() << 5, pix.get_g() << 5,
                                    pix.get_b() << 6));
      Serial.print(" ");
    }

    for (int i = 0; i < width; i++)
    {
      uint8_t y = curr_buf[j * width + i];

      rgb332_t pix = rgb332_t{y};
      Serial.print(set_serial_color(pix.get_r() << 5, pix.get_g() << 5,
                                    pix.get_b() << 6));
      Serial.print(" ");
    }

    for (int i = 0; i < width; i++)
    {
      uint8_t y = curr_buf[j * width + i];
      uint8_t old_y = prev_buf[j * width + i];

      rgb332_t pix = rgb332_t{y};
      rgb332_t old_pix = rgb332_t{old_y};

      // uint8_t y_dist_sq = (y - old_y) * (y - old_y) / 256;

      uint8_t y_dist_sq = rgb332_t::sq_dist(pix, old_pix);

      // max value of sq_dist for rgb332 is 147 (3 * 7^2)
      y_dist_sq = (float(y_dist_sq) / 147 * 256);

      if (y_dist_sq > 200)
      {
        Serial.println(y_dist_sq);
      }
      Serial.print(set_serial_color(y_dist_sq, y_dist_sq, y_dist_sq));
      Serial.print(" ");
    }
    Serial.println();
  }
}
String set_serial_color(uint8_t r, uint8_t g, uint8_t b)
{
  return "\033[48;2;" + String(r) + ";" + String(g) + ";" + String(b) + "m";
}

// TODO: double for loop could be replace with single
void read_565_as_332(Stream &stream, rgb332_t *buf)
{
  for (int line = 0; line < IMG_HEIGHT; line++)
  {
    for (int x = 0; x < IMG_WIDTH; x++)
    {

      uint8_t pix_buf[2];
      stream.readBytes(pix_buf, 2);

      rgb565_t pixel = rgb565_t{pix_buf[0] | (pix_buf[1] << 8)};
      rgb332_t pixel332 = rgb332_t::from_rgb565(pixel);

      *buf = pixel332;
      buf++;
    }
  }
}

void read_888_as_332(Stream &stream, rgb332_t *buf)
{
  for (int line = 0; line < IMG_HEIGHT; line++)
  {
    for (int x = 0; x < IMG_WIDTH; x++)
    {
      uint8_t bgr[3];

      stream.readBytes(bgr, 3);

      rgb332_t pixel332 = rgb332_t::from_rgb888(bgr[2], bgr[1], bgr[0]);

      *buf = pixel332;
      buf++;
    }
  }
}

void capture_bmp(rgb332_t *buf)
{

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  // Start capture
  myCAM.start_capture();

  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
  {
    // Serial.println("waiting for capture");
    delay(100);
  }

  uint32_t length = myCAM.read_fifo_length();

  // Serial.println(length);

  myCAM.CS_LOW();
  myCAM.set_fifo_burst(); // Set fifo burst mode

  for (int line = 0; line < IMG_HEIGHT; line++)
  {
    for (int x = 0; x < IMG_WIDTH; x++)
    {
      uint16_t tmp = SPI.transfer(0x00) << 8;

      // tmp |= SPI.transfer(0x00) << 8;
      tmp |= SPI.transfer(0x00);

      rgb565_t pixel565 = rgb565_t{tmp};

      rgb332_t pixel332 = rgb332_t::from_rgb565(pixel565);

      *buf = pixel332;
      buf++;
    }
  }

  myCAM.CS_HIGH();
  myCAM.clear_fifo_flag();
}

void capture_jpeg()
{

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  // Start capture
  myCAM.start_capture();

  int status = WiFi.status();

  while (status != WL_CONNECTED)
  {
    WiFi.begin("Votre Box Mobile", "Grenier.c.16");
    status = WiFi.status();
    Serial.println("connecting to wifi");
  }

  WiFiClient client;

  // HttpClient http(client);

  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
  {
    // Serial.println("waiting for capture");
    delay(100);
  }

  if (client.connect("192.168.50.206", 3001))
  {
    client.println("POST / HTTP/1.1");
    client.println("Host: 192.168.50.206");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(11);
    client.println();
    client.println("1234567890");
    client.println();
    client.println();
  }

  // http.startRequest("192.168.50.206", 3001, "/", HTTP_METHOD_POST, NULL);

  // http.sendHeader("type", "alskdjfhasdf");

  // http.finishRequest();

  // if(http.post("192.168.50.206", 3001, "/")) {
  // http.sendHeader("type", "alskdjfhasdf");

  // }

  uint32_t length = myCAM.read_fifo_length();

  // Serial.println(length);

  myCAM.CS_LOW();
  myCAM.set_fifo_burst(); // Set fifo burst mode

  while (length > 0)
  {
    uint16_t tmp = SPI.transfer(0x00) << 8;
    Serial.print(tmp);
    length--;
  }

  myCAM.CS_HIGH();
  myCAM.clear_fifo_flag();
}

void write(WiFiClient client, byte *buffer, int size)
{
  const int PACKET_SIZE = 1000;
  for (int i = 0; i <= size / PACKET_SIZE; ++i)
  {
    byte *p = buffer + PACKET_SIZE * i;
    int l = PACKET_SIZE * (i + 1) > size ? size - PACKET_SIZE * i : PACKET_SIZE;
    int bytesWritten = 0;
    do
    {
      bytesWritten += client.write(p + bytesWritten,
                                   l - bytesWritten);
    } while (bytesWritten < l);
  }
}

// uint8_t read_fifo_burst(ArduCAM myCAM)
// {
//   uint8_t temp = 0, temp_last = 0;
//   uint32_t length = 0;
//   length = myCAM.read_fifo_length();
//   Serial.println(length, DEC);
//   if (length >= MAX_FIFO_SIZE) //512 kb
//   {
//     Serial.println(F("ACK CMD Over size.END"));
//     return 0;
//   }
//   if (length == 0 ) //0 kb
//   {
//     Serial.println(F("ACK CMD Size is 0.END"));
//     return 0;
//   }
//   myCAM.CS_LOW();
//   myCAM.set_fifo_burst();//Set fifo burst mode
//   temp =  SPI.transfer(0x00);
//   length --;
//   while ( length-- )
//   {
//     temp_last = temp;
//     temp =  SPI.transfer(0x00);
//     if (is_header == true)
//     {
//       Serial.write(temp);
//     }
//     else if ((temp == 0xD8) & (temp_last == 0xFF))
//     {
//       is_header = true;
//       Serial.println(F("ACK CMD IMG END"));
//       Serial.write(temp_last);
//       Serial.write(temp);
//     }
//     if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
//       break;
//     delayMicroseconds(15);
//   }
//   myCAM.CS_HIGH();
//   is_header = false;
//   return 1;
// }
