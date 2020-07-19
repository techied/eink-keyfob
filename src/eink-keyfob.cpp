/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#line 1 "c:/Users/slurp/eink-keyfob/src/eink-keyfob.ino"
// Display Library example for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: these e-papers require 3.3V supply AND data lines!
//
// Display Library based on Demo Example from Good Display: http://www.e-paper-display.com/download_list/downloadcategoryid=34&isMode=false.html
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2_PP
//
// Supporting Arduino Forum Topics:
// Waveshare e-paper displays with SPI: http://forum.arduino.cc/index.php?topic=487007.0
// Good Dispay ePaper for Arduino: https://forum.arduino.cc/index.php?topic=436411.0
// mapping suggestion from Waveshare SPI e-Paper to Particle Photon
// A5 MOSI
// A4 MISO
// A3 SCK
// A2 SS
// BUSY -> D4, RST -> A0, DC -> A1, CS -> A2, CLK -> A3, DIN -> A5, GND -> GND, 3.3V -> 3.3V
// NOTE: it looks like MISO can't be used as general input pin for BUSY.
// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#include <Adafruit_GFX_RK.h>
#include <GxEPD2_PP.h>
#include <bitmaps.h>
#define ENABLE_GxEPD2_GFX 0
#include <Arduino.h>
void setup();
void loop();
void initDisplay();
void drawStats();
void wipeScreen();
void showBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool partial);
int get_charging_state();
#line 29 "c:/Users/slurp/eink-keyfob/src/eink-keyfob.ino"
#include <Adafruit_GFX.h>
#include <FreeSans9pt7b.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
GxEPD2_BW<GxEPD2_290_T5, GxEPD2_290_T5::HEIGHT> display(GxEPD2_290_T5(/*CS=*/ A2, /*DC=*/A1, /*RST=*/A0, /*BUSY=*/D4));

#include "bitmaps/Bitmaps128x296.h"
unsigned long firstAvailable = 0;
int counter;
SystemSleepConfiguration config;
void setup()
{
  Serial.begin(115200);
  delay(1000); // give serial port time to open
  Serial.println("setup called");
  config.mode(SystemSleepMode::STOP)
      .gpio(D0, RISING)
      .duration(5min)
      .flag(SystemSleepFlag::WAIT_CLOUD)
      .network(NETWORK_INTERFACE_CELLULAR);
  display.init(0);
  Time.zone(-8);
  Time.beginDST();
  initDisplay();
}
int loopCount = 0;
void loop()
{
  bool cellReady = Cellular.ready();
  bool cloudReady = Particle.connected();
  Serial.printlnf("cell=%s cloud=%s counter=%d", (cellReady ? "on" : "off"), (cloudReady ? "on" : "off"), counter++);

  if (cellReady && cloudReady)
  {
    if (firstAvailable == 0)
    {
      firstAvailable = millis();
    }
    if (millis() - firstAvailable > 5000)
    {
      // After we've been up for 5 seconds, go to sleep

      Serial.println("calling System.sleep");
      drawStats();
      loopCount++;
      if(loopCount==4){
        Particle.publishVitals(0);
        loopCount = 0;
      }
      // This delay is here so the serial print above goes out before going to sleep
      System.sleep(config);
      

      // In this mode, setup() is not called again. Sometimes, but not always, the Serial port does
      // not restore properly. This solves the problem.
      Serial.begin(115200);
      // display.init(115200);

      // In this mode,after the sleep period is done, execution continues where we left off,
      // with variables intact.
      Serial.println("returned from sleep");

      // Important! If you don't reset this, you'll immediately go back to sleep, since variables
      // are preserved.
      firstAvailable = 0;
    }
  }
  else
  {
    firstAvailable = 0;
  }

  delay(1000);
}

void initDisplay()
{
  // wipeScreen(); // drawStats in full screen mode wipes the display anyways
  drawStats();
}

