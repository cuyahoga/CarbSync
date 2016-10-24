/*********************************************************************
 *
 * Carbotron 4000
 *
 *
 *********************************************************************/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>        // https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h>    // https://github.com/adafruit/Adafruit_SSD1306
#include <movingAvg.h>           // https://github.com/JChristensen/movingAvg
#include <Timer.h>               // https://github.com/JChristensen/Timer
#include "Vcc.h"                 // http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
#include <Potentiometer.h>       // http://playground.arduino.cc/Code/Potentiometer

// ===============================================================
// Adafruit SSD1306 128x64 OLED
// ===============================================================
#define OLED_DC         11
#define OLED_CS         12
#define OLED_CLK        10
#define OLED_MOSI       9
#define OLED_RESET      13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// ===============================================================
// VCC adjustment for the specific board the code is running on
//   (1.1 * (1+(1-(readVcc()/VccFromVoltmeter))) * 1023 * 1000
//   =
//   (1.1 * (1+(1-(4935/4965))) * 1023 * 1000
// ===============================================================
#define VCC_ADJUSTMENT 1125300L //1132099L

// ===============================================================
// Bosch 0 261 230 043 MAP sensor
// The family is documented at http://www.bosch.com.au/car_parts/en/downloads/Map_Sensor_Technical_Specification.pdf
// The only specific reference to this variant is at http://avtodata.by/product_10836.html from which the following specs were taken
// 
// The pressure sensor has the following specification at 5V supply;
// 
//   0.405V =  10 kPa
//   4.750V = 117 kPa
// ===============================================================
#define MAP_SENSOR_MIN_MV        405
#define MAP_SENSOR_MAX_MV        4750
#define MAP_SENSOR_MIN_KPA       10
#define MAP_SENSOR_MAX_KPA       117
#define MAP_SENSOR_REFERENCE_MV  5000.0

// Potentiometers
#define ADJUSTER_SECTORS 70
#define ADJUSTER_OFFSET  35

// ==========================
// Display modes
// ==========================
#define MODE_SPLASH            0
#define MODE_BALANCE           1
#define MODE_UNIT_OF_MEASURE   2
#define MODE_CALIBRATION       3
#define MODE_COUNT             4

// ==========================
// Units Of Measure
// ==========================
#define UOM_KPA     0
#define UOM_HGIN    1
#define UOM_PSI     2
#define UOM_COUNT   3


// Buttons
#define BUTTON_MODE          1     // The interrupt number for the Mode button
#define BUTTON_ACTION        0     // The interrupt number for the Action button

// Runtime configuration
#define ADJUSTER_INTERVAL    150   // Millis between updating adjuster values
#define DEBOUNCE_INTERVAL    50    // Millis for debouncing
#define DISPLAY_INTERVAL     1000  // Millis between display refreshes
#define MAP_SENSOR_COUNT     4     // Number of MAP sensors
#define MAP_SENSOR_INTERVAL  20    // Millis between sensor readings
#define MIN_PRESSURE         50
#define MAX_PRESSURE         120



byte mode = MODE_SPLASH;
byte uom = UOM_KPA;
volatile long lastButtonModeDebounceTime = 0;
volatile long lastButtonActionDebounceTime = 0;

Timer t;
int tickAdjusters, tickBaseline, tickDisplay, tickSensors;

int mapSensorPins[MAP_SENSOR_COUNT] = {A0, A1, A2, A3};
movingAvg mapSensorReadings[MAP_SENSOR_COUNT] = {movingAvg(), movingAvg(), movingAvg(), movingAvg()}; 
movingAvg mapSensorReadingsMv[MAP_SENSOR_COUNT] = {movingAvg(), movingAvg(), movingAvg(), movingAvg()}; 
movingAvg mapSensorReadingsKPa[MAP_SENSOR_COUNT] = {movingAvg(), movingAvg(), movingAvg(), movingAvg()}; 
Potentiometer adjusters[MAP_SENSOR_COUNT] = {Potentiometer(A4, ADJUSTER_SECTORS), Potentiometer(A5, ADJUSTER_SECTORS), Potentiometer(A6, ADJUSTER_SECTORS), Potentiometer(A7, ADJUSTER_SECTORS)};
int adjusterReadings[MAP_SENSOR_COUNT];
int atmosphericKPa = 0;


void setup() {
  Serial.begin(9600);

  // Set up the 128x64 OLED
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();

  attachInterrupt(BUTTON_MODE, handleButtonMode, RISING);
  attachInterrupt(BUTTON_ACTION, handleButtonAction, RISING);

  tickAdjusters = t.every(ADJUSTER_INTERVAL, readAdjusters);
  tickBaseline = t.after(1000, readAtmosphericPressure);
  tickDisplay = t.every(DISPLAY_INTERVAL, refreshDisplay);
  tickSensors = t.every(MAP_SENSOR_INTERVAL, readMAPSensors);
}

void loop() {
  
  // See if it's time to act on a timer
  t.update();

}

