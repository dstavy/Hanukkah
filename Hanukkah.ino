/*
  My Take
  Digital Hanukkah that react to wind (blow). 

  Modified slightly from the original by John Keefe
  in Feburary 2015 to add "candle" and button code.

  Details at http://johnkeefe.net/make-every-week-wind-sensor-candle

  Original code at https://github.com/moderndevice/Wind_Sensor

  Modern Device Wind Sensor Sketch for Rev C Wind Sensor
  This sketch is only valid if the wind sensor if powered from
  a regulated 5 volt supply. An Arduino or Modern Device BBB, RBBB
  powered from an external power supply should work fine. Powering from
  USB will also work but will be slightly less accurate in our experience.

  When using an Arduino to power the sensor, an external power supply is better. Most Arduinos have a
  polyfuse which protects the USB line. This fuse has enough resistance to reduce the voltage
  available to around 4.7 to 4.85 volts, depending on the current draw.

  The sketch uses the on-chip temperature sensing thermistor to compensate the sensor
  for changes in ambient temperature. Because the thermistor is just configured as a
  voltage divider, the voltage will change with supply voltage. This is why the
  sketch depends upon a regulated five volt supply.

  Other calibrations could be developed for different sensor supply voltages, but would require
  gathering data for those alternate voltages, or compensating the ratio.

  Hardware Setup:
  Wind Sensor Signals    Arduino
  GND                    GND
  +V                     5V
  RV                     A1    // modify the definitions below to use other pins
  TMP                    A0    // modify the definitions below to use other pins


  Paul Badger 2014

  Hardware setup:
  Wind Sensor is powered from a regulated five volt source.
  RV pin and TMP pin are connected to analog innputs.

*/

#include <Time.h>
#include <TimeAlarms.h>
#include "RTClib.h"

#include "HanukkahDates.h"

#define analogPinForRV 1  // change to pins you the analog pins are using
#define analogPinForTMP 0
#define NUM_SENSORS 4
#define MAX_LEDS 8
#define ON_HOUR 17   // start hour
#define OFF_HOUR 23  // end hour

RTC_DS1307 RTC;

// to calibrate your sensor, put a glass over it, but the sensor should not be
// touching the desktop surface however.
// adjust the zeroWindAdjustment until your sensor reads about zero with the glass over it.
// (John Keefe note: I didn't do any of that for the candle project)

const float zeroWindAdjustment = .2;  // negative numbers yield smaller wind speeds and vice versa.

int TMP_Therm_ADunits;  //temp thermistor value from wind sensor
float RV_Wind_ADunits;  //RV output from wind sensor
float RV_Wind_Volts;
unsigned long lastMillis;
int TempCtimes100;
float zeroWind_ADunits;
float zeroWind_volts;
float WindSpeed_MPH;
int lastWindSpeed_MPH = 0;
int leds[MAX_LEDS] = { 12, 11, 10, 9, 8, 7, 6, 5 };  // candle LED
int sensors[NUM_SENSORS] = { 0, 2, 4, 6 };
int windBrightness[NUM_SENSORS];
int brightness = 255;     // how bright the LED is
const int buttonPin = 2;  // the number of the pushbutton pin
int buttonState = 0;      // variable for reading the pushbutton status
int mappings[NUM_SENSORS][2] = { { 0, 20 }, { 0, 30 }, { 0, 30 }, { 0, 30 } };
int numLeds = 0;
bool candleOn = false;

typedef struct Influance {
  int sensor1;
  int sensor2;
  float influance1;
};

Influance sensorInfluances[MAX_LEDS] = { { 0, 1, .5 }, { 0, 1, .5 }, { 0, 1, .5 }, { 0, 1, .5 } };

uint32_t syncProvider() {
  return RTC.now().unixtime();  //either format works
}

