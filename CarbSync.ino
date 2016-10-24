/*********************************************************************

  Display 4 sliders representing values that go up and down, where
  each slider position represents the current value relative to the 
  lowest and highest of all four current values
  
*********************************************************************/

#include <Wire.h>
#include <Adafruit_GFX.h>        // https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h>    // https://github.com/adafruit/Adafruit_SSD1306
#include <Timer.h>               // https://github.com/JChristensen/Timer

#define OLED_DC 11
#define OLED_CS 12
#define OLED_CLK 10
#define OLED_MOSI 9
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
Timer t;

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

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

void setup()   {                
  Serial.begin(9600);

  // int tickEvent = t.every(20, refreshGaugeDisplay);
    
  // Set up the 128x32 OLED
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  mode = MODE_SUMMARY;
}

void loop() {
  t.update();
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
  display.clearDisplay();
  for (uint8_t i = 0; i < CARB_INPUTS; i++) {
    renderGaugeRow(i, carbPressure[i], pLow, pRange);
  }
  display.display();

  if (TEST_MODE)
  {
    // If all values match, hang around for a while
    if (pRange == 0) {
      delay(10000);
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
  display.setCursor(0, row);
  display.print(val);
  // Show a slider representing the current value for the row
  uint8_t rel = map((range == 0 ? 1 : val - low), 0, (range == 0 ? 2 : range), 0, 90);
  display.fillRect(21 + rel, row, 8, 7, WHITE);
  display.fillRect(21 + rel + 10, row, 8, 7, WHITE);  
}