void readAtmosphericPressure() {
  
   // Establish power specification and then set the baseline atmospheric pressure value
  int vccMv = readVcc(VCC_ADJUSTMENT);
  atmosphericKPa = convertReadingToKPa(analogRead(mapSensorPins[0]) + adjusterReadings[0], (float)vccMv / MAP_SENSOR_REFERENCE_MV, vccMv / 1023.0, MAP_SENSOR_MIN_MV, MAP_SENSOR_MAX_MV, MAP_SENSOR_MIN_KPA, MAP_SENSOR_MAX_KPA);
}

String formatReadingInUom(int source, bool appendUom) {
  
  String buf;
  switch(uom) {
    case UOM_PSI:
      buf = String(convertKPaToUom(source), 1);
      break;
    case UOM_HGIN:
      buf = String(convertKPaToUom(source), 0);
      break;
    default:
      buf += source;
  }
  if (appendUom) {
    buf += " " + getUomDesc();
  }
  return buf;
}

String getUomDesc() {
  
  String desc[UOM_COUNT] = {"KPa", "HgIn", "PSI"};
  return desc[uom];
}

void readAdjusters() {
    
  int offset;
  for (byte adjuster = 0; adjuster < MAP_SENSOR_COUNT; adjuster++) { 
    adjusterReadings[adjuster] = ((adjusters[adjuster].getSector() - ADJUSTER_SECTORS) * -1) - ADJUSTER_OFFSET;
  }
  
}
void readMAPSensors() {
  
  //unsigned long time = millis();
  
  // Establish power specification
  int vccMv = readVcc(VCC_ADJUSTMENT);
  //Serial.print(F("VCC: ")); Serial.print(vccMv); Serial.println(F(" mV"));

  int currentReading;
  float mvPerUnit = vccMv / 1023.0;
  float currentReadingMv;
  float vccOffset = (float)vccMv / MAP_SENSOR_REFERENCE_MV;
  float currentReadingKPa;

  for (byte sensor = 0; sensor < MAP_SENSOR_COUNT; sensor++) { 
    currentReading = analogRead(mapSensorPins[sensor]) + adjusterReadings[sensor];
    currentReadingMv = currentReading * (mvPerUnit);
    currentReadingKPa = convertReadingToKPa(currentReading, vccOffset, mvPerUnit, MAP_SENSOR_MIN_MV, MAP_SENSOR_MAX_MV, MAP_SENSOR_MIN_KPA, MAP_SENSOR_MAX_KPA);
    
    mapSensorReadings[sensor].reading(currentReading);
    mapSensorReadingsMv[sensor].reading(currentReadingMv);
    mapSensorReadingsKPa[sensor].reading(currentReadingKPa);
    
    //Serial.print(currentReading); Serial.print(F("  ")); Serial.print(currentReadingMv); Serial.println(F("mv"));
  }
  //Serial.print(F("Took: ")); Serial.println(millis() - time);
}
  
void refreshDisplay() {
 
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 1);

  switch (mode) {
    case MODE_BALANCE:
      refreshDisplayBalance();
      break;
    case MODE_UNIT_OF_MEASURE:
      refreshDisplayUnitOfMeasure();
      break;
    case MODE_CALIBRATION:
      refreshDisplayCalibration();
      break;
    default: 
      refreshDisplaySplash();
      break;
  }
  
  display.display();
}

void refreshDisplaySplash() {
  Serial.println(F("Splash"));
  /*display.setTextColor(BLACK, WHITE);
  display.print(F("       Splash        "));
  display.setTextColor(WHITE);*/
  
  display.setTextSize(2);
  display.setCursor(16, 0);
  display.print(F("CarbSync"));
  display.setTextSize(3);
  display.setCursor(29, 25);
  display.print(F("4000"));
  display.setTextSize(1);
  display.setCursor(8, 56);
  display.print(F("blog.cuyahoga.co.uk"));
}

void refreshDisplayBalance() {
  Serial.println(F("Balance"));
  display.setTextColor(BLACK, WHITE);
  display.print(F("       Balance       "));
  display.setTextColor(WHITE);
  display.setCursor(0, 17);
  display.print(getUomDesc());
  display.setCursor(0, 25);
  display.print(F("----"));
  
  // Determine the min, max and range of input values
  uint8_t pLow = MAX_PRESSURE;
  uint8_t pHigh = MIN_PRESSURE;
  uint8_t pRange = 0;
  for (byte sensor = 0; sensor < MAP_SENSOR_COUNT; sensor++) 
  {
    pLow = min(pLow, mapSensorReadingsKPa[sensor].getAvg());
    pHigh = max(pHigh, mapSensorReadingsKPa[sensor].getAvg());
  }
  pRange = pHigh - pLow;

  // Display each of the values relative to the current range
  for (byte sensor = 0; sensor < MAP_SENSOR_COUNT; sensor++) {
    renderGaugeRow(sensor, mapSensorReadingsKPa[sensor].getAvg(), pLow, pRange);
  }

  /*int mapKpa;
  for (byte sensor = 0; sensor < MAP_SENSOR_COUNT; sensor++) { 
    Serial.print(F("MAP Sensor: ")); Serial.print(sensor + 1); Serial.print(F("   val: ")); Serial.print(mapSensors[sensor].getCurrentReading()); Serial.print(F("  ")); Serial.print(round(mapSensors[sensor].getCurrentReadingMv())); Serial.print(F(" mV  Pressure: ")); Serial.print(mapSensors[sensor].getPressureKpa()); Serial.println(F(" kPa"));
  }
  Serial.println(F("------------"));*/
  
  /*
  for (byte sensor = 0; sensor < MAP_SENSOR_COUNT; sensor++) { 
    Serial.print((int)mapSensorReadings[sensor].getAvg()); Serial.print(F("  ")); 
    Serial.print((int)mapSensorReadingsMv[sensor].getAvg()); Serial.print(F("mv  "));
    Serial.print((int)mapSensorReadingsKPa[sensor].getAvg());Serial.println(F(" KPa"));
  }  
  Serial.println();
  */
}

