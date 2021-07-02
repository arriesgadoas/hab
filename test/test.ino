#include <RTClib.h>
#include <Wire.h>
#include <LoRa.h>
#include <SPI.h>

//declare RTC instance
RTC_DS3231 rtc;

RTC_DATA_ATTR int level = 100;
RTC_DATA_ATTR int sleepMinute;
RTC_DATA_ATTR int sleepSecond;
RTC_DATA_ATTR bool configured = false;
RTC_DATA_ATTR bool normalMode = false;
RTC_DATA_ATTR bool standby = true;
RTC_DATA_ATTR Ds3231Alarm1Mode alarmMode;
RTC_DATA_ATTR bool diagnostics = false;
RTC_DATA_ATTR int relayModeTime = 180000;
RTC_DATA_ATTR bool levelSet = false;
RTC_DATA_ATTR bool sleepTimeSet = false;
RTC_DATA_ATTR bool sendSuccess = false;

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

String sensorReading = "";
String packet = "";
int senderLevel;
int delay1; //delay before transmitting sensor readings
int long startMillis = 0;
int long standByTimer = 0;
String dummy = ",0,0,0,0,0,0,0,0,";

void setup() {

  String sensorReading = "";
  String packet = "";
  int senderLevel;
  int delay1; //delay before transmitting sensor readings
  int long startMillis = 0;
  int long standByTimer = 0;
  String dummy = ",0,0,0,0,0,0,0,0,";
  String diagString;

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 13, 15);
  delay(50);
  //setup LoRa
  setupLoRa();
  //setup rtc
  setupRTC();

  if (diagnostics == false) {
    Serial.println("WAITING FOR DIAGNOSIS");
    pinMode(batOK, OUTPUT);
    pinMode(batER, OUTPUT);
    pinMode(sensorOK, OUTPUT);
    pinMode(sensorER, OUTPUT);
    while (diagnostics == false) {
      diagString = readATMEGA();
      if (getValuebyIndex(diagString, ':', 0) == "D") {
        Serial.println(diagString);
        diagnosis(diagString);
      }
    }
  }

  //packet to receive has the format spdata,[command],[level],[sleep time],...[sensor readings]...
  //CONFIGURED MODE

  if (configured == true) {
    packet = "";
    Serial.println("NORMAL MODE OPERATION");
    sensorReading = readATMEGA() + "," + String(level) + "," + String(sleepMinute);
    Serial.println(sensorReading);
    delay1 = random(2000, 4000);
    delay(delay1);
    Serial.println("SENDING SENSOR READINGS");
    sendPacket(sensorReading);
    startMillis = millis();

    do {
      standby = true;
      Serial.println("WAITING FOR PACKETS");
      while (standby == true & (millis() - startMillis < (relayModeTime - delay1))) {
        packet = receivePacket();
      }
      packet.trim();
      Serial.println(packet);
      if (checkValid(packet) == 1) {
        Serial.println("VALID PACKET");
        if (getCommand(packet) == "R") {
          if (getSenderLevel(packet).toInt() == level + 1) {
            Serial.println("BOUNCE READING");
            sendPacket(packet);
            packet = "";
          }
          else {
            Serial.println("LOW LEVEL NO BOUNCE");
            packet = "";
          }
        }
      }
    } while (millis() - startMillis < (relayModeTime - delay1));

    Serial.println("SEND SLEEP COMMAND FOR UNCONFIGURED SENSPAK");
    sendPacket("spdata,SL");
    alarmMode = DS3231_A1_Minute;
    standByTimer = startMillis - millis();
    rtc.setAlarm1(rtc.now() + TimeSpan(0, 0, sleepMinute, 0), alarmMode);
    sleepESP32();
  }
  //    //send set-up packet for unconfigured senspak units
  //    //sendPacket("S," + String(level) + ","+String(sleepMinute));
  //    startMillis = millis();
  //    standby = true;
  //    do {
  //
  //      if (standby == true) {
  //        Serial.println("WAITING FOR PACKETS");
  //        while (standby == true) {
  //          packet = receivePacket();
  //        }
  //      }
  //      packet.trim();
  //      Serial.println(packet);
  //      if (packet == "") {
  //        Serial.println("empty");
  //      }
  //      if (checkValid(packet) == 1) {
  //        if (getSenderLevel(packet) == "R") {
  //          standby = true;
  //          Serial.println("RECEIVED READINGS");
  //          int senderLevel = getSenderLevel(packet).toInt();
  //          if (senderLevel ==  level + 1) {
  //            sendPacket(packet);
  //          }
  //
  //          else {
  //            standby = true;
  //            Serial.println("LOW LEVEL. IGNORE PACKET");
  //            delay(500);
  //            setup();
  //          }
  //
  //        }
  //
  //        if (getCommand(packet) == "SL") {
  //          packet = "";
  //
  //          sendPacket("spdata,SL");
  //          standby = false;
  //          Serial.println("ENTER SLEEP MODE: " + String(sleepMinute) + " mins.");
  //          alarmMode = DS3231_A1_Minute;
  //          standByTimer = startMillis - millis();
  //          rtc.setAlarm1(rtc.now() + TimeSpan(0, 0, sleepMinute, 0), alarmMode);
  //          sleepESP32();
  //
  //        }
  //
  //      }
  //
  //      else {
  //        standby = true;
  //        packet = "";
  //        Serial.println("INVALID PACKET");
  //        delay(500);
  //        setup();
  //      }
  //    } while (millis() - startMillis < relayModeTime);
  //  }


  //SETUP MODE
  if (configured == false) {
    sensorReading = readATMEGA();
    sensorReading.trim();
    if(sensorReading != ""){
      sensorReading = sensorReading + ","+String(level) + "," + String(sleepMinute);
      Serial.println(sensorReading);
      sendPacket(sensorReading);
      }
      
    if (standby == true) {
      Serial.println("WAITING FOR INSTRUCTIONS");
      while (standby == true) {
        packet = receivePacket();
      }
    }

    packet.trim();
    Serial.println(packet);

    if (checkValid(packet) == 1) {

      if (getCommand(packet) == "C") {
        if (getValuebyIndex(packet, ',', 2) == "1") {
          packet = "";
          standby = true;
          //readATMEGA();
          sleepSecond = 3;
          alarmMode = DS3231_A1_Second;
          Serial.println("TAKING SENSOR READINGS");
          rtc.setAlarm1(rtc.now() + TimeSpan(0, 0, 0, sleepSecond), alarmMode);
          sleepESP32();
        }
      }

      if (getCommand(packet) == "R") {
        sleepMinute = getSleepTime(packet).toInt();
        if (getSenderLevel(packet).toInt() < level) {
          level = getSenderLevel(packet).toInt();
        }
        levelSet = true;
        sleepTimeSet = true;
        standby = true;
        sendPacket("spdata,R" + dummy + String(level) + "," + String(sleepMinute));
        packet = "";
        Serial.println("SLEEPTIME = " + String(sleepMinute));
        Serial.println("LEVEL = " + String(level));
        delay(500);
        setup();
      }
      //
      //      if (getCommand(packet) == "S") {
      //        sleepMinute = getSleepTime(packet).toInt();
      //        if (getSenderLevel(packet).toInt() < level) {
      //          level = getSenderLevel(packet).toInt();
      //        }
      //        levelSet = true;
      //        sleepTimeSet = true;
      //        standby = true;
      //        sendPacket("spdata,S,"+String(level)+","+String(sleepMinute));
      //        packet = "";
      //        Serial.println("SLEEPTIME = " + String(sleepMinute));
      //        Serial.println("LEVEL = " + String(level));
      //        delay(500);
      //        setup();
      //      }
      //
      if (getCommand(packet) == "SL") {
        sendPacket("spdata,SL");
        if (sleepTimeSet == true & levelSet == true) {
          packet = "";
          configured = true;
          alarmMode = DS3231_A1_Minute;
          Serial.println("CONFIGURED AND GOING TO SLEEP");
          rtc.setAlarm1(rtc.now() + TimeSpan(0, 0, sleepMinute, 0), alarmMode);
          sleepESP32();
        }

        else {
          standby = true;
          packet = "";
          Serial.println("CAN'T SLEEP. NEED CONFIGURATION");
          delay(500);
          setup();
        }
      }
    }
    else {
      packet = "";
      standby = true;
      Serial.println("INVALID PACKET");
      delay(500);
      setup();
    }
  }
}



