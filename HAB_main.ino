//include libraries
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//setup temperature sensor
#define ONE_WIRE_BUS 6                  //temperature sensor signal output to D6 of ATMEGA
OneWire oneWire_temp(ONE_WIRE_BUS);     // Setup a oneWire instance to communicate with the temperature sensor
DallasTemperature tempSensor(&oneWire_temp);    // Pass oneWire_temp reference to DallasTemperature library


//declare software serial ports for ec and do sensors
SoftwareSerial ecSerial(2, 3); //ec sensor TX --> D2 of atmega
SoftwareSerial doSerial(4, 5);  // do sensor TX --> D4 of atmega

//diagnostic String
String diagnostics = "";
int checkStatus;

//SensPak ID
String ID = "3";
//sensor values in string
String dataToLora = "";
String ec = "";
String dO = "";
String temp = "";
String ph = "";
String wp = "";
String chl = "";

//analogRead sensor variables
int phAnalog;
int wpAnalog;
int chlAnalog;

//5V pins for sensors
int tempPow = 7;
int phPow = 8;
int wpPow = 10;
int chlPow = 12;
int ecDoPow = 13;

//sensor voltage values
float phVoltage;
float wpVoltage;
float chlVoltage;

//calibration constants,use separate sketch for calibration and then replace these values
const float phSlope = 3.743;
const float phIntercept = 0.7118;
const float  wpOffSet = 0.2 ;



void setup() {
  Serial.begin(9600);
  ecSerial.begin(9600);
  doSerial.begin(9600);

  pinMode(ecDoPow, OUTPUT);
  pinMode(tempPow, OUTPUT);
  pinMode(phPow, OUTPUT);
  //  pinMode(turbPow, OUTPUT);
  pinMode(wpPow, OUTPUT);
  //  pinMode(tdsPow, OUTPUT);
  pinMode(chlPow, OUTPUT);

  unsigned long startMillis;
  unsigned long currentMillis;
  unsigned long sleepTime;
  startMillis = millis();  //initial start time
  sleepTime = 20000;

  do {
    currentMillis = millis();
    dataToLora = diagnostic();
    Serial.println(dataToLora);
    //startMillis = currentMillis;
  } while (currentMillis - startMillis <= sleepTime);



}

//function to get battery level
float getBatLevel() {
  float actualVoltage;
  actualVoltage = analogRead(A5) * 5.0 / 1023.00;
  return actualVoltage;
}

//function to check if probe head is connected
int checkProbe() {
  /*--------------temp sensor-----------*/
  //get temperature reading
  digitalWrite(tempPow, HIGH);  //turn on temp sensor
  delay(1000);
  tempSensor.begin();
  tempSensor.requestTemperatures();  //get celsius temp
  temp = tempSensor.getTempCByIndex(0); //actual temp reading in celsius
  if (temp.toFloat() <= -127.0) {
    digitalWrite(tempPow, LOW);
    return 0;
  }
  else {
    digitalWrite(tempPow, LOW);
    return 1;
  }

}

//function to return the final data reading in string format, simple concatenation of all sensor readings
String dataString(String sensor1, String sensor2, String sensor3, String sensor4, String sensor5, String sensor6) {
  String reading;
  reading = sensor1 + "," + sensor2 + "," + sensor3 + "," + sensor4 + "," + sensor5 + "," + sensor6;
  return reading;
}

