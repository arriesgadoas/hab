#include <EEPROM.h>
int address1 = 0;                         //ID address
int address2 = address1 + sizeof(float);    //sleep address
int address3 = address2 + sizeof(float);  //ph_slope address
int address4 = address3 + sizeof(float);  //ph_intercept address
int address5 = address4 + sizeof(float);  //chl_slope address
int address6 = address5 + sizeof(float);  //chl_intercept address\

int data1;
float data2;
float data3;
float data4;
float data5;
float data6;

void setup() {
  Serial.begin(9600);
  EEPROM.get(address1,data1);
  delay(200);
  EEPROM.get(address2, data2);
  delay(200);
  EEPROM.get(address3, data3);
  delay(200);
  EEPROM.get(address4, data4);
  delay(200);
  EEPROM.get(address5, data5);
  delay(200);
  EEPROM.get(address6, data6);
  Serial.print("ID: ");
  Serial.println(data1);
  Serial.print("SLEEP: ");
  Serial.println(data2);
  Serial.print("PH SLOPE: ");
  Serial.println(data3);
  Serial.print("PH INTERCEPT: ");
  Serial.println(data4);
  Serial.print("CHL SLOPE: ");
  Serial.println(data5);
  Serial.print("CHL INTERCEPT: ");
  Serial.println(data6);
}

void loop() {
  // put your main code here, to run repeatedly:

}
