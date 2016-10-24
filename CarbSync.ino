/*********************************************************************
 *
 * CarbSync 4000
 *
 * http://blog.cuyahoga.co.uk/carbsync
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

// ==========================
// Display modes
// ==========================
#define MODE_SPLASH            0
#define MODE_MONITOR           1
#define MODE_BALANCE           2
#define MODE_UNIT_OF_MEASURE   3
#define MODE_CALIBRATION       4
#define MODE_COUNT             5

// ==========================
// Balance options
// ==========================
#define BALANCE_1234  0
#define BALANCE_12    1
#define BALANCE_34    2
#define BALANCE_COUNT 3

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
#define ADJUSTER_INTERVAL    50    // Millis between updating adjuster values
#define DEBOUNCE_INTERVAL    200   // Millis for debouncing
#define DISPLAY_INTERVAL     250   // Millis between display refreshes
#define MAP_SENSOR_COUNT     4     // Number of MAP sensors
#define MAP_SENSOR_INTERVAL  20    // Millis between sensor readings
#define MIN_PRESSURE         50
#define MAX_PRESSURE         120



byte mode = MODE_SPLASH;
byte balance = BALANCE_1234;
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
byte atmosphericKPa = 0;


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

// ===============================================================
// Functions
// ===============================================================

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

float getMvPerUnit(int vccMv) {
  return vccMv / 1023.0;
}

String getUomDesc() {
  
  String desc[UOM_COUNT] = {"KPa", "HgIn", "PSI"};
  return desc[uom];
}

void handleButtonAction() 
{                
  if ((millis() - lastButtonActionDebounceTime) > DEBOUNCE_INTERVAL) 
  {
    lastButtonActionDebounceTime = millis();
    switch (mode) {
      case MODE_BALANCE:
        // Change the current Balance display
        if (++balance == BALANCE_COUNT) {
          // Cycle back round to the first UOM
          balance = 0;
        }
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
      // Cycle back round to the Monitor mode
      mode = MODE_MONITOR;
    }
  }  
}

void readAdjusters() {
    
  int offset;
  for (byte adjuster = 0; adjuster < MAP_SENSOR_COUNT; adjuster++) { 
    adjusterReadings[adjuster] = ((adjusters[adjuster].getSector() - ADJUSTER_SECTORS) * -1) - (ADJUSTER_SECTORS / 2);
  }
  
}

void readAtmosphericPressure() {
  
   // Establish power specification and then set the baseline atmospheric pressure value
  int vccMv = readVcc(VCC_ADJUSTMENT);
  atmosphericKPa = convertReadingToKPa(analogRead(mapSensorPins[0]) + adjusterReadings[0], (float)vccMv / MAP_SENSOR_REFERENCE_MV, vccMv / 1023.0, MAP_SENSOR_MIN_MV, MAP_SENSOR_MAX_MV, MAP_SENSOR_MIN_KPA, MAP_SENSOR_MAX_KPA);
}

void readMAPSensors() {
  
  //unsigned long time = millis();
  
  // Establish power specification
  int vccMv = readVcc(VCC_ADJUSTMENT);
  //Serial.print(F("VCC: ")); Serial.print(vccMv); Serial.println(F(" mV"));

  int currentReading;
  float currentReadingMv;
  float vccOffset = (float)vccMv / MAP_SENSOR_REFERENCE_MV;
  float currentReadingKPa;

  for (byte sensor = 0; sensor < MAP_SENSOR_COUNT; sensor++) { 
    currentReading = analogRead(mapSensorPins[sensor]) + adjusterReadings[sensor];
    currentReadingMv = currentReading * (getMvPerUnit(vccMv));
    currentReadingKPa = convertReadingToKPa(currentReading, vccOffset, getMvPerUnit(vccMv), MAP_SENSOR_MIN_MV, MAP_SENSOR_MAX_MV, MAP_SENSOR_MIN_KPA, MAP_SENSOR_MAX_KPA);
    
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
    case MODE_MONITOR:
      refreshDisplayMonitor();
      break;
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

void refreshDisplayBalance() {
  Serial.println(F("Balance"));
  display.setTextColor(BLACK, WHITE);
  display.print(F("       Balance       "));
  display.setTextColor(WHITE);
  display.setCursor(0, 17);
  display.print(getUomDesc());
  if (!balance == BALANCE_1234) {
    display.setCursor(40, 17);
    display.print(F("Ports "));
    if (balance == BALANCE_12) {
      display.print(F("1 & 2"));
    } else {
      display.print(F("3 & 4"));
    }
  }
  display.setCursor(0, 25);
  display.print(F("----"));
  if (!balance == BALANCE_1234) {
    display.setCursor(40, 25);
    display.print(F("-----------"));
  }
  
  // Determine the min, max and range of input values
  int pLow = 1023;
  int pHigh = 0;
  int pRange = 0;
  for (byte sensor = (balance == BALANCE_34 ? 2 : 0); sensor < (balance == BALANCE_12 ? 2 : MAP_SENSOR_COUNT); sensor++) 
  {
    pLow = min(pLow, mapSensorReadings[sensor].getAvg());
    pHigh = max(pHigh, mapSensorReadings[sensor].getAvg());
  }
  pRange = pHigh - pLow;

  // Display each of the values relative to the current range
  renderGauge(pLow, pRange);
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
  }
}

void refreshDisplayMonitor() {
  Serial.println(F("Monitor"));
  display.setTextColor(BLACK, WHITE);
  display.print(F("       Monitor       "));
  display.setTextColor(WHITE);
  display.setCursor(0, 17);
  display.print(getUomDesc());
  display.setCursor(0, 25);
  display.print(F("----"));
  
  // Display each of the values relative to the full range of the sensor (plus/minus adjusters)
  int vccMv = readVcc(VCC_ADJUSTMENT);
  float offset = vccMv / MAP_SENSOR_REFERENCE_MV;
  int low = (MAP_SENSOR_MIN_MV * offset) / getMvPerUnit(vccMv) - (ADJUSTER_SECTORS / 2);
  renderGauge(low, (((MAP_SENSOR_MAX_MV * offset) / getMvPerUnit(vccMv)) + (ADJUSTER_SECTORS / 2)) - low);
}

void refreshDisplaySplash() {
  Serial.println(F("Splash"));
  
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

void refreshDisplayUnitOfMeasure() {
  Serial.println(F("Unit Of Measure"));
  display.setTextColor(BLACK, WHITE);
  display.print(F("   Unit Of Measure   "));
  display.setTextColor(WHITE);
  
  display.setCursor(20, 33);
  display.setTextSize(4);
  display.print(getUomDesc());  
}

void renderGauge(int pLow, int pRange) {
  // Display each of the values relative to the current range
  int row = 0;
  for (byte sensor = (balance == BALANCE_34 ? 2 : 0); sensor < (balance == BALANCE_12 ? 2 : MAP_SENSOR_COUNT); sensor++) {
    renderGaugeRow(row++, mapSensorReadingsKPa[sensor].getAvg(), mapSensorReadings[sensor].getAvg(), pLow, pRange);
  }
}

void renderGaugeRow(byte row, int pressure, int val, int low, int range) {
  
  // Map the row number to a Y co-ordinate
  row = (row * (balance == BALANCE_1234 ? 8 : 16)) + 33;
  // Show the current value
  display.setCursor(0, row);
  if (!balance == BALANCE_1234) {
    display.setTextSize(2);
  }
  display.print(formatReadingInUom(pressure, false));

  // Scale the range
  int scale = (range < 30 ? roundup(range, 10) : (range < 100 ? roundup(range, 25) : (range < 500 ? roundup(range, 100) : range)));
  if (scale != range) {
    low -= ((scale - range) / 2);
  }

  // Show a slider representing the current value for the row
  byte rel = (range < 3 ? 42 : map(val, low, low + scale, 0, (!balance == BALANCE_1234 && uom == UOM_PSI ? 64 : 84)));
  display.fillRect(27 + (!balance == BALANCE_1234 && uom == UOM_PSI ? 20 : 0) + rel, row, 8, (balance == BALANCE_1234 ? 7 : 14), WHITE);
  display.fillRect(27 + (!balance == BALANCE_1234 && uom == UOM_PSI ? 20 : 0) + rel + 10, row, 8, (balance == BALANCE_1234 ? 7 : 14), WHITE);
}

int roundup(int numToRound, int multiple) 
{ 
  if(multiple == 0) 
  { 
    return numToRound;  
  } 

  int remainder = numToRound % multiple;
  if (remainder == 0)
    return numToRound;
  return numToRound + multiple - remainder;
}

