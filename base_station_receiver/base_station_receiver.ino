
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


typedef struct r {
  byte key;
  byte command;
  byte id;
  float ec;
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

String ec;
String sal;
String dO;
String temp;
String pH;
String chl;
String volts;
String id;

void commandPacket(byte c, byte l, byte s) {
  sensorReading.key = 83;
  sensorReading.command = c;
  sensorReading.id = 0;
  sensorReading.ec = 0.00;
  sensorReading.sal = 0;
  sensorReading.dO = 0.00;
  sensorReading.temp = 0.00;
  sensorReading.pH = 0.00;
  sensorReading.chl = 0.00;
  sensorReading.volts = 0.00;
  sensorReading.level = l;
  sensorReading.sleepTime = s;
}


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

  commandPacket(0, 0, 59);
  LoRa.beginPacket();
  LoRa.write((uint8_t*)&sensorReading, sizeof(sensorReading));
  LoRa.endPacket();
  delay(10000);
  commandPacket(1, 0, 59);
  LoRa.beginPacket();
  LoRa.write((uint8_t*)&sensorReading, sizeof(sensorReading));
  LoRa.endPacket();
  
}

void loop() {

  int packetSize = LoRa.parsePacket ();
  if (packetSize) // Only read if there is some data to read..
  {
    LoRa.readBytes((uint8_t *)&sensorReading, packetSize);
    if (sensorReading.key = 83) {
      id = String(sensorReading.id);
      ec = String(sensorReading.ec);
      sal = String(sensorReading.sal);
      dO = String(sensorReading.dO);
      temp = String(sensorReading.temp);
      pH = String(sensorReading.pH);
      chl = String(sensorReading.chl);
      volts = String(sensorReading.volts);
      Serial.println("spdata," + id + ',' + ec + ',' + sal + ',' + dO + ',' + temp + ',' + pH + ',' + chl + ',' + volts);
      //sendToRpi = "";
      ec = "";
      sal = "";
      dO = "";
      temp = "";
      pH = "";
      chl = "";
      volts = "";
    }
  }
}
