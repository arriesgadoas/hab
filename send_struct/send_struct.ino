
/* Example from Sandeep Mistry
   With mods from AliExpress/LilyGo docs
   For TTGo ESP32 LoRa-OLED board
   http://www.lilygo.cn/down_view.aspx?TypeId=11&Id=78&Fid=t14:11:14
   Based on SX1276 LoRa Radio
   http://www.semtech.com/apps/product.php?pn=SX1276
   RMB 29Nov2017-
*/

#include <SPI.h>
#include <LoRa.h>       // https://github.com/sandeepmistry/arduino-LoRa


// SPI LoRa Radio
#define LORA_SCK 5        // GPIO5 - SX1276 SCK
#define LORA_MISO 19     // GPIO19 - SX1276 MISO
#define LORA_MOSI 27    // GPIO27 -  SX1276 MOSI
#define LORA_CS 18     // GPIO18 -   SX1276 CS
#define LORA_RST 14   // GPIO14 -    SX1276 RST
#define LORA_IRQ 26  // GPIO26 -     SX1276 IRQ (interrupt request)


typedef struct r{
  byte key;
  byte command;
  byte id;
  unsigned long ec;
  byte sal;
  float dO;
  float temp;
  float pH;
  float chl;
  float volts;
  byte level;
  byte sleepTime;
};

  struct r sensorReading;

//const int unionSize = sizeof(sensorReading);
//
//typedef union loraPacket{
//  sensorReading packetData;
//  byte byteArray[unionSize];
//  }

//String getValuebyIndex(String s, char delimiter, int index) {
//  int found = 0;
//  int strIndex[] = {0, -1};
//
//  int maxIndex = s.length();
//
//  for (int i = 0; i <= maxIndex && found <= index; i++) {
//    if (s.charAt(i) == delimiter || i == maxIndex) {
//      found++;
//      strIndex[0] = strIndex[1] + 1;
//      strIndex[1] = (i == maxIndex) ? i + 1 : i;
//    }
//  }
//  return found > index ? s.substring(strIndex[0], strIndex[1]) : "";
//}

void setup() {
  //delay(1000);
  Serial.begin(115200);
  while (!Serial);
  // Very important for SPI pin configuration!
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

  // Very important for LoRa Radio pin configuration!
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);

  }

  LoRa.setSpreadingFactor(12);
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  //LoRa.setSignalBandwidth(62.5E3);

  //strcpy(sensorReading.key ,"sp");
  sensorReading.key = 82;
  sensorReading.command = 1;
  sensorReading.id = 2;
  sensorReading.ec = 34500;
  sensorReading.sal= 50;
  sensorReading.dO= 9.08;
  sensorReading.temp= 37;
  sensorReading.pH= 14;
  sensorReading.chl= 100;
  sensorReading.volts = 100;
  sensorReading.level= 5;
  sensorReading.sleepTime= 60;
}

void loop() {
  LoRa.beginPacket();
  LoRa.write((uint8_t*)&sensorReading, sizeof(sensorReading));
  LoRa.endPacket();
  Serial.println("packet sent");
  delay(2000);
}
