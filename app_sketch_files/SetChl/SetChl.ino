  #include "Filter.h"
#include <SoftwareSerial.h>
#include <EEPROM.h>
#define power 12
#define data A4
#define samples 100
float final_arr[20];
float arr[samples + 1];
int address = (5 * sizeof(float)) + sizeof(long);
long FilterWeight = 20;
long refVoltageScale;
ExponentialFilter<long> ADCFilter(FilterWeight, 0);
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(power, OUTPUT);
  digitalWrite(power, HIGH);
}

void loop() {
  
  for (int i = 0; i < 20; i++) {
    voltageArray(data);
    final_arr[i] = getAverage(arr, samples);
  }
  Serial.println(getAverage(final_arr, samples), 2);

}

void voltageArray(int analogPin) {
  int raw;
  int i = 0;
  do {
    raw = filter(analogPin);
    i++;
  } while (i < 100); //throw first ten readings

  i = 0;

  do {
    raw = filter(analogPin);
    arr[i] = readVcc(raw);
    i++;
  } while (i < samples);
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
  EEPROM.get(address, refVoltageScale);
  result = refVoltageScale / result;
  voltage = ((result / 1000.00) * analog) / 1024.00;
  return voltage;
}