String readData() {
  unsigned long startMillis;
  unsigned long currentMillis;
  String stringData;
  unsigned long ec_samplingTime = 30000;
  unsigned long do_samplingTime = 10000;
  unsigned long ph_samplingTime = 30000;
  unsigned long chl_samplingTime = 2000;
  float voltagePercent;
  voltagePercent = getBatLevel() * 100 / 3.7;


  /*--------------temp sensor-----------*/
  //get temperature reading

  digitalWrite(tempPow, HIGH);  //turn on temp sensor
  tempSensor.begin();
  delay(1000);
  tempSensor.requestTemperatures();  //get celsius temp
  temp = tempSensor.getTempCByIndex(0); //actual temp reading in celsius
  digitalWrite(tempPow, LOW);

  /*--------------EC sensor-----------*/
  //get ec sensor reading
  digitalWrite(ecDoPow, HIGH);
  ecSerial.print("T,"+temp);
  ecSerial.print("\r");
  startMillis = millis();  //initial start time

  do {
    currentMillis = millis();
    ec = "";
    ecSerial.listen();    //listen to ecSerial port
    delay(1000);          //wait!!
    while (ecSerial.available() > 0) {
      char serialChar = (char)ecSerial.read();
      ec += serialChar;
    }
  } while (currentMillis - startMillis <= ec_samplingTime);
  
  /*--------------DO sensor-----------*/
  //get do sensor reading
  doSerial.print("T," + temp);
  doSerial.print("\r");
  doSerial.print("S," + ec);
  doSerial.print("\r");
  startMillis = millis();  //initial start time
  do {
    currentMillis = millis();
    dO = "";
    Serial.println("test");
    doSerial.listen();  //listen to doSerial port
    delay(999);         //wait!! for some reason hindi pwedeng pareho sila ng delay ng unang software serial, thus 999
    while (doSerial.available() > 0) {
      char serialChar = (char)doSerial.read();
      dO += serialChar;
    }
  } while (currentMillis - startMillis <= do_samplingTime);
  digitalWrite(ecDoPow, LOW);

  /*--------------pH sensor-----------*/
  //get pH sensor reading
  digitalWrite(phPow, HIGH); //turn on pH sensor
  startMillis = millis();
  do {
    currentMillis = millis();
    phVoltage = analogRead(A0) * 5.00 / 1023.00;
  } while (currentMillis - startMillis <= ph_samplingTime);
  ph = String((phSlope * phVoltage) + phIntercept); //voltage to actual pH value
  digitalWrite(phPow, LOW);   //turn off pH sensor

  /*--------------wp sensor-----------*/
  //get wp sensor reading
  digitalWrite(wpPow, HIGH);  //turn on wp sensor
  delay(1000);
  wpVoltage = analogRead(A2) * 5.00 / 1023.00;
  wp = String((wpVoltage - wpOffSet) * 400);  //calibration formula of w.p. sensor from manufacturer's spec sheet
  digitalWrite(wpPow, LOW);  //turn off turb sensor

  /*-------------chl sensor---------*/
  //get chl sensor reading
  digitalWrite(chlPow, HIGH);
  startMillis = millis();
  do {
    currentMillis = millis();
    chlVoltage = analogRead(A4) * 5.00 / 1023.00;
  } while (currentMillis - startMillis <= chl_samplingTime);

  chl = String(chlVoltage);
  digitalWrite(chlPow, LOW);

  return stringData = ID + ","  + dataString(ec, dO, temp, ph, wp, chl) + "," + String(voltagePercent);
}

//function to perform system diagnostic
String diagnostic() {
  String stringData;
  int probeStatus;
  float batVoltage;
  batVoltage = getBatLevel();
  if (batVoltage <= 3.0) {
    checkStatus =  0;
    stringData = "D:ER";
  }
  else {
    stringData = "D:OK";
    checkStatus = 1;
  }

  probeStatus = checkProbe();
  if (probeStatus == 1) {
    stringData = stringData + ":OK";
  }
  else {
    stringData = stringData + ":ER";
  }
  return stringData;
}

ISR(WDT_vect) {
}

void loop() {
  //sleep mode declarations
  WDTCSR = (24);
  WDTCSR = (33); //32 -- 4 sec, 33 -- 8 sec
  WDTCSR |= (1 << 6);
  SMCR |= (1 << 2);
  SMCR |= 1;

  byte old_ADCSRA;
  old_ADCSRA = ADCSRA;
  ADCSRA = 0;

  delay(100);
  Serial.end();
  // pinMode(1, OUTPUT);
  // digitalWrite(1, LOW);
  for (byte i = 0; i <= 13; i++)
  {
    pinMode (i, OUTPUT);    // changed as per below
    digitalWrite (i, LOW);
  }

  for (int i = 0; i < 2  ; i++) { //adjust this to extend sleep time
    MCUCR |= (3 << 5);
    MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6);
    __asm__ __volatile("sleep"::);
  }

  ADCSRA = old_ADCSRA;

  SoftwareSerial ecSerial(2, 3); //ec sensor TX --> D2 of atmega
  SoftwareSerial doSerial(4, 5);  // do sensor TX --> D4 of atmega
  ecSerial.begin(9600);
  doSerial.begin(9600);
  dataToLora = readData();
  Serial.begin(9600);
  for (int i = 0; i < 6; i++) {
    Serial.println(dataToLora);
    delay(500);
  }
  ec = "";
  dO = "";

}