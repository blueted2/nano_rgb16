#include <ArduCAM.h>
#include <SPI.h>
#include <Wire.h>

#include <WiFiNINA.h>

#include "rgb_utils.h"

#define IMG_WIDTH 120
#define IMG_HEIGHT 100

#define CS 10
ArduCAM myCAM(OV2640, CS);

rgb565_t img_buffer[IMG_WIDTH * IMG_HEIGHT];

#define SERVER "192.168.96.206"
#define PORT 3002

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

  // set_jpeg();
}

uint32_t last_change_to_bmp = 0;

void set_bmp()
{
  myCAM.set_format(BMP);
  myCAM.InitCAM();

  myCAM.wrSensorReg8_8(0xff, 0x00);
  myCAM.wrSensorReg8_8(0x5a, IMG_WIDTH / 4);
  myCAM.wrSensorReg8_8(0x5b, IMG_HEIGHT / 4);

  last_change_to_bmp = millis();
}

void set_jpeg()
{
  myCAM.set_format(JPEG);
  myCAM.InitCAM();

  myCAM.OV2640_set_JPEG_size(OV2640_640x480);
  // delay(1000);
}

void update_wifi()
{
  if (WiFi.status() == WL_CONNECTED)
    return;

  Serial.print("connecting to wifi");
  while (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin("Pixel_6148", "12345678");
    delay(100);
    Serial.print(".");
  }

  Serial.println("connected!");
}

void loop()
{
  // return;
  uint32_t width = IMG_WIDTH;
  uint32_t height = IMG_HEIGHT;

  uint32_t delta = capture_bmp(img_buffer);

  // uint32_t delta = 0;
  // for (int j = 0; j < height; j++)
  // {
  //   for (int i = 0; i < width; i++)
  //   {
  //     rgb332_t pix = rgb332_t{y};
  //     // rgb332_t old_pix = rgb332_t{old_y};

  //     // uint8_t y_dist_sq = (y - old_y) * (y - old_y) / 256;

  //     uint8_t y_dist_sq = rgb332_t::sq_dist(pix, old_pix);

  //     // max value of sq_dist for rgb332 is 147 (3 * 7^2)
  //     y_dist_sq = (float(y_dist_sq) / 147 * 256);

  //     delta += y_dist_sq;
  //   }
  // }


  uint16_t now = millis();
  Serial.println(delta);

  if (delta > 1000000 && now - last_change_to_bmp > 5000)
  {
    Serial.println("---MOUVEMENT---");
    set_jpeg();
    delay(500);

    capture_jpeg();
    set_bmp();
    // delay(1000);
  }

  draw_images();
}

