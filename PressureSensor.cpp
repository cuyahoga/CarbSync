#include "Arduino.h"
#include "PressureSensor.h"

PressureSensor::PressureSensor(String desc, byte pressurePin) : Sensor(desc, pressurePin) {
  init();
}
  
void PressureSensor::init() {
}

int PressureSensor::getPressureKpa() {
  return mapReading(getCurrentReading() + getPressureOffset(), getPressureMinMv(), getPressureMaxMv(), getPressureMinKpa(), getPressureMaxKpa());
}

int PressureSensor::getPressureOffset() {
  return _pressureOffset;
}

byte PressureSensor::getPressurePin() {
  return PressureSensor::getPin();
}

int PressureSensor::readPressureKpa(int vccMv) {
  readSensor(vccMv);
  return getPressureKpa();
}

void PressureSensor::setPressureOffset(int offset) {
  _pressureOffset = offset;
}



