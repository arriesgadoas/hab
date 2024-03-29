#include <RTClib.h>
#include <Wire.h>
#include <LoRa.h>
#include <SPI.h>

//declare RTC instance
RTC_DS3231 rtc;

//task handlers
static TaskHandle_t rT = NULL;
static TaskHandle_t sT = NULL;

//specify cpu core where receive packet task and send packet task will run
static const BaseType_t core = 1;

RTC_DATA_ATTR int level = 100;
RTC_DATA_ATTR int sleepMinute;
RTC_DATA_ATTR int sleepSecond;
RTC_DATA_ATTR bool configured = false;
RTC_DATA_ATTR bool normalMode = false;
RTC_DATA_ATTR bool standby = true;
RTC_DATA_ATTR Ds3231Alarm1Mode alarmMode;
RTC_DATA_ATTR bool diagnostics = false;
RTC_DATA_ATTR int relayModeTime = 60000;
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
//String packet = "";
int senderLevel;
int delay1; //delay before transmitting sensor readings
int delay2; //delay before transmitting bounced packet
int long wakeTimerStart = 0;
int long wakeTimerEnd = 0;
int long standByTimer = 0;
String dummy = ",0,0,0,0,0,0,0,0,";

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
struct r myPacket;
String diagString;
void setup() {

  //  String sensorReading = "";
  //  //String //packet = "";
  //  int senderLevel;
  //  int delay1; //delay before transmitting sensor readings
  //  int long startMillis = 0;
  //  int long standByTimer = 0;
  //  String dummy = ",0,0,0,0,0,0,0,0,";
 
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
    Serial.println("Normal mode");
    sensorReading = readATMEGA();
    sensorReading.trim();
    if (sensorReading != "") {
      sensorReading = sensorReading + "," + String(level) + "," + String(sleepMinute);
      stringToStruct(sensorReading);
      myPacket = packet;
    }

  xTaskCreatePinnedToCore(rTask, "receive packet", 1024, NULL, 1, &rT, core);
  xTaskCreatePinnedToCore(sTask, "send packet", 1024, NULL, 1, &sT, core);

  unsigned long s = millis();
  do{}while(millis() - s < relayModeTime);
  
  delay(2000);
  sleepPacket(); //assemble sleep command packet
  sendPacket(packet);
  Serial.println("going to sleep...");
  alarmMode = DS3231_A1_Minute;
  rtc.setAlarm1(rtc.now() + TimeSpan(0, 0, sleepMinute, 0), alarmMode);
  sleepESP32();
}


//SETUP MODE
if (configured == false) {
  sensorReading = readATMEGA();
  sensorReading.trim();
  if (sensorReading != "") {
    sensorReading = sensorReading + "," + String(level) + "," + String(sleepMinute);
    stringToStruct(sensorReading);
    Serial.println(sensorReading);
    delay1 = random(5000);
    delay(delay1);
    sendPacket(packet);
  }

  if (standby == true) {
    Serial.println("WAITING FOR INSTRUCTIONS"); 
    while (standby == true) {
      receivePacket();
    }
  }

  //updateStruct(packet);
  if (packet.command == 2) { // continuous read mode
    //packet = "";
    standby = true;
    //readATMEGA();
    sleepSecond = 3;
    alarmMode = DS3231_A1_Second;
    Serial.println("TAKING SENSOR READINGS");
    rtc.setAlarm1(rtc.now() + TimeSpan(0, 0, 0, sleepSecond), alarmMode);
    sleepESP32();
  }

  if (packet.command == 0) { //set level and sleep time
    standby = true;
    sleepMinute = packet.sleepTime;
    if (packet.level < level) {
      level = packet.level+1;
    }
    levelSet = true;
    sleepTimeSet = true;
    standby = true;
    //sendPacket(packet);
    Serial.println("SLEEPTIME = " + String(sleepMinute));
    Serial.println("LEVEL = " + String(level));
    delay(500);
    setup();
  }

  if (packet.command == 1) { //go to sleep command
    if (sleepTimeSet == true & levelSet == true) {
      configured = true;
      alarmMode = DS3231_A1_Minute;
      Serial.println("CONFIGURED AND GOING TO SLEEP");
      rtc.setAlarm1(rtc.now() + TimeSpan(0, 0, sleepMinute, 0), alarmMode);
      sleepESP32();
    }

    else {
      Serial.println("not configured --> can't sleep");
      standby = true;
      delay(500);
      setup();
    }

  }

}
}

