#include <EEPROM.h>
int address1 = 0;                       //ID
int address2 = address1 + sizeof(float);  //sleep time
int address3 = address2 + sizeof(float);  //ph_slope address
int address4 = address3 + sizeof(float);  //ph_intercept address
float data1;
float data2;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(13, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  String dataString = "";
  /*while (Serial.available() > 0) {
    char serialChar = (char)Serial.read();
    dataString += serialChar;
    }*/
  if (Serial.available() > 0) {
    dataString = Serial.readString();
    Serial.println(dataString);
    char buff[dataString.length() + 1];
    dataString.toCharArray(buff, dataString.length() + 1);
    char* first =  strtok(buff, ":");
    char* second = strtok(NULL, "\r");
    String data1_S = first;
    data1 = data1_S.toFloat();
    String data2_S = second;
    data2 = data2_S.toFloat();
    Serial.println(data1);
    Serial.println(data2);
    EEPROM.put(address3, data1);
    EEPROM.put(address4, data2);
  }
    
  delay(1000);
}
