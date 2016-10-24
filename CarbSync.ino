/*********************************************************************

  Display 4 sliders representing values that go up and down, where
  each slider position represents the current value relative to the 
  lowest and highest of all four current values
  
*********************************************************************/
#include <SPI.h>
#include <Wire.h>
#include <Button.h>              // https://github.com/JChristensen/Button
#include <Timer.h>               // https://github.com/JChristensen/Timer
#include <RunningAverage.h>      // http://playground.arduino.cc/Main/RunningAverage
//#include <Vcc.h>
#include <Potentiometer.h>       // http://playground.arduino.cc/Code/Potentiometer
#include "Bosch0261230043.h"

RunningAverage lcdRefreshRate(10);

// ===============================================================
// Adafruit ST7565 128x64 GLCD
// ===============================================================
/*
  #define SCREEN_ST7565
  #include "JeeLib.h"              // https://github.com/jcw/jeelib
  #include <GLCD_ST7565.h>         // https://github.com/cuyahoga/glcdlib
  #include "utility/font_clR6x8.h"
  #define PIN_RED       3  // Red cathode
  #define PIN_GREEN     5  // Green cathode
  #define PIN_BLUE      6  // Blue cathode
  #define LCD_BLACK     1
  #define LCD_WHITE     0
  #define LCD_RED       0, 255, 255
  #define LCD_GREEN     255, 0, 255
  #define LCD_BLUE      255, 255, 0
  #define LCD_ORANGE    0,  90, 255
  GLCD_ST7565           glcd;
*/  
// ===============================================================
// Adafruit SSD1306 128x32 OLED
// ===============================================================

  #define SCREEN_SSD1306
  #include <Adafruit_GFX.h>        // https://github.com/adafruit/Adafruit-GFX-Library
  #include <Adafruit_SSD1306.h>    // https://github.com/adafruit/Adafruit_SSD1306
  #define OLED_DC         11
  #define OLED_CS         12
  #define OLED_CLK        10
  #define OLED_MOSI       9
  #define OLED_RESET      13
  Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
  #if (SSD1306_LCDHEIGHT != 32)
  #error("Height incorrect, please fix Adafruit_SSD1306.h!");
  #endif

// ===============================================================
// Bosch 0 261 230 043 MAP sensors
// ===============================================================
Bosch0261230043 mapSensors[] = {Bosch0261230043("MAP 1", A0), Bosch0261230043("MAP 2", A1), Bosch0261230043("MAP 3", A2), Bosch0261230043("MAP 4", A3)};
#define MAP_SENSOR_COUNT 4
#define MAP_SENSOR_MS    100
int vccMv = 0;

// Buttons
#define BUTTON_MODE     2      // Selects the operating mode
#define BUTTON_ACTION   3      // Triggers an action within a mode
#define PULLUP true            // Use the internal pullup resistor.
#define INVERT true            // Since the pullup resistor will keep the pin high unless the switch is closed, this is negative logic, i.e. a high state means the button is NOT pressed. (Assuming a normally open switch.)
#define DEBOUNCE_MS 20         // A debounce time of 20 milliseconds usually works well for tactile button switches.

Button btnMode(BUTTON_MODE, PULLUP, INVERT, DEBOUNCE_MS);
Button btnAction(BUTTON_ACTION, PULLUP, INVERT, DEBOUNCE_MS);

// Potentiometers
//const byte adjusters[4] = {A4, A5, A6, A7};
#define ADJUSTER_SECTORS 70
#define ADJUSTER_OFFSET  35
Potentiometer adjusters[4] = {Potentiometer(A4, ADJUSTER_SECTORS), Potentiometer(A5, ADJUSTER_SECTORS), Potentiometer(A6, ADJUSTER_SECTORS), Potentiometer(A7, ADJUSTER_SECTORS)};

Timer t;
int tickDisplay, tickButtons, tickSensors;

#define MODE_SPLASH            0
#define MODE_BALANCE           1
#define MODE_AUTO_CALIBRATE    2
#define MODE_MANUAL_CALIBRATE  3
#define MODE_COUNT             4
byte mode = MODE_SPLASH;

#define PIN_BALANCE           5
#define PIN_AUTO_CALIBRATE    7
#define PIN_MANUAL_CALIBRATE  8

#define DISPLAY_REFRESH   500

#define CARB_INPUTS 4
#define MIN_PRESSURE 50
#define MAX_PRESSURE 120

#define TEST_MODE false

//uint8_t carbPressure[CARB_INPUTS] = {0, 0, 0, 0};
uint8_t carbPressure[CARB_INPUTS] = {MIN_PRESSURE, MAX_PRESSURE, MIN_PRESSURE, MAX_PRESSURE};
uint8_t adj[CARB_INPUTS] = {2, 4, 6, 8};

char   buffer[60]; 

