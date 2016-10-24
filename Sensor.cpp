#include "Arduino.h"
#include "Sensor.h"

Sensor::Sensor(String desc, byte pin) {
  init(desc, pin);
}

void Sensor::init(String desc, byte pin) {
  _desc = desc;
  _pin = pin;
  _sampleCount = 1;
  _sampleDelay = 0;
}

float Sensor::getCurrentReading() {
  return _currentReading;
}

float Sensor::getCurrentReadingMv() {
  return getCurrentReading() * getVccMvPerUnit();
}

String Sensor::getDesc() {
  return _desc;
}

byte Sensor::getPin() {
  return _pin;
}

float Sensor::getVccOffset() {
  return (float)getVccMv() / (float)getReferenceMv();
}

unsigned int Sensor::getVccMv() {
  return _vccMv;
}

float Sensor::getVccMvPerUnit() {
  return getVccMv() / 1023.0;
}

int Sensor::mapCurrentReading(float inMinMv, float inMaxMv, int outMin, int outMax) {
  /**
   * Map the voltage reading from a sensor to a given value range, against a soure range compensated against supply voltage
   *
   * We know that the 10-bit ADC has a range of 1024 values, so when it is supplied with 5V that gives us 5000/1024 = 4.8828125 mV/ADC increment
   * The in_min voltage represents out_min pressure and the in_max voltage represents out_max pressure
   * We therefore map the supplied reading on the input scale and return its relative value on the output scale
   *
   * Worked example;
   *  
   * pressure = mapsensor(807, 250, 4650, 17, 117)
   * pressure = map(807, round((250 / 4.8828125) - 1), round(4650 / 4.8828125) - 1), 17, 117)
   * pressure = map(807, round(51.2 - 1), round(952.32 - 1), 17, 117)
   * pressure = map(807, 50, 951, 17, 117)
   * pressure = 101 kPa
   */
//  Serial.print("mapCurrentReading(");Serial.print(inMinMv);Serial.print(", ");Serial.print(inMaxMv);Serial.print(", ");Serial.print(outMin);Serial.print(", ");Serial.print(outMax);Serial.println(");");
  return map(getCurrentReading(), round(((inMinMv * getVccOffset()) / getVccMvPerUnit()) - 1), round(((inMaxMv * getVccOffset()) / getVccMvPerUnit()) - 1), outMin, outMax);
}

void Sensor::readSensor(int vccMv) {

  if (_sampleCount == 1) {
    _currentReading = analogRead(getPin());
  } else {
    for (byte sample = 0; sample < _sampleCount; sample++) {
      _samples[sample] = analogRead(getPin());
      delay(_sampleDelay);
    }
    _currentReading = 0;
    for (int sample = 0; sample < _sampleCount; sample++) {
       _currentReading += _samples[sample];
    }
    _currentReading /= _sampleCount;
  }
  _vccMv = vccMv;
}

void Sensor::setSampling(byte sampleCount, byte sampleDelay) {
  _sampleCount = min(sampleCount, SAMPLE_MAX_COUNT);
  _sampleDelay = sampleDelay;
}

