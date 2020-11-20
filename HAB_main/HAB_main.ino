//include libraries
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include "Filter.h"

#define samples 100

float arr[samples + 1];
long FilterWeight = 50;
ExponentialFilter<long> ADCFilter(FilterWeight, 0);

int address1 = 0;                         //ID address
int address2 = address1 + sizeof(float);    //sleep time address
int address3 = address2 + sizeof(float);  //ph_slope address
int address4 = address3 + sizeof(float);  //ph_intercept address
int address5 = address4 + sizeof(float);  //chl_slope address
int address6 = address5 + sizeof(float);  //chl_intercept address

//setup temperature sensor
#define ONE_WIRE_BUS 6                  //temperature sensor signal output to D6 of ATMEGA
OneWire oneWire_temp(ONE_WIRE_BUS);     // Setup a oneWire instance to communicate with the temperature sensor
DallasTemperature tempSensor(&oneWire_temp);    // Pass oneWire_temp reference to DallasTemperature library

//5V pins for sensors
#define tempPow 7
#define phPow 8
#define wpPow 10
#define doPow 11
#define chlPow 12
#define ecPow 13

//declare software serial ports for ec and do sensors
/*SoftwareSerial ecSerial(2, 3); //ec sensor TX --> D2 of atmega
SoftwareSerial doSerial(4, 5);  // do sensor TX --> D4 of atmega*/


//diagnostic String
String diagnostics = "";
int checkStatus;


//restrieve constants from EEPROM
int ID;
float sleep_length;
float phSlope;
float phIntercept;
float chlSlope;
float chlIntercept;

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

//sensor voltage values
float phVoltage;
float wpVoltage;
float chlVoltage;

void setup() {
  Serial.begin(9600);
  
 // ecSerial.begin(9600);
 // doSerial.begin(9600);

  pinMode(ecPow, OUTPUT);
  pinMode(tempPow, OUTPUT);
  pinMode(phPow, OUTPUT);
  pinMode(doPow, OUTPUT);
  pinMode(wpPow, OUTPUT);
  pinMode(chlPow, OUTPUT);
  unsigned long startMillis;
  unsigned long currentMillis;
  unsigned long diagnosticTime;
  startMillis = millis();  //initial start time
  diagnosticTime = 5000;

  do {
    currentMillis = millis();
    dataToLora = diagnostic();
    Serial.println(dataToLora);
    //startMillis = currentMillis;
  } while (currentMillis - startMillis <= diagnosticTime);

  EEPROM.get(address1, ID);
  EEPROM.get(address2, sleep_length);
  EEPROM.get(address3, phSlope);
  EEPROM.get(address4, phIntercept);
  EEPROM.get(address5, chlSlope);
  EEPROM.get(address6, chlIntercept);
}