void setup()   {                
  Serial.begin(9600);
//  while (!Serial) {
//    ; // wait for serial port to connect. Needed for Leonardo only
//  }

  // Set the pressure sensors to smooth readings
  for (byte sensor = 0; sensor < MAP_SENSOR_COUNT; sensor++) { 
    mapSensors[sensor].setSampling(5, 10);
  }
    
#if defined(SCREEN_SSD1306)
  // Set up the 128x32 OLED
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
#endif
#if defined(SCREEN_ST7565)
  glcd.begin(0x19);
  glcd.setFont(font_clR6x8);

  // Initialise the backlight
  pinMode(PIN_RED, OUTPUT);   
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  setBacklight(LCD_BLUE);
#endif

  pinMode(PIN_BALANCE, OUTPUT);
  pinMode(PIN_AUTO_CALIBRATE, OUTPUT);
  pinMode(PIN_MANUAL_CALIBRATE, OUTPUT);

  tickButtons = t.every(DEBOUNCE_MS * 2, readButtons);
  tickDisplay = t.every(DISPLAY_REFRESH, refreshDisplay);
  tickSensors = t.every(MAP_SENSOR_MS, readMAPSensors);
    
}

void loop() {
  
  // See if it's time to refresh the display
  t.update();
  
  /*
  long t = millis();
  refreshGaugeDisplay();
  uint8_t refresh = millis() - t;
  Serial.print("refresh: ");Serial.print(refresh);Serial.print("  avg : ");Serial.print(lcdRefreshRate.getAverage() * 1.1);Serial.print(" : ");Serial.println((int)((lcdRefreshRate.getAverage() * 1.1) + 0.5f));
  lcdRefreshRate.addValue(refresh);
  //delay(10);
  delay((int)((lcdRefreshRate.getAverage() * 1.1) + 0.5f));
  */
}

void readButtons() {
  btnMode.read(); 
  if (btnMode.wasPressed()) {
    // Increment MODE with each button press
    if (++mode == MODE_COUNT) {
      // Cycle back round to the balance mode
      mode = MODE_BALANCE;
    }
  }
//  Serial.print(F("Mode : ")); Serial.println(mode);
  
  btnAction.read(); 
  
}

void refreshLEDs() {
  digitalWrite(PIN_BALANCE, LOW);
  digitalWrite(PIN_AUTO_CALIBRATE, LOW);
  digitalWrite(PIN_MANUAL_CALIBRATE, LOW);
  
  switch(mode) {
    case MODE_BALANCE:
      digitalWrite(PIN_BALANCE, HIGH);
      break;
    case MODE_AUTO_CALIBRATE:
      digitalWrite(PIN_AUTO_CALIBRATE, HIGH);
      break;
    case MODE_MANUAL_CALIBRATE:
      digitalWrite(PIN_MANUAL_CALIBRATE, HIGH);
      break;
    default:
      // Do nothing
      break;
  }
} 

void refreshDisplay() {
  
#if defined(SCREEN_SSD1306)
  refreshLEDs();
  display.clearDisplay();
#endif  
  
  display.setCursor(0, 1);
  switch (mode) {
    case MODE_BALANCE:
      //display.print("Balance");
      refreshGaugeDisplay();
      break;
    case MODE_AUTO_CALIBRATE:
      display.print("Auto Calibration");
      break;
    case MODE_MANUAL_CALIBRATE:
      //display.print("Manual Calibration");
      refreshManualCalibrationDisplay();
      break;
    default: 
      display.print("Splash");
      break;
  }

#if defined(SCREEN_SSD1306)
  display.display();
#endif
}

void refreshManualCalibrationDisplay() {
  
  byte row;
  
#if defined(SCREEN_SSD1306)
  display.clearDisplay();
#endif  
  
  for (byte sensor = 0; sensor < MAP_SENSOR_COUNT; sensor++) {
    // Map the row number to a Y co-ordinate
    row = (sensor * 8) + 1;
#if defined(SCREEN_SSD1306)
    display.setCursor(0, row);
//    display.print("Port :");
//    display.setCursor(22, row);
    display.print(sensor + 1);
    display.setCursor(15, row);
//    display.print("Offset:");
//    display.setCursor(62, row);
    display.print(mapSensors[sensor].getCurrentReading() + mapSensors[sensor].getPressureOffset());
    display.setCursor(70, row);
    display.print(mapSensors[sensor].getPressureOffset());
    display.setCursor(90, row);
    display.print(mapSensors[sensor].getPressureKpa());
//    display.print("Adj val:");
//    display.setCursor(102, row);
//    display.print(mapSensors[sensor].getPressureKpa());
//    dtostrf(f_val,7, 3, outstr)
    //sprintf(buffer, "%d: Offset: %d  Val :%d  Kpa: %d", sensor + 1, mapSensors[sensor].getPressureOffset(), mapSensors[sensor].getCurrentReading(), mapSensors[sensor].getPressureKpa());
    //display.print(buffer);
    //Serial.println(buffer);
    
    Serial.print(F("MAP Sensor: ")); Serial.print(sensor + 1); Serial.print(F("   val: ")); Serial.print(mapSensors[sensor].getCurrentReading() + mapSensors[sensor].getPressureOffset()); Serial.print(F("  Offset: ")); Serial.print(mapSensors[sensor].getPressureOffset()); Serial.print(F("  Pressure: ")); Serial.print(mapSensors[sensor].getPressureKpa()); Serial.println(F(" kPa"));

#endif
  }

  
#if defined(SCREEN_SSD1306)
  display.display();
#endif
  
}