void draw_images()
{
  uint32_t width = IMG_WIDTH;
  uint32_t height = IMG_HEIGHT;
  for (int y = 0; y < height; y += 4)
  {
    for (int x = 0; x < width; x += 2)
    {
      rgb565_t top = img_buffer[x + y * width];
      rgb565_t bottom = img_buffer[x + (y + 2) * width];

      print_two_pixels(top, bottom);
    }
    // {
    //   rgb565_t pix = img_buffer[j * width + i];

    //   Serial.print(print_two_pixels(pix.get_r() << 5, pix.get_g() << 5,
    //                                 pix.get_b() << 6));
    //   Serial.print(" ");
    // }

    // for (int i = 0; i < width; i++)
    // {
    //   uint8_t y = img_buffer[j * width + i];

    //   rgb565_t pix = rgb332_t{y};
    //   Serial.print(set_serial_color(pix.get_r_norm(), pix.get_g_norm(),
    //                                 pix.get_b_norm()));
    //   Serial.print(" ");
    // }

    // for (int i = 0; i < width; i++)
    // {
    //   uint8_t y = curr_buf[j * width + i];
    //   uint8_t old_y = prev_buf[j * width + i];

    //   rgb332_t pix = rgb332_t{y};
    //   rgb332_t old_pix = rgb332_t{old_y};

    //   // uint8_t y_dist_sq = (y - old_y) * (y - old_y) / 256;

    //   uint8_t y_dist_sq = rgb332_t::sq_dist(pix, old_pix);

    //   // max value of sq_dist for rgb332 is 147 (3 * 7^2)
    //   y_dist_sq = (float(y_dist_sq) / 147 * 256);

    //   if (y_dist_sq > 200)
    //   {
    //     Serial.println(y_dist_sq);
    //   }
    //   Serial.print(set_serial_color(y_dist_sq, y_dist_sq, y_dist_sq));
    //   Serial.print(" ");
    // }
    Serial.println();
  }

  reset_color();
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


// returns delta with old and new image
uint32_t capture_bmp(rgb565_t *buf)
{

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();

  // Start capture
  myCAM.start_capture();
  delay(0);

  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
  {
    // Serial.println("waiting for capture");
    delay(0);
  }

  uint32_t length = myCAM.read_fifo_length();

  // Serial.println(length);

  myCAM.CS_LOW();
  myCAM.set_fifo_burst(); // Set fifo burst mode


  uint32_t delta = 0;

  for (int y = 0; y < IMG_HEIGHT; y++)
  {
    for (int x = 0; x < IMG_WIDTH; x++)
    {
      uint16_t tmp = SPI.transfer(0x00) << 8;

      tmp |= SPI.transfer(0x00);
      // tmp |= SPI.transfer(0x00);

      rgb565_t new_pixel565 = rgb565_t{tmp};

      rgb565_t old_pixel565 = img_buffer[x + y * IMG_WIDTH];

      // uint8_t y_dist_sq = rgb332_t::sq_dist(pix, old_pix);

      uint32_t dist = rgb565_t::sq_dist(new_pixel565, old_pixel565);

      // max value of sq_dist for rgb332 is 147 (3 * 7^2)
      dist = (float(dist) / 147 * 256);

      delta += dist;
      img_buffer[x + y * IMG_WIDTH] = new_pixel565;
    }
  }

  myCAM.CS_HIGH();
  myCAM.clear_fifo_flag();

  return delta;
}

void capture_jpeg()
{

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  // Start capture
  myCAM.start_capture();


  update_wifi();

  // HttpClient http(client);

  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
  {
    Serial.println("waiting for capture");
    delay(100);
  }

  update_wifi();

  // Serial.println("capture done");

  uint32_t length = myCAM.read_fifo_length();

  Serial.println(length);

  myCAM.CS_LOW();
  myCAM.set_fifo_burst(); // Set fifo burst mode

  WiFiClient client;

  int total_sent = 0;

  if (client.connect(SERVER, PORT))
  {
    client.println("POST / HTTP/1.1");
    client.print("Host: ");
    client.println(SERVER);
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println("Content-Type: image/jpeg");
    client.print("Content-Length: ");
    client.println(length);
    client.println();

    const int BUF_SIZE = 1024;

    uint8_t buf[BUF_SIZE];
    int buf_size = 0;

    update_wifi();
    while (length > 0)
    {

      buf[buf_size] = SPI.transfer(0x00);
      buf_size++;
      length--;

      // if this is the last byte
      if (buf_size == BUF_SIZE)
      {
        int written = client.write(buf, buf_size);
        delay(200);
        Serial.println(written);
        total_sent += buf_size;
        buf_size = 0;
        Serial.print(".");
      }

      // if (length % 100 == 0) {
      //   Serial.println(length);
      // }
      
    }
    if (buf_size > 0)
    {
      client.write(buf, buf_size);
    }
    client.println();

    total_sent += buf_size;

    Serial.println();
    Serial.println(total_sent);

    myCAM.CS_HIGH();
    myCAM.clear_fifo_flag();
  }
  client.flush();
  client.stop();
}

// void write(WiFiClient client, byte *buffer, int size)
// {
//   const int PACKET_SIZE = 1000;
//   for (int i = 0; i <= size / PACKET_SIZE; ++i)
//   {
//     byte *p = buffer + PACKET_SIZE * i;
//     int l = PACKET_SIZE * (i + 1) > size ? size - PACKET_SIZE * i : PACKET_SIZE;
//     int bytesWritten = 0;
//     do
//     {
//       bytesWritten += client.write(p + bytesWritten,
//                                    l - bytesWritten);
//     } while (bytesWritten < l);
//   }
// }

// void jpeg_upload(uint8_t *buffer, uint16_t buffer_len) {
//     if (client.connect(SERVER, PORT))
//     {
//         client.println("POST / HTTP/1.1");
//         client.print("Host: "); client.println(SERVER);
//         client.println("User-Agent: Arduino/1.0");
//         client.println("Connection: close");
//         client.println("Content-Type: image/jpeg");
//         client.print("Content-Length: ");
//         client.println(buffer_len);
//         client.println();
//         client.write(buffer, buffer_len);
//         client.println();
//     }
// }

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
