#include "RTClib.h" // https://github.com/adafruit/RTClib
#include <Wire.h>

RTC_DS3231 rtc;

void setup() {
  Wire.begin(0,25);
  Serial.begin(115200);

  // start the connexion to the RTC

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1); // can't go further
  }

  // Uncomment this code to set the time of your RTC to the last compile time of this code
  // if you compile and upload in one go, that will be close enough to a good time
  // this is convenient as you don't need to manually mess around with the time manually

   rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));



  // or uncomment this line to decide what you want as a date/time for your RTC
  // format is year, month, day, hours, minutes, seconds
  // but you need to set that manually, take into account it takes a bit of time to compile, upload and run
  // so give yourself some padding in the seconds and check your wall clock and press reset on your arduino
  // when it's the right time. 

  // rtc.adjust(DateTime(2017, 7, 16, 16, 35, 20));

  Serial.println("RTC date is set");
}

void loop() {
  DateTime time = rtc.now();
  Serial.println(time.timestamp(DateTime::TIMESTAMP_TIME));
  delay(1000);
  }