//function to get battery level
float getBatLevel() {
  float actualVoltage;/*
  actualVoltage = readVcc(A5) //analogRead(A5) * 5.0 / 1023.00;
  return actualVoltage;*/
  createArray(A5);
  actualVoltage = getAverage(arr, samples);
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
String dataString(String sensor1, String sensor2, String sensor3, String sensor4, String sensor6) {
  String reading;
  reading = sensor1 + "," + sensor2 + "," + sensor3 + "," + sensor4 + ","  + sensor6;
  return reading;
}

void createArray(int analogPin) {
  int raw;
  int i = 0;
  do {
    raw = filter(analogPin);
    i++;
  } while (i < 10); //throw first ten readings

  i = 0;

  do {
    raw = filter(analogPin);
    arr[i] = readVcc(raw);
    i++;
  } while (i < 100);
}

float getAverage(float *a, int numbers) {
  float sum = 0;
  for (int i = 0; i < numbers; i++) {
    sum = sum + a[i];
  }
  float average = sum / numbers;
  return average;
}

int filter(int analogPin) {
  int rawValue = analogRead(analogPin);
  ADCFilter.Filter(rawValue);
  return ADCFilter.Current();
}

float readVcc(int analog) {
  long result;
  float voltage;
  // Read 1.1V reference against AVcc
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Calculate Vcc (in mV); 1126400 = 1.1*1024*1000
  //return result;
  voltage = ((result / 1000.00) * analog) / 1024.00;
  return voltage;
}


String readData() {
  SoftwareSerial ecSerial(2, 3); //ec sensor TX --> D2 of atmega
  SoftwareSerial doSerial(4, 5);  // do sensor TX --> D4 of atmega
 
  unsigned long startMillis;
  unsigned long currentMillis;
  String stringData;
  unsigned long ec_responseTime = 10000;
  unsigned long do_responseTime = 10000;
  unsigned long ph_responseTime = 10000;
  unsigned long chl_responseTime = 10000;
  float voltagePercent;
  /*int samples = 30;
    float arr[samples + 1];*/
  voltagePercent = getBatLevel() * 100 / 3.7;


  /*--------------temp sensor-----------*/
  //get temperature reading

  digitalWrite(tempPow, HIGH);  //turn on temp sensor
  tempSensor.begin();
  delay(1000);
  tempSensor.requestTemperatures();  //get celsius temp
  temp = tempSensor.getTempCByIndex(0); //actual temp reading in celsius
  digitalWrite(tempPow, LOW);
  delay(100);
  /*--------------EC sensor-----------*/
  //get ec sensor reading
  digitalWrite(ecPow, HIGH);
  ecSerial.begin(9600);
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
  } while (currentMillis - startMillis <= ec_responseTime);

  for (int i = 0; i < samples; i++) {
    while (ecSerial.available() > 0) {
      char serialChar = (char)ecSerial.read();
      ec += serialChar;
    }
    arr[i] = ec.toFloat();
    delay(50);
  }

  ec = String(getAverage(arr, samples));
  digitalWrite(ecPow, LOW);
  delay(100);
  ecSerial.end();
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
  /*--------------DO sensor-----------*/
  digitalWrite(doPow, HIGH);
  doSerial.begin(9600);
  //get do sensor reading
  startMillis = millis();  //initial start time
  do {
    currentMillis = millis();
    dO = "";
    doSerial.listen();  //listen to doSerial port
    delay(999);         //wait!! for some reason hindi pwedeng pareho sila ng delay ng unang software serial, thus 999
    while (doSerial.available() > 0) {
      char serialChar = (char)doSerial.read();
      dO += serialChar;
    }
  } while (currentMillis - startMillis <= do_responseTime);
  
  for (int i = 0; i < samples; i++) {
    while (doSerial.available() > 0) {
      char serialChar = (char)doSerial.read();
      dO += serialChar;
    }
    arr[i] = dO.toFloat();
    delay(50);
  }

  dO = String(getAverage(arr, samples));
  digitalWrite(doPow, LOW);
  delay(100);
  doSerial.end();
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  /*--------------pH sensor-----------*/
  //get pH sensor reading
  digitalWrite(phPow, HIGH); //turn on pH sensor
  //delay(100);
  /*startMillis = millis();
    do {
    currentMillis = millis();
    analogRead(A0);
    } while (currentMillis - startMillis <= ph_responseTime);
  */
  Serial.begin(9600);
  createArray(A0);
  phVoltage = getAverage(arr, samples);
  Serial.print("phVoltage:"); Serial.println(phVoltage);
  ph = String((phSlope * phVoltage) + phIntercept); //voltage to actual pH value
  digitalWrite(phPow, LOW);   //turn off pH sensor
  delay(100);
  /*--------------wp sensor-----------
    //get wp sensor reading
    digitalWrite(wpPow, HIGH);  //turn on wp sensor
    delay(1000);
    wpVoltage = analogRead(A2) * 5.00 / 1023.00;
    wp = String((wpVoltage - wpOffSet) * 400);  //calibration formula of w.p. sensor from manufacturer's spec sheet
    digitalWrite(wpPow, LOW);  //turn off turb sensor*/

  /*-------------chl sensor---------*/
  //get chl sensor reading
  digitalWrite(chlPow, HIGH);
  startMillis = millis();
  do {
    currentMillis = millis();
    analogRead(A4);
  } while (currentMillis - startMillis <= chl_responseTime);

  createArray(A4);
  chlVoltage = getAverage(arr, samples);
  Serial.print("chlVoltage:"); Serial.println(chlVoltage); Serial.end();
  chl = String((chlSlope * chlVoltage) + chlIntercept); //voltage to actual chl value
  digitalWrite(chlPow, LOW);

  return stringData = "spdata," + String(ID) + ","  + dataString(ec, dO, temp, ph, chl) + "," + String(voltagePercent);
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

  for (int i = 0; i < (round(sleep_length * 450) + random(0, 5)) ; i++) { //adjust this to extend sleep time -- 450 is the 8 sec conversion factor for sleep time
    MCUCR |= (3 << 5);
    MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6);
    __asm__ __volatile("sleep"::);
  }
  /*SoftwareSerial ecSerial(2, 3); //ec sensor TX --> D2 of atmega
  SoftwareSerial doSerial(4, 5);  // do sensor TX --> D4 of atmega*/
//  ecSerial.begin(9600);
//  doSerial.begin(9600); 
  ADCSRA = old_ADCSRA;
  dataToLora = readData();
  Serial.begin(9600);
  delay(1000);
  Serial.println(dataToLora);

  ec = "";
  dO = "";


}
