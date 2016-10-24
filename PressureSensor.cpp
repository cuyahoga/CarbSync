#include "Arduino.h"
#include "PressureSensor.h"

PressureSensor::PressureSensor(String desc, byte pressurePin) : Sensor(desc, pressurePin) {
  init();
}
  
void PressureSensor::init() {
}

int PressureSensor::getPressureKpa() {
  return mapCurrentReading(getPressureMinMv(), getPressureMaxMv(), getPressureMinKpa(), getPressureMaxKpa());
}

byte PressureSensor::getPressurePin() {
  return PressureSensor::getPin();
}

int PressureSensor::readPressureKpa(int vccMv) {
  readSensor(vccMv);
  return getPressureKpa();
}




