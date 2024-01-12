#include <ArduCAM.h>
#include <SPI.h>
#include <Wire.h>

#include <WiFiNINA.h>
#include <HttpClient.h>

#include <Arduino_LSM6DS3.h>

#include "rgb_utils.h"

#define IMG_WIDTH 120
#define IMG_HEIGHT 100

#define CS 10
ArduCAM myCAM(OV2640, CS);

rgb565_t img_buffer[IMG_WIDTH * IMG_HEIGHT];

#define SERVER "192.168.27.206"
#define PORT 3002

// #define SERVER_CAPTURE
#define MOUVEMENT_DETECT

void setup()
{
  Serial.begin(9600);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  Wire.begin();

  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

#ifndef SERVER_CAPTURE
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
#endif
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
  // immediately return without printing message if already connected
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

  // if(checkIntegrity()) {
  //   Serial.println("=== CAMÉRA DEPLACÉE ===");
  // }

  // return;
  uint32_t width = IMG_WIDTH;
  uint32_t height = IMG_HEIGHT;

#ifdef SERVER_CAPTURE

  char HOST_NAME[] = "192.168.27.206";
  char PATH_NAME[] = "/capture?format=bmp&width=120&height=100";

  WiFiClient client;
  HttpClient http = HttpClient(client);

  update_wifi();

  uint32_t delta = 0;

  if (http.get(HOST_NAME, PORT, PATH_NAME) == 0)
  {

    int err = http.skipResponseHeaders();
    int bodyLen = http.contentLength();

    char header[54];
    http.readBytes(header, 54);

    uint32_t width = IMG_WIDTH;
    uint32_t height = IMG_HEIGHT;

    uint32_t _width = *(uint32_t *)(header + 0x12);
    int32_t _height = *(int32_t *)(header + 0x16);

    if (width != _width || height != abs(_height))
    {
      Serial.println("Wrong image size, aborting!");
      Serial.println(_width);
      Serial.println(_height);
      http.stop();
      return;
    }

    delta = read_888_as_565(http, img_buffer, 120 * 100);
  }
  else
  {
    Serial.println("http error");
  }

  http.stop();

#else
  // overwrite last image and return difference with new image (sum of sqaure distances between pixel values)
  uint32_t delta = capture_bmp(img_buffer);

#endif
  uint16_t now = millis();
  Serial.println(delta);




#ifndef SERVER_CAPTURE
#ifdef MOUVEMENT_DETECT
  if (delta > 1000000 && now - last_change_to_bmp > 5000)
  {
    Serial.println("---MOUVEMENT---");
    set_jpeg();
    delay(500);

    capture_jpeg();
    set_bmp();
    // delay(1000);
  }
  else if (checkIntegrity())
  {
    Serial.println("---CAMÉRA DEPLACÉE---");
    set_jpeg();
    delay(500);

    capture_jpeg();
    set_bmp();
    // delay(1000);
  }
#endif
#endif

  draw_image(img_buffer, IMG_WIDTH, IMG_HEIGHT, 1, false);
}

void draw_image(rgb565_t *buf, uint32_t width, uint32_t height, uint32_t step, bool flip)
{
  for (int y = 0; y < height; y += (step * 2))
  {
    for (int x = 0; x < width; x += step)
    {
      rgb565_t top;
      rgb565_t bottom;

      // jank but whatever
      if (flip)
      {
        top = img_buffer[x + (height - y - 1) * width];
        bottom = img_buffer[x + (height - y - step - 1) * width];
      }
      else
      {
        top = img_buffer[x + y * width];
        bottom = img_buffer[x + (y + step) * width];
      }

      print_two_pixels(top, bottom);
    }
    reset_color();
    Serial.println();
  }
}

uint32_t read_888_as_565(Stream &stream, rgb565_t *buf, uint32_t len)
{
  uint32_t delta = 0;
  for (int x = 0; x < len; x++)
  {
    uint8_t bgr[3];

    stream.readBytes(bgr, 3);

    // store previous pixel
    rgb565_t old_pixel565 = *buf;

    // replace
    *buf = rgb565_t::from_rgb888(bgr[2], bgr[1], bgr[0]);

    uint32_t dist = rgb565_t::sq_dist(*buf, old_pixel565);

    // max value of sq_dist for rgb332 is 147 (3 * 7^2)
    dist = (float(dist) / 147 * 256);

    delta += dist;

    buf++;
  }

  return delta;
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


//La fonction checkIntegrity vérifie que la carte n'a pas été bougé lors de la capture d'images
//On renvoit true si le mouvement est perçu, sinon false.
bool checkIntegrity () {

  //Paramètre des valeurs par défaut:
  int degreesX = 0;
  int degreesY = 0;
  static int degreesXplus = 0;
  static int degreesYplus =0;

  float x, y, z;

  //On auvre la lecture des données de l'accéléromètres
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
  }

  //Conversion en degré (°)
  x = 100*x;
  degreesX = x;
  y = 100*y;
  degreesY = y;

  // Ici on cherche à vérifié que entre 2 positions précédentes, 
  // il n'y a pas une distance supérieur à 5° sur les axes X et Y

  //On place la valeur de la première donnée capturé (degreesX ou degreesY)
  //dans degreesXplus ou degreesYplus si elles valent 0
  if ((degreesXplus == 0) || (degreesYplus == 0)) {
    degreesXplus = degreesX;
    degreesYplus = degreesY;
  }

  //On vérifit que la différence absolu entre la première mesure d'angle et celle juste après ne dépasse
  //pas 5°. Cette valeur a été déterminé expérimentalement pour garantir un véritable mouvement volontaire.
  //On remet alors les valeurs degreesXplus et degreesYplus à 0 pour éviter l'envoi répété de notification 
  //si la carte reste en position "dégradé".
  if ((abs(degreesXplus - degreesX) >= 5) || (abs(degreesYplus - degreesY) >= 5)) {
    degreesXplus = 0;
    degreesYplus = 0;
    return true;
  }
  return false;
}
