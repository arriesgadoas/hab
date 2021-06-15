//include libraries
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include "Filter.h"
#include <avr/sleep.h>
#include <avr/power.h>

//arrays and variables for sampling
#define samples 20
#define ecDo_samples 10
float arr[samples];
float ecDo_arr[ecDo_samples];

//decalarations for filter parameters
long FilterWeight = 20; 
ExponentialFilter<long> ADCFilter(FilterWeight, 0);

//EEPROM addresses
int address1 = 0;                                   //ID address
int address2 = address1 + sizeof(float);            //sleep time address
int address3 = address2 + sizeof(float);            //ph_slope address
int address4 = address3 + sizeof(float);            //ph_intercept address
int address5 = address4 + sizeof(float);            //chl_slope address
int address6 = address5 + sizeof(float);            //chl_intercept address
int address7 = (5 * sizeof(float)) + sizeof(long);  //voltage reference constant

//constants from EEPROM
int ID;
float sleep_length;
float phSlope;
float phIntercept;
float chlSlope;
float chlIntercept;
long refVoltageScale;

//temperature sensor setup
#define ONE_WIRE_BUS 6
OneWire oneWire_temp(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire_temp);

//5V pins for sensors
#define tempPow 7
#define phPow 8
#define wpPow 10
#define doPow 11
#define chlPow 12
#define ecPow 9

//diagnostic variables
String diagnostics = "";
int checkStatus;


//sensor values in String
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
  EEPROM.get(address1, ID);
  pinMode(ecPow, OUTPUT);
  pinMode(tempPow, OUTPUT);
  pinMode(phPow, OUTPUT);
  pinMode(doPow, OUTPUT);
  pinMode(wpPow, OUTPUT);
  pinMode(chlPow, OUTPUT);
  pinMode(A2, OUTPUT);

  dataToLora = diagnostic();
  digitalWrite(A2, HIGH);
  delay(500);
  digitalWrite(A2, LOW);
  Serial.println(dataToLora);
  delay(3000);
}


//function to get battery level
float getBatLevel() {
  float actualVoltage;
  voltageArray(A1);
  actualVoltage = getAverage(arr, samples);
  return actualVoltage;
}

int checkProbe() {
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

//dataString function
String dataString(String sensor1, String sensor2, String sensor3, String sensor4, String sensor6) {
  String reading;
  reading = sensor1 + "," + sensor2 + "," + sensor3 + "," + sensor4 + ","  + sensor6;
  return reading;
}

//create array for analog sensor readings
void voltageArray(int analogPin) {
  int raw;
  int i = 0;
  do {
    raw = filter(analogPin);
    delay(500);
    i++;
  } while (i < 10);
  i=0;
  do {
    raw = filter(analogPin);
    arr[i] = readVcc(raw); 
//    Serial.println(arr[i]);
    i++;
    delay(100);
  } while (i < samples);
}

//getaverage from arrays
float getAverage(float *a, int numbers) {
  float sum = 0;
  for (int i = 0; i < numbers; i++) {
    sum = sum + a[i];
  }
  float average = sum / numbers;
  return average;
}

//filtering function
int filter(int analogPin) {
  int rawValue = analogRead(analogPin);
  ADCFilter.Filter(rawValue);
  return ADCFilter.Current();
}

//get voltage reading for sensors
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
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  EEPROM.get(address7, refVoltageScale);
  result = refVoltageScale / result;
  voltage = (((result / 1000.00) * analog) / 1024.00);
  return voltage;
}

