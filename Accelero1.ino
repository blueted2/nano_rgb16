#include <Arduino_LSM6DS3.h>

//Paramètre qui setup des valeurs par défaut pour:
int degreesX = 0;
int degreesY = 0;
int degreesXplus = 0;
int degreesYplus =0;

//Le SETUP "récupère" les données du capteur LSM6DS3
void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
}

//Interprétation toutes les 0.5s de l'inclinaison de la carte
void loop() {
  float x, y, z;

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
  }

  //Conversion en degré
  x = 100*x;
  degreesX = x;
  y = 100*y;
  degreesY = y;

  /**
  Serial.print(degreesX);
  Serial.print(" - ");
  Serial.print(degreesXplus);
  **/


  //Axe X :
  Serial.println("\n");
  Serial.print("\nAngle X = ");
  Serial.print(degreesX);
  Serial.print(" °");

  //Compare l'inclinaison du capteur sur l'axe avec l'ancienne.
  //Envoi un message dès qu'il y a un soucis
  //Remet les paramètre à l'état d'origine pour éviter le spam
  if (degreesXplus == 0) {
    degreesXplus = degreesX;
  }
  else if (abs(degreesXplus - degreesX) >= 5) {
    Serial.print("\nIntégrité compromise ! sur X");
    degreesXplus = 0;
  }


  //Axe Y :
  Serial.print("\nAngle Y = ");
  Serial.print(degreesY);
  Serial.print(" °");


  //Compare l'inclinaison du capteur sur l'axe avec l'ancienne.
  //Envoi un message dès qu'il y a un soucis
  //Remet les paramètre à l'état d'origine pour éviter le spam
  if (degreesYplus == 0) {
    degreesYplus = degreesY;
  }
  if (abs(degreesYplus - degreesY) >= 5) {
    Serial.print("\nIntégrité compromise ! sur Y");
    degreesYplus = 0;
  }

delay(500);

}
