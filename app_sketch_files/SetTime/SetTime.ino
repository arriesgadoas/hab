#include <EEPROM.h>
int address1 = 0;
int address2 = address1 + sizeof(float);
float data;
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
  if(Serial.available()>0){
    dataString = Serial.readString();
    }
  data = dataString.toFloat();
  Serial.println(data);
  if (data > 0) {
    EEPROM.put(address2, data);
  }
}