void refreshGaugeDisplay() 
{
  // Determine the min, max and range of input values
  uint8_t pLow = MAX_PRESSURE;
  uint8_t pHigh = MIN_PRESSURE;
  uint8_t pRange = 0;
  for (byte sensor = 0; sensor < MAP_SENSOR_COUNT; sensor++) 
  {
    pLow = min(pLow, mapSensors[sensor].getPressureKpa());
    pHigh = max(pHigh, mapSensors[sensor].getPressureKpa());
  }
  pRange = pHigh - pLow;

  // Display each of the values relative to the current range
#if defined(SCREEN_SSD1306)
  display.clearDisplay();
#endif  
#if defined(SCREEN_ST7565)
    //glcd.fillRect(0, 0, 128, 32, LCD_WHITE);
    glcd.clear();
    sprintf(buffer, "Range: %d", pRange);
    glcd.drawString(2, 41, buffer);
#endif
  for (byte sensor = 0; sensor < MAP_SENSOR_COUNT; sensor++) {
    renderGaugeRow(sensor, mapSensors[sensor].getPressureKpa(), pLow, pRange);
  }
#if defined(SCREEN_SSD1306)
  display.display();
#endif
#if defined(SCREEN_ST7565)
  glcd.refresh();
  if (pRange > 10) {
    setBacklight(LCD_BLUE);
  } else if (pRange > 5) {
    setBacklight(LCD_ORANGE);
  } else {
    setBacklight(LCD_GREEN);
  }
#endif

  int mapKpa;
  for (byte sensor = 0; sensor < MAP_SENSOR_COUNT; sensor++) { 
    Serial.print(F("MAP Sensor: ")); Serial.print(sensor + 1); Serial.print(F("   val: ")); Serial.print(mapSensors[sensor].getCurrentReading()); Serial.print(F("  ")); Serial.print(round(mapSensors[sensor].getCurrentReadingMv())); Serial.print(F(" mV  Pressure: ")); Serial.print(mapSensors[sensor].getPressureKpa()); Serial.println(F(" kPa"));
  }
  Serial.println(F("------------"));

  if (TEST_MODE)
  {
    // If all values match, hang around for a while
    if (pRange <= 10) {
      delay(1000);
    }
  }
}

void generateGaugeTestData() {
  // Adjust the current values of each gauge row, then determine min, max and range of values
  for (uint8_t i = 0; i < CARB_INPUTS; i++) {
    if (carbPressure[i] > MAX_PRESSURE || carbPressure[i] < MIN_PRESSURE) {
      adj[i] = adj[i] * -1;
    }
    carbPressure[i] = carbPressure[i] + adj[i];
  }
}

void readMAPSensors() {
  // Establish power specification
  vccMv = readVcc();
  Serial.print(F("VCC: ")); Serial.print(vccMv); Serial.println(F(" mV"));

  if (TEST_MODE)
  {
    generateGaugeTestData();
  } else {
    // Bosch MAP sensors
    readAdjusters();
    for (byte sensor = 0; sensor < MAP_SENSOR_COUNT; sensor++) { 
      mapSensors[sensor].readPressureKpa(vccMv);
    }
  }
}

void readAdjusters() {
    
  int offset;
  for (byte adjuster = 0; adjuster < MAP_SENSOR_COUNT; adjuster++) { 
    offset = ((adjusters[adjuster].getSector() - ADJUSTER_SECTORS) * -1) - ADJUSTER_OFFSET;
    mapSensors[adjuster].setPressureOffset(offset);
  }
  
}

void renderGaugeRow(byte row, uint8_t val, uint8_t low, uint8_t range) {
  
  // Map the row number to a Y co-ordinate
  row = (row * 8) + 1;
  // Show the current value
#if defined(SCREEN_SSD1306)
  display.setCursor(0, row);
  display.print(val);
#endif
#if defined(SCREEN_ST7565)
  sprintf(buffer, "%d", val);
  glcd.drawString(2, row, buffer);
#endif
  // Show a slider representing the current value for the row
  uint8_t rel = map((range == 0 ? 1 : val - low), 0, (range == 0 ? 2 : range), 0, 90);
#if defined(SCREEN_SSD1306)
  display.fillRect(21 + rel, row, 8, 7, WHITE);
  display.fillRect(21 + rel + 10, row, 8, 7, WHITE);
#endif
#if defined(SCREEN_ST7565)
  glcd.fillRect(21 + rel, row, 8, 7, LCD_BLACK);
  glcd.fillRect(21 + rel + 10, row, 8, 7, LCD_BLACK);
#endif
}

void setBacklight(uint8_t r, uint8_t g, uint8_t b) {
#if defined(SCREEN_ST7565)
  analogWrite(PIN_RED,   r); 
  analogWrite(PIN_GREEN, g);
  analogWrite(PIN_BLUE,  b);
#endif
}

long readVcc() {
  // http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  
 
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
 
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both
 
  long result = (high<<8) | low;
 
  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

