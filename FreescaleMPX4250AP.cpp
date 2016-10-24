#include "Arduino.h"
#include "FreescaleMPX4250AP.h"

FreescaleMPX4250AP::FreescaleMPX4250AP(String desc, byte pressurePin) : PressureSensor(desc, pressurePin) {
}

unsigned int FreescaleMPX4250AP::getPressureMinKpa() {
  return PRESSURE_MIN_KPA;
}

unsigned int FreescaleMPX4250AP::getPressureMinMv() {
  return PRESSURE_MIN_MV;
}

unsigned int FreescaleMPX4250AP::getPressureMaxKpa() {
  return PRESSURE_MAX_KPA;
}

unsigned int FreescaleMPX4250AP::getPressureMaxMv() {
  return PRESSURE_MAX_MV;
}

unsigned int FreescaleMPX4250AP::getReferenceMv() {
  return REFERENCE_MV;
}

