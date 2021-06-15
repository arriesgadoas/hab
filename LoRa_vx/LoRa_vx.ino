#include <RTClib.h>
#include <Wire.h>
#include <LoRa.h>
#include <SPI.h>

RTC_DATA_ATTR int level = 1;
RTC_DATA_ATTR int configured = 0;

//declare RTC instance
RTC_DS3231 rtc;

//diagnostic LEDs
#define batOK 21
#define batER 22
#define sensorOK 23
#define sensorER 4

// SPI LoRa Radio
#define LORA_SCK 5        // GPIO5 - SX1276 SCK
#define LORA_MISO 19     // GPIO19 - SX1276 MISO
#define LORA_MOSI 27    // GPIO27 -  SX1276 MOSI
#define LORA_CS 18     // GPIO18 -   SX1276 CS
#define LORA_RST 14   // GPIO14 -    SX1276 RST
#define LORA_IRQ 26  // GPIO26 -     SX1276 IRQ (interrupt request)

String message = "";
String sensorReading = "";
String key = "";
String payload = "";
int senderLevel;
int messageDirection;



void setup() {
  Serial.println("I'm awake.");
  pinMode(batOK, OUTPUT);
  pinMode(batER, OUTPUT);
  pinMode(sensorOK, OUTPUT);
  pinMode(sensorER, OUTPUT);

//  EEPROM.begin(2);
//  int configured = EEPROM.read(1);
//  int level = EEPROM.read(0);
  
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 13, 15);
  
  //setup rtc
  Serial.println("I'm awake.");
  //setup LoRa
  setupLoRa();

  delay(1000);
  if (Serial2.available()) {                  //get serial message from ATMEGA328P
    while (Serial2.available() > 0) {
      char serial2Char = (char)Serial2.read();
      sensorReading += serial2Char;
    }
    sensorReading.trim();
    Serial.println(sensorReading);
    if (sensorReading != "") {
      //if serial is not empty do...
      if (sensorReading.indexOf('D') == 0) {  //...print diagnostics or...

        diagnosis(sensorReading);
      }

      else {                                  //...send data with initial bounce count of 0
        // sendPacket(sensorReading);  
      }
    }
    sensorReading = "";
  }

  // while(configured == 0) {
  //     message = receivePacket();
  //     if (message.length() > 0){
  //       key = getValuebyIndex(message, ',', 0);
  //       if(key == "SP"){
  //          messageDirection = getValuebyIndex(message, ',', 1);
  //          senderLevel = getValuebyIndex(message, ',', 2);
  //          if(messageDirection == 1){
  //            level = senderLevel + 1;
  //          }
  //          else{
  //            if(senderLevel > 0){
               
  //            }
  //          }          
  //         }
  //       }
  // }
  
  Serial.println("Going to sleep");
  setupRTC();  
  rtc.setAlarm1(rtc.now() + TimeSpan(0, 0, 1, 0), DS3231_A1_Minute);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1);
  esp_deep_sleep_start();
}

/*---------------------------------------------*/
void diagnosis(String string) {
  String bat = getValuebyIndex(string, ':', 1);
  String sensor = getValuebyIndex(string, ':', 2);

  if (String(bat) == "ER") {
    digitalWrite(batER, HIGH);
  }
  if (String(bat) == "OK") {
    digitalWrite(batOK, HIGH);
  }
  if (String(sensor) == "ER") {
    digitalWrite(sensorER, HIGH);
  }
  if (String(sensor) == "OK") {
    digitalWrite(sensorOK, HIGH);
  }

  delay(3000);
  digitalWrite(batER, LOW);
  digitalWrite(batOK, LOW);
  digitalWrite(sensorER, LOW);
  digitalWrite(sensorOK, LOW);
}

// /*---------------------------------------------*/
// int setSleepTime(int slTime, int newSlTime) {
//   if (slTime != newSlTime) {
//     EEPROM.write(1, newSlTime);
//     EEPROM.commit();
//   }
//   return EEPROM.read(1);
// }

// /*---------------------------------------------*/
// int setupLevel(int lvl, int senderlvl) {
//   if (lvl == 0) {
//     lvl =  senderlvl + 1;
//     EEPROM.write(0, lvl);
//     //EEPROM.write(1, slTime);
//     EEPROM.commit();
//     //sendPacket("S," + String(lvl) + "," + String(slTime));
//   }

//   else {
//     if (senderlvl < lvl) {
//       lvl = senderlvl + 1;
//       EEPROM.write(0, lvl);
//       // EEPROM.write(1, slTime);
//       EEPROM.commit();
//       // sendPacket("S," + String(lvl) + "," + String(slTime));
//     }
//   }
//   return EEPROM.read(0);
// }

/*---------------------------------------------*/
void setupRTC() {
  Wire.begin(0, 25);
  if (!rtc.begin()) {
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  rtc.disable32K();
  rtc.writeSqwPinMode(DS3231_OFF);
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.disableAlarm(2);
}

/*---------------------------------------------*/
void setupLoRa() {
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

/*---------------------------------------------*/
String receivePacket() {
  String received;
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    while (LoRa.available()) {
      received = LoRa.readString();
    }
  }
  return received;
}

/*---------------------------------------------*/
void sendPacket(String packet) {
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();
}

/*---------------------------------------------*/
String getValuebyIndex(String s, char delimiter, int index) {
  int found = 0;
  int strIndex[] = {0, -1};

  int maxIndex = s.length();

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (s.charAt(i) == delimiter || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? s.substring(strIndex[0], strIndex[1]) : "";
}

/*---------------------------------------------*/
void loop() {
  // put your main code here, to run repeatedly:

}