void setup() {

  Serial.begin(9600);  // faster printing to get a bit better throughput on extended info
  // remember to change your serial monitor
  while (!Serial)
    ;  // wait until Arduino Serial Monitor opens
  Serial.println("start");

  if (!RTC.begin()) {
    // Serial.println("Couldn't find RTC");
    // Serial.flush();
    // while (1) delay(10);
  }

  if (!RTC.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  setSyncProvider(syncProvider);  // the function to get the time from the RTC
  if (timeStatus() != timeSet)
    Serial.println("Unable to sync with the RTC");
  else
    Serial.println("RTC has set the system time");
  digitalClockDisplay();



  int row = year() - 2023;

  int currentDay = dateDiff(years[row], months[row], days[row], year(), month(), day());
  numLeds = (currentDay > 0 && currentDay <= 8) ? currentDay + 1 : 0;
  Serial.print("Leds Number: ");
  Serial.println(numLeds);

  Serial.println(numLeds);
  if (hour() >= ON_HOUR && hour() <= OFF_HOUR) {
    candleOn = true;
  } else {
    candleOn = false;
  }

  //setTime(RTC.getEpoch());
  // setTime(8,29,0,1,1,11);
  Alarm.alarmRepeat(ON_HOUR, 0, 0, candleOnAlarm);    // 5:00pm every day
  Alarm.alarmRepeat(OFF_HOUR, 0, 0, candleOffAlarm);  // 5:00pm every day


  // initialize the digital pin as an output.
  for (int i = 0; i < MAX_LEDS; i++) {
    pinMode(leds[i], OUTPUT);
  }
  pinMode(13, OUTPUT);
  digitalWrite(13, candleOn ? HIGH : LOW);
}

void loop() {
  // get the state of the button, HIGH (1) or LOW (0)
  //buttonState = digitalRead(buttonPin);

  if (millis() - lastMillis > 200) {  // read every 200 ms - printing slows this down further
    for (int i = 0; i < NUM_SENSORS; i++) {
      TMP_Therm_ADunits = analogRead(sensors[i]);
      RV_Wind_ADunits = analogRead(sensors[i] + 1);
      RV_Wind_Volts = (RV_Wind_ADunits * 0.0048828125);

      // these are all derived from regressions from raw data as such they depend on a lot of experimental factors
      // such as accuracy of temp sensors, and voltage at the actual wind sensor, (wire losses) which were unaccouted for.
      TempCtimes100 = (0.005 * ((float)TMP_Therm_ADunits * (float)TMP_Therm_ADunits)) - (16.862 * (float)TMP_Therm_ADunits) + 9075.4;

      zeroWind_ADunits = -0.0006 * ((float)TMP_Therm_ADunits * (float)TMP_Therm_ADunits) + 1.0727 * (float)TMP_Therm_ADunits + 47.172;  //  13.0C  553  482.39

      zeroWind_volts = (zeroWind_ADunits * 0.0048828125) - zeroWindAdjustment;

      // This from a regression from data in the form of
      // Vraw = V0 + b * WindSpeed ^ c
      // V0 is zero wind at a particular temperature
      // The constants b and c were determined by some Excel wrangling with the solver.

      WindSpeed_MPH = pow(((RV_Wind_Volts - zeroWind_volts) / .2300), 2.7265);

      // map values
      int output = constrain(WindSpeed_MPH, mappings[i][0], mappings[i][1]);
      windBrightness[i] = map(output, mappings[i][0], mappings[i][1], 255, 0);
      /*
      Serial.print("  ID ");
      Serial.print(i);

      Serial.print("  TMP volts ");
      Serial.print(TMP_Therm_ADunits * 0.0048828125);

      Serial.print(" RV volts ");
      Serial.print((float)RV_Wind_Volts);

      Serial.print("\t  TempC*100 ");
      Serial.print(TempCtimes100 );

      Serial.print("   ZeroWind volts ");
      Serial.print(zeroWind_volts);

      Serial.print("   WindSpeed MPH ");
      Serial.print((float)WindSpeed_MPH);

      Serial.print("  brightness   ");
      Serial.println((float)windBrightness[i] );
      */
    }
    //   Serial.println(buttonState);
    //if (WindSpeed_MPH > 10) {
    douseCandle();
    /*
      // run the douseCandle function if the wind speed deteted is more than 6 mph
      if (WindSpeed_MPH > 8 && ledOn > 800) {
      ledOff = 200;
      ledOn = 0;
      douseCandle();
      }
      else if (ledOff > 0 && ledOff < 600 && (lastWindSpeed_MPH - WindSpeed_MPH) < 10){
      ledOff += 200;
      Serial.print("led low");
      Serial.print("   WindSpeed MPH ");
      Serial.println((float)WindSpeed_MPH);
      } else {
      // relight the LED if the button is being pushed
      if (buttonState == HIGH) {
        lightCandle();
        ledOff = 0;
        ledOn += 200;
      }
    */
    lastMillis = millis();
    // lastWindSpeed_MPH = WindSpeed_MPH;
  }
  // }
}

void douseCandle() {
  for (int i = 0; i < numLeds; i++) {
    switch (i) {
      case 0:
      case 1:
        analogWrite(leds[i], candleOn ? windBrightness[0] : 0);
        break;
      case 2:
      case 3:
        analogWrite(leds[i], candleOn ? windBrightness[1] : 0);
        break;
      case 4:
      case 5:
        analogWrite(leds[i], candleOn ? windBrightness[2] : 0);
        break;
      case 6:
      case 7:
        analogWrite(leds[i], candleOn ? windBrightness[3] : 0);
        break;
    }
  }
}

void candleOnAlarm() {
  numLeds++;
  digitalWrite(13, HIGH);
  candleOn = true;
  Serial.println("Alarm candleOn: " + numLeds);
}
void candleOffAlarm() {
  candleOn = false;
  digitalWrite(13, LOW);
  Serial.println("Alarm candleOff: " + numLeds);
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(year());
  printDigits(month());
  printDigits(day());
  Serial.print(" ");
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println();
}

void printDigits(int digits) {
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

int dateDiff(int year1, int mon1, int day1, int year2, int mon2, int day2) {
  int ref, dd1, dd2, i;
  ref = year1;
  if (year2 < year1)
    ref = year2;
  dd1 = 0;
  dd1 = dater(mon1);
  for (i = ref; i < year1; i++) {
    if (i % 4 == 0)
      dd1 += 1;
  }
  dd1 = dd1 + day1 + (year1 - ref) * 365;
  dd2 = 0;
  for (i = ref; i < year2; i++) {
    if (i % 4 == 0)
      dd2 += 1;
  }
  dd2 = dater(mon2) + dd2 + day2 + ((year2 - ref) * 365);
  return dd2 - dd1;
}

int dater(int x) {
  const int dr[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
  return dr[x - 1];
}