void sTask(void *param) {
  int rD;
  //wait for random length of time between zero and the declared relay mode time to send node information (id and level)
  rD = random(relayModeTime);
  vTaskDelay(rD / portTICK_PERIOD_MS);
  //create an id packet that contains the node information (id and level)
  sendPacket(myPacket);
  vTaskDelete(NULL);
}

/*---------------------------------------------*/
void rTask(void *param) {
  standByTimer = millis() ;
    do { 
      
      standby = true;
      Serial.println("waiting for packets...");
      while (standby == true && (millis() - standByTimer) < (relayModeTime)) {
        receivePacket();
      }
      if (packet.command == 0) {
        if (level > packet.level) {
          level = packet.level + 1;
          Serial.println("low level packet --> do not bounce...");
        }
        if (level < packet.level) {
//          delay2 = random(5000);
//          vTaskDelay(delay2 / portTICK_PERIOD_MS);
          Serial.println("high level packet --> bounce...");
          sendPacket(packet);
        }
      }

      
  } while (millis() - standByTimer < relayModeTime);
  vTaskDelete(NULL);
}
/*---------------------------------------------*/
void stringToStruct(String s) {
  packet.key = getValuebyIndex(s, ',', 0).toInt();
  packet.command = getValuebyIndex(s, ',', 1).toInt();
  packet.id = getValuebyIndex(s, ',', 2).toInt();
  packet.ec = getValuebyIndex(s, ',', 3).toFloat();
  packet.sal = getValuebyIndex(s, ',', 4).toInt();
  packet.dO = getValuebyIndex(s, ',', 5).toFloat();
  packet.temp = getValuebyIndex(s, ',', 6).toFloat();
  packet.pH = getValuebyIndex(s, ',', 7).toFloat();
  packet.chl = getValuebyIndex(s, ',', 8).toFloat();
  packet.volts = getValuebyIndex(s, ',', 9).toFloat();
  packet.level = getValuebyIndex(s, ',', 10).toInt();
  packet.sleepTime = getValuebyIndex(s, ',', 11).toInt();
}

/*---------------------------------------------*/
void sleepPacket() {
  packet.key = 83;
  packet.command = 1;
  packet.id = 0;
  packet.ec = 0;
  packet.sal = 0;
  packet.dO = 0;
  packet.temp = 0;
  packet.pH = 0;
  packet.chl = 0;
  packet.volts = 0;
  packet.level = 0;
  packet.sleepTime = sleepMinute;

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
    //        ////sendPacket(sensorReading);
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
//String receivePacketString() {
//  String received;
//  int packetSize = LoRa.parsePacket();
//  if (packetSize) {
//    while (LoRa.available()) {
//      received = LoRa.readString();
//    }
//    standby = false;
//  }
//  received.trim();
//  return received;
//}
/*---------------------------------------------*/
String receivePacket() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    LoRa.readBytes((uint8_t *)&packet, packetSize);
    if (packet.key == 83) {
      Serial.println("packet struct received");
      standby = false;
    }

    else {
      Serial.println("invalid packet received");
      standby = true;
    }
  }
}


/*---------------------------------------------*/
//void sendPacket(String packet) {
//  //delay(2000);
//  LoRa.beginPacket();
//  LoRa.print(packet + '\r');
//  LoRa.endPacket();
//  Serial.println("PACKET SENT");
//}

/*---------------------------------------------*/
void sendPacket(struct r p) {
  //delay(2000);
  LoRa.beginPacket();
  LoRa.write((uint8_t*)&p, sizeof(p));
  LoRa.endPacket();
  Serial.println("PACKET SENT");
}
/*---------------------------------------------*/




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
  //packet = "";
  delay(500);
  setup();
}
