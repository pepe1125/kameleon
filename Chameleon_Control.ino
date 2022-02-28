#include <OneWire.h>
#include <DallasTemperature.h>
#include <DS3231.h>
#include <Wire.h>
OneWire oneWire(A2);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;
DS3231 Clock;
byte Year;
byte Month;
byte Date;
byte DoW;
byte Hour;
byte Minute;
byte Second;
bool Century = false;
bool h12;
bool PM;
int interval = 1000;          // interval between refresh in millisec
long lastTime = 0;

volatile uint16_t i = 2000;
const uint16_t erosites = 3600;

byte time_hour, time_min, old_time_min;
float tempC;
float tempC_old;
byte uvcnt;
bool disp;
bool uv;
bool reading;

void GetDateStuff(byte& Year, byte& Month, byte& Day, byte& DoW,
                  byte& Hour, byte& Minute, byte& Second) {
  // Call this if you notice something coming in on
  // the serial port. The stuff coming in should be in
  // the order YYMMDDwHHMMSS, with an 'x' at the end.
  boolean GotString = false;
  char InChar;
  byte Temp1, Temp2;
  char InString[20];
  byte j = 0;
  while (!GotString) {
    if (Serial.available()) {
      InChar = Serial.read();
      InString[j] = InChar;
      j += 1;
      if (InChar == 'x') {
        GotString = true;
      }
    }
  }
  Serial.println(InString);
  // Read Year first
  Temp1 = (byte)InString[0] - 48;
  Temp2 = (byte)InString[1] - 48;
  Year = Temp1 * 10 + Temp2;
  // now month
  Temp1 = (byte)InString[2] - 48;
  Temp2 = (byte)InString[3] - 48;
  Month = Temp1 * 10 + Temp2;
  // now date
  Temp1 = (byte)InString[4] - 48;
  Temp2 = (byte)InString[5] - 48;
  Day = Temp1 * 10 + Temp2;
  // now Day of Week
  DoW = (byte)InString[6] - 48;
  // now Hour
  Temp1 = (byte)InString[7] - 48;
  Temp2 = (byte)InString[8] - 48;
  Hour = Temp1 * 10 + Temp2;
  // now Minute
  Temp1 = (byte)InString[9] - 48;
  Temp2 = (byte)InString[10] - 48;
  Minute = Temp1 * 10 + Temp2;
  // now Second
  Temp1 = (byte)InString[11] - 48;
  Temp2 = (byte)InString[12] - 48;
  Second = Temp1 * 10 + Temp2;
}

void setup() {
  Serial.begin(57600);
  Wire.begin();
  sensors.begin();
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Temp Err.");
  sensors.setResolution(insideThermometer, 12);

  pinMode(3, OUTPUT); // TRIAC   PORTD, 3
  pinMode(9, OUTPUT); // SSR     PORTB, 1

  attachInterrupt(0, zero_cross_detect, RISING);
}

void zero_cross_detect() {
  delayMicroseconds(9800 - i);
  bitSet(PORTD, 3);
  delayMicroseconds(25);
  bitClear(PORTD, 3);
}

void loop(void) {
  if (Serial.available()) {
    byte c = Serial.read();
    if ((c == 49)) {
      uv = 1;
      Serial.println("UV_ON");
    }
    else if ((c == 48)) {
      uv = 0;
      Serial.println("UV_OFF");
    }
    else if ((c == 't')) {
      Serial.print(Clock.getYear(), DEC);
      Serial.print("-");
      Serial.print(Clock.getMonth(Century), DEC);
      Serial.print("-");
      Serial.print(Clock.getDate(), DEC);
      Serial.print(" ");
      Serial.print(Clock.getHour(h12, PM), DEC); //24-hr
      Serial.print(":");
      Serial.print(Clock.getMinute(), DEC);
      Serial.print(":");
      Serial.println(Clock.getSecond(), DEC);
    }
    else if ((c == 'T')) {
      GetDateStuff(Year, Month, Date, DoW, Hour, Minute, Second);
      Clock.setClockMode(false);  // set to 24h
      Clock.setSecond(Second);
      Clock.setMinute(Minute);
      Clock.setHour(Hour);
      Clock.setDate(Date);
      Clock.setMonth(Month);
      Clock.setYear(Year);
      Clock.setDoW(DoW);
      Serial.println("OK");
    }
    else if ((c == 'i')) {
      byte value = Serial.read();
      value = constrain(value, 49, 57);
      value = value - 48;
      i = 1000 * value;
      Serial.print(i);
      Serial.println();
    }
  }

  if ((millis() - lastTime > interval)) { // REFRESH
    reading = !reading;
    if (reading) {
      sensors.requestTemperatures(); // Send the command to get temperatures
      tempC = sensors.getTempC(insideThermometer);
      if (tempC != tempC_old) {
        tempC_old = tempC;
        disp = true;
        Serial.print(tempC);
        Serial.print("C ");
      }
    }
    else {
      time_hour = Clock.getHour(h12, PM);
    }
    uvcnt++;
    if (uvcnt > 60) {
      uv = 0;
      uvcnt = 0;
    }
    lastTime = millis();
  }

  uint16_t futes;
  if (30.0 - tempC >= 0.0)
    futes = erosites * (30.0 - tempC);
  else futes = 0;


  if (time_hour >= 7 && time_hour < 8) {
    futes = constrain(futes, 0, 9500);
    if (i < futes && (millis() % 1000 == 0)) i = i + 5;
    if (disp) {
      Serial.print(i);
      Serial.println();
    }
  }
  else if (time_hour >= 8 && time_hour < 19) {
    i = constrain(futes, 0, 9500);
    if (disp) Serial.print(i);
    if (time_hour >= 15)
    {
      if (uv) {
        bitSet(PORTB, 1);
      }
      else {
        bitClear(PORTB, 1);
      }
      if (disp) Serial.print(" UVt");
    }
    else if (tempC >= 27.0 && time_hour >= 11) {
      bitSet(PORTB, 1);
      if (disp) Serial.print(" UV>");
    }
    else if (tempC <= 26.0)
    {
      if (uv) {
        bitSet(PORTB, 1);
      }
      else {
        bitClear(PORTB, 1);
      }
      if (disp) Serial.print(" UV<");
    }
    if (disp) Serial.println();
  }
  else {
    if (i > 2 && (millis() % 1000 == 0)) i -= 2;
    if (uv) {
      bitSet(PORTB, 1);
    }
    else {
      bitClear(PORTB, 1);
    }
    if (disp) {
      Serial.print(i);
      Serial.println();
    }
  }
  disp = false;

}
