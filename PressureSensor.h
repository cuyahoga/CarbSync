#ifndef PressureSensor_h
#define PressureSensor_h

#include "Arduino.h"
#include "Sensor.h"

class PressureSensor : public Sensor {
    public:
      PressureSensor(String desc, byte pressurePin);
      int getPressureKpa();
      byte getPressurePin();
      int readPressureKpa(int vccMv);
    protected:
      virtual unsigned int getPressureMinKpa() = 0;
      virtual unsigned int getPressureMinMv() = 0;
      virtual unsigned int getPressureMaxKpa() = 0;
      virtual unsigned int getPressureMaxMv() = 0;
    private:
      void init();
};




#endif // PressureSensor_h