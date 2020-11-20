#include <EEPROM.h>
int address1 = 0;
int address2 = address1 + sizeof(int);
int data;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(13, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  String dataString = "";
  while (Serial.available() > 0) {
    char serialChar = (char)Serial.read();
    dataString += serialChar;
  }
  data = dataString.toInt();
  Serial.println(data);
  if (data > 0) {
    EEPROM.put(address2, data);
  }
}
