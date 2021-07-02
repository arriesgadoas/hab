#include "Senspak.h"

Senspak::Senspak(bool state) {

}

SoftwareSerial Senspak::setEcSerial(byte t, byte r) {
  SoftwareSerial serial(t, r);
  return serial;
}

SoftwareSerial Senspak::setDoSerial(byte t, byte r) {
  SoftwareSerial serial(t, r);
  return serial;
}

void Senspak::setEcPowPin(byte p, bool state) {
  pinMode(p, OUTPUT);
  if (state == false) {
    digitalWrite(p, LOW);
  }
  else {
    digitalWrite(p, HIGH);
  }
}

void Senspak::setDoPowPin(byte p, bool state) {
  pinMode(p, OUTPUT);
  if (state == false) {
    digitalWrite(p, LOW);
  }
  else {
    digitalWrite(p, HIGH);
  }
}
