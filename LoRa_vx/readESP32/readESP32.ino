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


struct r packet;
boolean standby;

void setup() {
  // put your setup code here, to run once:
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
  sendReadCommand();
}

void sendReadCommand() {
  packet.key = 83;
  packet.command = 2;
  packet.id = 2;
  packet.ec = 34500;
  packet.sal = 50;
  packet.dO = 9.08;
  packet.temp = 37;
  packet.pH = 14;
  packet.chl = 100;
  packet.volts = 100;
  packet.level = 1;
  packet.sleepTime = 1;
  Serial.println("");
  LoRa.beginPacket();
  LoRa.write((uint8_t*)&packet, sizeof(packet));
  LoRa.endPacket();
}

void receivePacket() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    LoRa.readBytes((uint8_t *)&packet, packetSize);
    if (packet.key == 83) {
      standby = false;
    }

    else {
      Serial.println("invalid packet received");
      standby = true;
    }
  }
}

void loop() {
  standby  = true;
  if (standby == true) {
    while (standby == true) {
      receivePacket();
    }
  }
  Serial.print(packet.id);
  Serial.print(",");
  Serial.print(packet.ec);
  Serial.print(",");
  Serial.print(packet.sal);
  Serial.print(",");
  Serial.print(packet.dO);
  Serial.print(",");
  Serial.print(packet.temp);
  Serial.print(",");
  Serial.print(packet.pH);
  Serial.print(",");
  Serial.print(packet.chl);
  Serial.print(",");
  Serial.println(packet.volts);
  sendReadCommand();
}
