#include "Arduino.h"
#include "Bosch0261230043.h"

Bosch0261230043::Bosch0261230043(String desc, byte pressurePin) : PressureSensor(desc, pressurePin) {
}

unsigned int Bosch0261230043::getPressureMinKpa() {
  return PRESSURE_MIN_KPA;
}

unsigned int Bosch0261230043::getPressureMinMv() {
  return PRESSURE_MIN_MV;
}

unsigned int Bosch0261230043::getPressureMaxKpa() {
  return PRESSURE_MAX_KPA;
}

unsigned int Bosch0261230043::getPressureMaxMv() {
  return PRESSURE_MAX_MV;
}

unsigned int Bosch0261230043::getReferenceMv() {
  return REFERENCE_MV;
}