/*---------------------------------------------*/
int checkValid(String s) {
  if (getValuebyIndex(s, ',', 0) == "spdata") {
    return 1;
  }
  else {
    return 0;
  }
}
/*---------------------------------------------*/
String getCommand(String s) {
  return getValuebyIndex(s, ',', 1);
}
/*---------------------------------------------*/
String getSenderLevel(String s) {
  return getValuebyIndex(s, ',', 10);
}

/*---------------------------------------------*/
String getSleepTime(String s) {
  return getValuebyIndex(s, ',', 11);
}

/*---------------------------------------------*/
void sleepESP32() {
  Serial.println("internal sleep check");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1);
  esp_deep_sleep_start();
}
/*---------------------------------------------*/
String readATMEGA() {
  delay(1000);
  String s;
  if (Serial2.available()) {                  //get serial message from ATMEGA328P
    //Serial.println("Get senspak reading...");
    while (Serial2.available() > 0) {
      char serial2Char = (char)Serial2.read();
      s += serial2Char;
    }
    s.trim();
    return s;
    //    if (sensorReading != "") {
    //      //if serial is not empty do...
    //      if (sensorReading.indexOf('D') == 0) {  //...print diagnostics or...
    //        Serial.println(sensorReading);
    //        diagnosis(sensorReading);
    //      }
    //
    //      else {                                  //...send data with initial bounce count of 0
    //        //        delay1 = random(3000);
    //        //        delay(delay1);
    //        sensorReading = sensorReading + "," + String(level) + "," + String(sleepMinute);
    //        Serial.println(sensorReading);
    //        //sendPacket(sensorReading);
    //      }
    //    }
    //    sensorReading = "";
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
    Serial.println("Couldn't find RTC!");
    Serial.flush();
    abort();
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  rtc.disable32K();
  rtc.writeSqwPinMode(DS3231_OFF);
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.disableAlarm(2);
  rtc.disableAlarm(1);
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
  received.trim();
  return received;
}

/*---------------------------------------------*/
void sendPacket(String packet) {
  //delay(2000);
  LoRa.beginPacket();
  LoRa.print(packet + '\r');
  LoRa.endPacket();
  Serial.println("PACKET SENT");
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
  packet = "";
  delay(500);
  setup();
}
