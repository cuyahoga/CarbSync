#ifndef FreescaleMPX4250AP_h
#define FreescaleMPX4250AP_h

#include "Arduino.h"
#include "PressureSensor.h"

class FreescaleMPX4250AP : public PressureSensor {
  /*
   * The spec sheet for this sensor is available at http://cache.freescale.com/files/sensors/doc/data_sheet/MPX4250A.pdf
   *
   * The pressure sensor has the following specification at 5.1V supply;
   *
   * 0.204V =  20 kPa
   * 4.896V = 250 kPa
   */

  public:
    FreescaleMPX4250AP(String desc, byte pressurePin);
  protected:
    static const unsigned int PRESSURE_MIN_MV  = 204;
    static const unsigned int PRESSURE_MAX_MV  = 4896;
    static const unsigned int PRESSURE_MIN_KPA = 20;
    static const unsigned int PRESSURE_MAX_KPA = 250;
    static const unsigned int REFERENCE_MV     = 5100;
    virtual unsigned int getPressureMinKpa();
    virtual unsigned int getPressureMinMv();
    virtual unsigned int getPressureMaxKpa();
    virtual unsigned int getPressureMaxMv();
    virtual unsigned int getReferenceMv();
};


#endif // FreescaleMPX4250AP_h
