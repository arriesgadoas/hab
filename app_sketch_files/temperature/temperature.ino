#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 6                  //temperature sensor signal output to D6 of ATMEGA168
OneWire oneWire_temp(ONE_WIRE_BUS);     // Setup a oneWire instance to communicate with the temperature sensor
DallasTemperature tempSensor(&oneWire_temp);    // Pass oneWire_temp reference to DallasTemperature library
#define tempPow 7
String temp = "";
void setup() {
  // put your setup code here, to run once:
  pinMode(tempPow, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(tempPow, HIGH);
  tempSensor.begin();
  tempSensor.requestTemperatures();  //get celsius temp
  temp = tempSensor.getTempCByIndex(0); //actual temp reading in celsius
  Serial.println(temp);
  delay(500);
}
