#include <RTClib.h>
#include <Wire.h>
#include <LoRa.h>
#include <SPI.h>

//declare RTC instance
RTC_DS3231 rtc;

RTC_DATA_ATTR int level = 1;
RTC_DATA_ATTR int sleepMinute;
RTC_DATA_ATTR int sleepSecond;
RTC_DATA_ATTR bool configured = false;
RTC_DATA_ATTR bool normalMode = false;
RTC_DATA_ATTR bool readMode = false;
RTC_DATA_ATTR bool standby = true;
RTC_DATA_ATTR Ds3231Alarm1Mode alarmMode;
RTC_DATA_ATTR bool diagnostics = false;

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
String packet = "";
int senderLevel;
int messageDirection;



void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 13, 15);

  //setup LoRa
  setupLoRa();


  //setup rtc
  
  
  delay(1000);
  if (diagnostics == false) {
    pinMode(batOK, OUTPUT);
    pinMode(batER, OUTPUT);
    pinMode(sensorOK, OUTPUT);
    pinMode(sensorER, OUTPUT);
    while (diagnostics == false) {
      readATMEGA();
    }
  }

  if (normalMode == true){
    readATMEGA();
    }

//  while (packet == "" & readMode == false) {
//    packet = receivePacket();
//  }

  while (standby == true) {
    packet = receivePacket();
  }
  packet.trim();
  Serial.println(packet);

  //check if received packet is from senspak
  if (getValuebyIndex(packet, ',', 0) == "spdata") {
    Serial.println("VALID PACKET");
    //If continuous reading commands are received (on/off)
    if (getValuebyIndex(packet, ',', 1) == "C") {
      //if senspak unit is not yet configured do the following
      if (configured == false) {
        //continuous reading ON
        if (getValuebyIndex(packet, ',', 2) == "1") {
          standby = true;
          sleepSecond = 1;
          alarmMode = DS3231_A1_Second;
          Serial.println("TAKING SENSOR READINGS");
          setupRTC();
          rtc.setAlarm1(rtc.now() + TimeSpan(0, 0, 0, sleepSecond), alarmMode);
          sleep();
        }
        //continuous reading OFF
        else {
          standby = true;
          Serial.println("WAIT FOR OTHER SETUP/SYNC COMMAND");
          delay(500);
          setup();
        }
      }
      else {
        //do nothing if senspak is already configured
      }
    }
    packet = "";
  }
  else {
    //if packet is invalid redo setup function
    packet = "";
    standby = true;
    Serial.println("RECURSION INVALID PACKET");
    delay(500);
    setup();
  }

}


/*---------------------------------------------*/
void sleep() {
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1);
  esp_deep_sleep_start();
}
/*---------------------------------------------*/
void readATMEGA() {
  if (Serial2.available()) {                  //get serial message from ATMEGA328P
    //Serial.println("Get senspak reading...");
    while (Serial2.available() > 0) {
      char serial2Char = (char)Serial2.read();
      sensorReading += serial2Char;
    }
    sensorReading.trim();
    //Serial.println(sensorReading);
    if (sensorReading != "") {
      //if serial is not empty do...
      if (sensorReading.indexOf('D') == 0) {  //...print diagnostics or...

        diagnosis(sensorReading);
      }

      else {                                  //...send data with initial bounce count of 0
        sendPacket(sensorReading);
      }
    }
    sensorReading = "";
  }
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

  diagnostics = true;
}

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
    standby = false;
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
  Serial.println("xxx");
  delay(1000);
}