void drawStats()
{
  // for Argon & Xenon: int battPercentage = analogRead(BATT) * 0.0011224 / 4.7 * 100;
  // initDisplay();
  int charging_state = get_charging_state();
  Serial.printlnf("Charging state: %d", charging_state);
  int rssi = Cellular.RSSI().rssi;
  int strength = map(rssi, -131, -51, 0, 4);
  String time = Time.format(Time.now(), "%b %d %I:%M %p");
  const char netDisabled[] = "x";
  char percentage[64];
  snprintf(percentage, sizeof(percentage), "%i%%", (int) System.batteryCharge());
  display.setRotation(1);
  display.setFont(&FreeSans9pt7b);
  display.setTextColor(GxEPD_BLACK);
  // display.setPartialWindow(0, 0, 296, 16); // Partial window has dark spot issues in sunlight for some reason...
  display.setFullWindow();
  display.firstPage();
  const int beginX = 235;
  const int beginY = 3;
  do
  {
    // battery %
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(259, 12);
    display.print(percentage);

    // date & time
    display.setCursor(0, 12);
    display.print(time);

    // delineator
    display.fillRect(0, 15, 296, 1, GxEPD_BLACK);
    
    // battery charge status icon
    switch (charging_state){
      case 0:
        Serial.println("Charged");
        display.drawInvertedBitmap(beginX - 16, 0, charged, 15, 15, GxEPD_BLACK);
        break;
      case 1:
        Serial.println("Charging");
        display.drawInvertedBitmap(beginX - 16, 0, charging, 15, 15, GxEPD_BLACK);
        break;
      case 4:
        Serial.println("Discharging");
        break;
      default:
        Serial.println("Battery fault");
        display.drawInvertedBitmap(beginX - 16, 0, battery_fault, 15, 15, GxEPD_BLACK);
        break;
    }

    // cell strength icon
    if (rssi == 0)
    {
      display.setCursor(beginX, beginY + 10);
      display.print(netDisabled);
      display.fillRect(beginX + 8, beginY + 4, 2, 6, GxEPD_BLACK);
      display.fillRect(beginX + 12, beginY + 2, 2, 8, GxEPD_BLACK);
      display.fillRect(beginX + 16, beginY, 2, 10, GxEPD_BLACK);
      continue;
    }
    display.fillRect(beginX, beginY + 8, 2, 2, GxEPD_BLACK);
    if (strength >= 1)
    {
      display.fillRect(beginX + 4, beginY + 6, 2, 4, GxEPD_BLACK);
    }
    if (strength >= 2)
    {
      display.fillRect(beginX + 8, beginY + 4, 2, 6, GxEPD_BLACK);
    }
    if (strength >= 3)
    {
      display.fillRect(beginX + 12, beginY + 2, 2, 8, GxEPD_BLACK);
    }
    if (strength == 4)
    {
      display.fillRect(beginX + 16, beginY, 2, 10, GxEPD_BLACK);
    }
  } while (display.nextPage());
  
  display.hibernate(); // Keep the display from developing dark spots when in sunlight.
}
void wipeScreen()
{
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());
}

void showBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool partial)
{
  //Serial.println("showBox");
  display.setRotation(1);
  if (partial)
  {
    display.setPartialWindow(x, y, w, h);
  }
  else
  {
    display.setFullWindow();
  }
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.fillRect(x, y, w, h, GxEPD_BLACK);
  } while (display.nextPage());
  //Serial.println("showBox done");
}

int get_charging_state() {
    PMIC power(true);
    FuelGauge fuel(true);
    power.begin();

    // In order to read the current fault status, the host has to read REG09 two times
    // consecutively. The 1st reads fault register status from the last read and the 2nd
    // reads the current fault register status.
    const uint8_t curFault = power.getFault();

    const uint8_t status = power.getSystemStatus();

    int state = -1; // unknown

    // Deduce current battery state
    const uint8_t chrg_stat = (status >> 4) & 0b11;
    if (chrg_stat) {
        // Charging or charged
        if (chrg_stat == 0b11) {
            state = 0; // charged
        } else {
            state = 1; // charging
        }
    } else {
        // For now we only know that the battery is not charging
        state = 2; // not charging
        // Now we need to deduce whether it is NOT_CHARGING, DISCHARGING, or in a FAULT state
        // const uint8_t chrg_fault = (curFault >> 4) & 0b11;
        const uint8_t bat_fault = (curFault >> 3) & 0b01;
        // const uint8_t ntc_fault = curFault & 0b111;
        const uint8_t pwr_good = (status >> 2) & 0b01;
        if (bat_fault) {
            state = 3; // battery fault
        } else if (!pwr_good) {
            state = 4; // battery discharging
        }
    }
    return state;
}

#include "Particle.h"
SYSTEM_THREAD(ENABLED);
// SYSTEM_MODE(SEMI_AUTOMATIC);