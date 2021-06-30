#ifndef senspak
#define senspak

#include <Arduino.h>
#include <SoftwareSerial.h>

class Senspak{
  public:
    //constructor
    Senspak(bool state = false);
    SoftwareSerial setEcSerial(byte t, byte r);
    SoftwareSerial setDoSerial(byte t, byte r);
    void setEcPowPin(byte p = 0, bool state = false);
    void setDoPowPin(byte p = 0, bool state = false);
    //methods

  private:
  };
#endif