void refreshDisplayUnitOfMeasure() {
  Serial.println(F("Unit Of Measure"));
  display.setTextColor(BLACK, WHITE);
  display.print(F("   Unit Of Measure   "));
  display.setTextColor(WHITE);
  
  display.setCursor(20, 33);
  display.setTextSize(4);
  display.print(getUomDesc());  
}

void refreshDisplayCalibration() {
  Serial.println(F("Calibration"));
  display.setTextColor(BLACK, WHITE);
  display.print(F("     Calibration     "));
  display.setTextColor(WHITE);
  display.setCursor(0, 17);
  display.print(F("  Rdg  Off   Value"));
  display.setCursor(0, 25);
  display.print(F("  ---  ---   -----"));

  byte row;
  for (byte sensor = 0; sensor < MAP_SENSOR_COUNT; sensor++) {
    // Map the row number to a Y co-ordinate
    row = (sensor * 8) + 33;
    display.setCursor(0, row);
    display.print(sensor + 1);
    display.setCursor(12, row);
    display.print(mapSensorReadings[sensor].getAvg());
    display.setCursor(43, row);
    display.print(adjusterReadings[sensor]);
    display.setCursor(78, row);
    display.print(formatReadingInUom(mapSensorReadingsKPa[sensor].getAvg(), true));

    //Serial.print(F("Sensor: ")); Serial.print(sensor + 1); Serial.print(F("   val: ")); Serial.print(mapSensorReadings[sensor].getAvg()); Serial.print(F("  Offset: ")); Serial.print(adjusterReadings[sensor]); Serial.print(F("  Pressure: ")); Serial.print(mapSensorReadingsKPa[sensor].getAvg()); Serial.println(F(" kPa"));
  }
}

void renderGaugeRow(byte row, uint8_t val, uint8_t low, uint8_t range) {
  
  // Map the row number to a Y co-ordinate
  row = (row * 8) + 33;
  // Show the current value
  display.setCursor(0, row);
  display.print(formatReadingInUom(val, false));

  // Show a slider representing the current value for the row
  uint8_t rel = map((range == 0 ? 1 : val - low), 0, (range == 0 ? 2 : range), 0, 84);

  display.fillRect(27 + rel, row, 8, 7, WHITE);
  display.fillRect(27 + rel + 10, row, 8, 7, WHITE);

}

float convertKPaToUom(int kpa) {
  switch(uom) {
    case UOM_HGIN:
      Serial.print(atmosphericKPa); Serial.print("  "); Serial.print(kpa); Serial.print("  "); Serial.println((atmosphericKPa - kpa) * 0.295301);
      return (atmosphericKPa - kpa) * 0.295301;
      break;
    case UOM_PSI:
    return kpa * 0.145037738;
      break;
    default:
      return kpa;
  }
}

int convertReadingToKPa(float reading, float vccOffset, float vccMvPerUnit, float inMinMv, float inMaxMv, int outMin, int outMax) {
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
  return map(reading, round(((inMinMv * vccOffset) / vccMvPerUnit) - 1), round(((inMaxMv * vccOffset) / vccMvPerUnit) - 1), outMin, outMax);
}

void handleButtonAction() 
{                
  if ((millis() - lastButtonActionDebounceTime) > DEBOUNCE_INTERVAL) 
  {
    lastButtonActionDebounceTime = millis();
    switch (mode) {
      case MODE_BALANCE:
        break;
      case MODE_UNIT_OF_MEASURE:
        // Change the current Unit Of Measure
        if (++uom == UOM_COUNT) {
          // Cycle back round to the first UOM
          uom = 0;
        }
        break;
      case MODE_CALIBRATION:
        break;
      default: 
        break;
    }
  }  
}

void handleButtonMode() 
{                
  // Change the current mode
  if ((millis() - lastButtonModeDebounceTime) > DEBOUNCE_INTERVAL) 
  {
    lastButtonModeDebounceTime = millis();
    if (++mode == MODE_COUNT) {
      // Cycle back round to the balance mode
      mode = MODE_BALANCE;
    }
  }  
}


