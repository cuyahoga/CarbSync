/*********************************************************************

  Display 4 sliders representing values that go up and down, where
  each slider position represents the current value relative to the 
  lowest and highest of all four current values
  
*********************************************************************/

#include <Wire.h>
#include <Timer.h>               // https://github.com/JChristensen/Timer
#include "RunningAverage.h"      // http://playground.arduino.cc/Main/RunningAverage

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
  #if (SSD1306_LCDHEIGHT != 64)
  #error("Height incorrect, please fix Adafruit_SSD1306.h!");
  #endif

  #define LCD_REFRESH   150


Timer t;

#define MODE_SUMMARY 1
#define MODE_GAUGE   2

#define CARB_INPUTS 4
#define MIN_PRESSURE 30
#define MAX_PRESSURE 70

#define TEST_MODE true

//uint8_t carbPressure[CARB_INPUTS] = {0, 0, 0, 0};
uint8_t carbPressure[CARB_INPUTS] = {MIN_PRESSURE, MAX_PRESSURE, MIN_PRESSURE, MAX_PRESSURE};
uint8_t adj[CARB_INPUTS] = {2, 4, 6, 8};

uint8_t mode;
char   buffer[10]; 

void setup()   {                
  Serial.begin(9600);

  int tickEvent = t.every(LCD_REFRESH, refreshGaugeDisplay);
    
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

  mode = MODE_SUMMARY;
}

void loop() {
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

void refreshGaugeDisplay() 
{
  if (TEST_MODE)
  {
    generateGaugeTestData();
  }
  
  // Determine the min, max and range of input values
  uint8_t pLow = MAX_PRESSURE;
  uint8_t pHigh = MIN_PRESSURE;
  uint8_t pRange = 0;
  for (uint8_t i = 0; i < CARB_INPUTS; i++) 
  {
    pLow = min(pLow, carbPressure[i]);
    pHigh = max(pHigh, carbPressure[i]);
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
  for (uint8_t i = 0; i < CARB_INPUTS; i++) {
    renderGaugeRow(i, carbPressure[i], pLow, pRange);
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

void renderGaugeRow(uint8_t row, uint8_t val, uint8_t low, uint8_t range) {
  
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


