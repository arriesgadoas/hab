#include <SPI.h>
#include <LoRa.h>       // https://github.com/sandeepmistry/arduino-LoRa
#include <U8g2lib.h>   // https://github.com/olikraus/U8g2_Arduino

// SPI LoRa Radio
#define LORA_SCK 5        // GPIO5 - SX1276 SCK
#define LORA_MISO 19     // GPIO19 - SX1276 MISO


#define LORA_MOSI 27    // GPIO27 -  SX1276 MOSI
#define LORA_CS 18     // GPIO18 -   SX1276 CS
#define LORA_RST 14   // GPIO14 -    SX1276 RST
#define LORA_IRQ 26  // GPIO26 -     SX1276 IRQ (interrupt request)

// I2C OLED Display works with SSD1306 driver
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST 16

U8G2_SSD1306_128X64_NONAME_F_SW_I2C Display(U8G2_R0, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA, /* reset=*/ OLED_RST); // Full framebuffer, SW I2C

String sensorReading = "";
String receivedData = "";

int bounce = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 13, 15);
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
}
void diagnosis() {

  Display.begin();
  Display.enableUTF8Print();    // enable UTF8 support for the Arduino print() function
  Display.setFont(u8g2_font_6x13_tf);
  Display.clearBuffer();
  Display.setCursor(0, 12);
  Display.print("DIAGNOSTIC:");
  Display.setCursor(0, 30);
  Display.print(sensorReading);
  Display.sendBuffer();
  delay(500);
}

void sendPacket(String packet) {
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();
}

String receivePacket() {
  String received;
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // read packet
    while (LoRa.available()) {
      received = LoRa.readString(); // Assemble new packet
    }
  }
  return received;
}
int getBounce(String string) {
  String dataString = string;

  char buff[dataString.length() + 1];
  dataString.toCharArray(buff, dataString.length() + 1);
  char* a =  strtok(buff, ",");
  char* b =  strtok(NULL, ",");
  char* c =  strtok(NULL, ",");
  char* d =  strtok(NULL, ",");
  char* e =  strtok(NULL, ",");
  char* f =  strtok(NULL, ",");
  char* g =  strtok(NULL, ",");
  char* h =  strtok(NULL, ",");
  char* i = strtok(NULL, "\r");
  String x = i;
  return x.toInt();
}

String insertNewBounce(String string, int bounceCtr) {
  String dataString = string;
  char buff[dataString.length() + 1];
  dataString.toCharArray(buff, dataString.length() + 1);
  char* a =  strtok(buff, ",");
  String checker = a;
  char* b =  strtok(NULL, ",");
  String ID = b;
  char* c =  strtok(NULL, ",");
  String ec = c;
  char* d =  strtok(NULL, ",");
  String dO = d;
  char* e =  strtok(NULL, ",");
  String temp = e;
  char* f =  strtok(NULL, ",");
  String ph = f;
  char* g =  strtok(NULL, ",");
  String chl = g;
  char* h =  strtok(NULL, ",");
  String volt = h;
  char* i = strtok(NULL, "\r");
  String bounce = i;
 
  String parsed =  checker + "," + ID + "," + ec + "," + dO+ "," + temp + "," + ph + "," + chl + "," + volt + "," + String(bounceCtr);
  return parsed;
}
/*int getBounce(String s){
  String dataString = s;
  char buff[dataString.length() + 1];
  dataString.toCharArray(buff, dataString.length() + 1);
  char* b =  strtok(buff, ",");
  String ctr = b;
  return ctr.toInt();
  }*/

void loop() {

  if (Serial2.available()) {                  //get serial message from ATMEGA328P
    while (Serial2.available() > 0) {
      char serial2Char = (char)Serial2.read();
      sensorReading += serial2Char;
    }
    if (sensorReading != "") {                //if serial is not empty do...
      if (sensorReading.indexOf('D') == 0) {  //...print diagnostics or...
        Display.setPowerSave(0);
        diagnosis();
        Display.setPowerSave(1);
      }
      else {                                  //...send data with initial bounce count of 0
        sensorReading.trim();
        sensorReading = sensorReading + "0";
        Serial.println(sensorReading);
        sendPacket(sensorReading);
      }
    }
    sensorReading = "";
  }
  receivedData = receivePacket();
  if (receivedData.length() > 1) {          //if packet is received
    Serial.println(receivedData);
    
    bounce = getBounce(receivedData);       //get number of bounces--
    
    if (bounce < 5) {                       //if bounce count is less than 5 then send packet
      bounce++;
      receivedData =  insertNewBounce(receivedData, bounce);/*receivedData+","+String(bounce);*/
      sendPacket(receivedData);
    }
  }

}
