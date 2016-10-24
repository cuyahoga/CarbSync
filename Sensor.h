#ifndef Sensor_h
#define Sensor_h

#include "Arduino.h"

#define SAMPLE_MAX_COUNT 20

class Sensor {
  public:
    Sensor(String desc, byte pin);
    inline virtual void begin(){ /*nothing*/ };
    float getCurrentReading();
    float getCurrentReadingMv();
    String getDesc();
    byte getPin();
    void setSampling(byte sampleCount, byte sampleDelay);
  protected:
    virtual unsigned int getReferenceMv() = 0;
    float getVccOffset();
    unsigned int getVccMv();
    float getVccMvPerUnit();
    int mapCurrentReading(float inMinMv, float inMaxMv, int outMin, int outMax);
    int mapReading(float reading, float inMinMv, float inMaxMv, int outMin, int outMax);
    void readSensor(int vccMv);
  private:
    String _desc;
    byte _pin;
    int _samples[SAMPLE_MAX_COUNT];
    byte _sampleCount;
    byte _sampleDelay;
    float _currentReading;
    int _vccMv;
    void init(String desc, byte pin);
};

#endif // Sensor_h
