#ifndef Bosch0261230043_h
#define Bosch0261230043_h

#include "Arduino.h"
#include "PressureSensor.h"

class Bosch0261230043 : public PressureSensor {
  /*
   * The family is documented at http://www.bosch.com.au/car_parts/en/downloads/Map_Sensor_Technical_Specification.pdf
   * The only specific reference to this variant is at http://avtodata.by/product_10836.html from which the following specs were taken
   *
   * The pressure sensor has the following specification at 5V supply;
   *
   * 0.405V =  10 kPa
   * 4.750V = 117 kPa
   */

  public:
    Bosch0261230043(String desc, byte pressurePin);
  protected:
    static const unsigned int PRESSURE_MIN_MV  = 405;
    static const unsigned int PRESSURE_MAX_MV  = 4750;
    static const unsigned int PRESSURE_MIN_KPA = 10;
    static const unsigned int PRESSURE_MAX_KPA = 117;
    static const unsigned int REFERENCE_MV     = 5000;
    virtual unsigned int getPressureMinKpa();
    virtual unsigned int getPressureMinMv();
    virtual unsigned int getPressureMaxKpa();
    virtual unsigned int getPressureMaxMv();
    virtual unsigned int getReferenceMv();
};

#endif // Bosch0261230043_h
