#include <OneWire.h>
#include <DallasTemperature.h>
#include <DS3231.h>
#include <Wire.h>
#include <TimeLib.h>

DS3231 ora;

OneWire oneWire(A2);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

volatile uint16_t i = 2000;
const uint16_t erosites = 3600;

byte time_hour, time_min, old_time_min;

float tempC_old;
bool disp;

bool uv;
bool century = false;
bool h12Flag;
bool pmFlag;

void setup() {
  Serial.begin(57600);
  Wire.begin();
  sensors.begin();
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Temp Err.");
  sensors.setResolution(insideThermometer, 12);

  pinMode(3, OUTPUT); // TRIAC   PORTD, 3
  pinMode(9, OUTPUT); // SSR     PORTB, 1

  setTime(ora.getHour(h12Flag, pmFlag), ora.getMinute(), ora.getSecond(), ora.getDate(), ora.getMonth(century), ora.getYear());

  attachInterrupt(0, zero_cross_detect, RISING);
}

void zero_cross_detect() {
  delayMicroseconds(9800 - i);
  bitSet(PORTD, 3);
  delayMicroseconds(25);
  bitClear(PORTD, 3);
}

void loop(void) {
  while (Serial.available() > 1) {
    byte c = Serial.read();
    if ((c == 49)) {
      bitSet(PORTB, 1);
      Serial.println("UV_ON");
    }
    else if ((c == 48)) {
      bitClear(PORTB, 1);
      Serial.println("UV_OFF");
    }
    else if ((c == 't')) {
      Serial.print(year());
      Serial.print("-");
      Serial.print(month());
      Serial.print("-");
      Serial.print(day());
      Serial.print(" ");
      Serial.print(hour());
      Serial.print(":");
      Serial.print(minute());
      Serial.println();
    }
    else if ((c == 'i')) {
      i = 4000;
      Serial.print(i);
      Serial.println();
    }
  }
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempC(insideThermometer);
  if (tempC != tempC_old) {
    tempC_old = tempC;
    disp = true;
    Serial.print(tempC);
    Serial.print("C ");
  }

  uint16_t futes;
  if (30.0 - tempC >= 0.0)
    futes = erosites * (30.0 - tempC);
  else futes = 0;

  time_hour = hour();
  time_min = ora.getMinute();
  if (time_min != old_time_min) {
    setTime(ora.getHour(h12Flag, pmFlag), time_min, ora.getSecond(), ora.getDate(), ora.getMonth(century), ora.getYear());
    old_time_min = time_min;
    if (disp) Serial.print(" T ");
  }

  if (time_hour >= 7 && time_hour < 8) {
    if (i < 9500) i = i + 3;
    if (disp) {
      Serial.print(i);
      Serial.println();
    }
  }
  else if (time_hour >= 8 && time_hour < 19) {
    i = constrain(futes, 0, 9500);
    if (disp) Serial.print(i);
    if (tempC >= 27.0) {
      bitSet(PORTB, 1);
      if (disp) Serial.print(" UV1");
    }
    else if (tempC <= 26.0)
    {
      bitClear(PORTB, 1);
      if (disp) Serial.print(" UV0");
    }
    if (disp) Serial.println();
  }
  else {
    if (i > 2) i -= 2;
    bitClear(PORTB, 1);
    if (disp) {
      Serial.print(i);
      Serial.println();
    }
  }
  disp = false;

}
