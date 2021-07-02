#include <SPI.h>
#include <LoRa.h>       // https://github.com/sandeepmistry/arduino-LoRa

#include "FS.h"
#include <SPI.h>

#define OFF 0   // For LED
#define ON 1

// SPI LoRa Radio
#define LORA_SCK 5        // GPIO5 - SX1276 SCK
#define LORA_MISO 19     // GPIO19 - SX1276 MISO
#define LORA_MOSI 27    // GPIO27 - SX1276 MOSI
#define LORA_CS 18     // GPIO18 - SX1276 CS
#define LORA_RST 14   // GPIO14 - SX1276 RST
#define LORA_IRQ 26  // GPIO26 - SX1276 IRQ (interrupt request)
//String sensorReading = "";
//byte syncWord = 0x14;

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

void setup() {
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

  // The larger the spreading factor the greater the range but slower data rate
  // Send and receive radios need to be set the same
  LoRa.setSpreadingFactor(12);  // ranges from 6-12, default 7 see API docs
  //LoRa.setSyncWord(syncWord);
}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket ();
  if (packetSize) // Only read if there is some data to read..
  {
    LoRa.readBytes((uint8_t *)&sensorReading, packetSize);
    Serial.print("key:");Serial.println(sensorReading.key);
    Serial.print("command:");Serial.println(sensorReading.command);
    Serial.print("id:");Serial.println(sensorReading.id);
    Serial.print("ec:");Serial.println(sensorReading.ec);
    Serial.print("sal:");Serial.println(sensorReading.sal);
    Serial.print("dO:");Serial.println(sensorReading.dO);
    Serial.print("temp:");Serial.println(sensorReading.temp);
   Serial.print("pH:"); Serial.println(sensorReading.pH);
   Serial.print("chl:"); Serial.println(sensorReading.chl);
    Serial.print("volts:");Serial.println(sensorReading.volts);
    Serial.print("level:");Serial.println(sensorReading.level);
    Serial.print("sleepTime:");Serial.println(sensorReading.sleepTime);
  }

}