//get sensor readings
void readData() {
  detachInterrupt(1);
  EEPROM.get(address3, phSlope);
  EEPROM.get(address4, phIntercept);
  EEPROM.get(address5, chlSlope);
  EEPROM.get(address6, chlIntercept);
  unsigned long startMillis;
  unsigned long currentMillis;
  String stringData;
  unsigned long ec_responseTime = 0;
  unsigned long do_responseTime = 25000;
  unsigned long ph_responseTime = 25000;
  unsigned long chl_responseTime = 25000;
  unsigned long temp_responseTime = 0;//10000;
  float voltagePercent;

  

  //get temperature reading

  Serial.println("Reading temperature...");
  digitalWrite(tempPow, HIGH);  //turn on temp sensor
  tempSensor.begin();
  delay(1000);
  startMillis = millis();

  do {
    currentMillis = millis();
    tempSensor.requestTemperatures();  //get celsius temp
    temp = tempSensor.getTempCByIndex(0); //actual temp reading in celsius
  } while (currentMillis - startMillis <= temp_responseTime);

  for (int i = 0; i < samples; i++) {
    tempSensor.requestTemperatures();  //get celsius temp
    arr[i] = tempSensor.getTempCByIndex(0); //actual temp reading in celsius
    delay(50);
  }
  temp = getAverage(arr, samples);
  digitalWrite(tempPow, LOW);
  delay(100);

//  get ec sensor reading
  
    Serial.println("Reading conductivity...");
    digitalWrite(ecPow, HIGH);
    SoftwareSerial ecSerial(2, 3); //ec sensor TX --> D2 of atmega
  
    ecSerial.begin(9600);
  
    delay(ec_responseTime);
    int i = 0;
    while(i < ecDo_samples){
      ec = "";
      while (ecSerial.available() > 0) {
        char serialChar = (char)ecSerial.read();
        ec += serialChar;
        if (serialChar == '\r') {
         // Serial.println(i);
          if(isdigit(ec[0])== false){
//              Serial.println("xxx");
            }
          else{
          ecDo_arr[i] = ec.toFloat();
//          Serial.println(ecDo_arr[i]);
          i++;
  
          }
        }
      }
      delay(1000);
    }
  
    ec = String(getAverage(ecDo_arr, ecDo_samples));
    digitalWrite(ecPow, LOW);
    pinMode(2, INPUT_PULLUP);
    pinMode(3, INPUT_PULLUP);
    digitalWrite(2, LOW);
    digitalWrite(3, LOW);
    delay(100);
    ecSerial.end();
  
  
    // get DO reading
    i = 0;
    Serial.println("Reading dissolved oxygen...");
    digitalWrite(doPow, HIGH);
    delay(50);
    SoftwareSerial doSerial(4, 5);  // do sensor TX --> D4 of atmega
    doSerial.begin(9600);
  
    //delay(5000);
   while(i < ecDo_samples){
      dO = "";
     // doSerial.println("T,27");
      while (doSerial.available() > 0) {
        char serialChar = (char)doSerial.read();
        dO += serialChar;
        if (serialChar == '\r'){
          if(isdigit(dO[0])==false){
            }
  
          else{
            ecDo_arr[i] = dO.toFloat();
//            Serial.println(ecDo_arr[i]);
            i++;
            }
          }
      }
  
      delay(1000);
    }
  
    dO = String(getAverage(ecDo_arr, ecDo_samples));
    digitalWrite(doPow, LOW);
  
    delay(100);
    doSerial.end();
    delay(100);
    i = 0;
  
  //  //get pH sensor reading
  Serial.println("Reading pH...");
  digitalWrite(phPow, HIGH); //turn on pH sensor

  while(i<2){ //let sensor settle
    voltageArray(A0);
    delay(500);
    i++;
    }
  voltageArray(A0);
  phVoltage = getAverage(arr, samples);
  ph = String((phSlope * phVoltage) + phIntercept); //voltage to actual pH value
  digitalWrite(phPow, LOW);   //turn off pH sensor
  delay(100);

  //get chl sensor reading

  Serial.println("Reading chl...");
  digitalWrite(chlPow, HIGH);
  voltageArray(A4);
  chlVoltage = getAverage(arr, samples);
  chl = String((chlSlope * chlVoltage) + chlIntercept); //voltage to actual chl value
  digitalWrite(chlPow, LOW);
  delay(100);

  Serial.println("Reading battery level...");
  voltagePercent = 100 * (1 - (4.2 - getBatLevel()));
  
  stringData = "spdata," + String(ID) + ","  + dataString(ec, dO, temp, ph, chl) + "," + String(voltagePercent);
  digitalWrite(A2, HIGH);
  delay(500);
  digitalWrite(A2, LOW);
  Serial.println(stringData);
  //pinMode(2, INPUT_PULLUP);
  delay(100);
  sleep_disable();
}


//function to perform system diagnostic
String diagnostic() {
  String stringData;
  int probeStatus;
  float batVoltage;
  batVoltage = getBatLevel();
  if (batVoltage <= 3.2) {
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

void enterSleep() {
  sleep_enable();//Enabling sleep mode
  attachInterrupt(1, readData, LOW);//attaching a interrupt to pin d2
  delay(1000);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);//Setting the sleep mode, in our case full sleep
  sleep_cpu();//activating sleep mode
}

void loop() {
  delay(1000);
  pinMode(3, INPUT_PULLUP);
  enterSleep();
}
