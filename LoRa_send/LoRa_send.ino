
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
#include <U8g2lib.h>   // https://github.com/olikraus/U8g2_Arduino 
//#include <U8x8lib.h>

#define OFF 0   // For LED
#define ON 1

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
unsigned long startMillis;
unsigned long currentMillis;
unsigned long period;

void setup() {
  //delay(1000);
  
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
  //LoRa.setSignalBandwidth(62.5E3);

  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  startMillis = millis();  //initial start time
  period = 3000;
  
  do {
    currentMillis = millis();
    while (Serial2.available() > 0) {
      char serial2Char = (char)Serial2.read();
      sensorReading += serial2Char;
    }
    Serial.println(sensorReading);
    if (sensorReading.indexOf('D') == 0) {
      Display.begin();
      Display.enableUTF8Print();    // enable UTF8 support for the Arduino print() function
      Display.setFont(u8g2_font_6x13_tf);
      //Display error code here
      Display.clearBuffer();
      Display.setCursor(0, 12);
      Display.print("DIAGNOSTIC:");
      Display.setCursor(0, 30);
      Display.print(sensorReading);
      Display.sendBuffer();
    }  

    else {
      LoRa.beginPacket();
      LoRa.print(sensorReading);
      LoRa.endPacket();
    }
    sensorReading = "";

  } while (currentMillis - startMillis <= period);
  Display.clearDisplay();
  
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 1);
  //gpio_pulldown_en(GPIO_NUM_13);
  esp_deep_sleep_start();
}

void loop() {


}





// Display Info
//Display.clearBuffer();
//Display.clearDisplay();
//Display.setCursor(0, 12);
//Display.print("Sent data:");
//Display.setCursor(0, 30);
// Display.print("(" + String(dataCount) + ")");
//Display.setCursor(0, 48);
//Display.print(sensorReading);
//Display.sendBuffer();
