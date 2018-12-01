// Michael Dempsey
// Doppler Radar Respiratory Monitor
// Menu Test Code 1.1
// April 11, 2018

#include <LiquidCrystal.h>

// pin declarations for circuit board rev. 1.2
const byte buttonA = 0;
const byte buttonB = 1;
const byte buttonC = 2;
const byte radar   = 3;
const byte buzzer  = 4;
const byte LEDA    = 5;
const byte LEDB    = 6;
const byte LEDC    = 7;
const byte d7      = 8;
const byte d6      = 9;
const byte d5      = 10;
const byte d4      = 11;
const byte en      = 12;
const byte rs      = 13;

const int delayParam          = 300;
const int avgParam            = 10;
const int prevValDim          = 50;
const double yellowThreshold  = 10;
const double redThreshold     = 20;

// start display screen
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// global variable declarations
boolean oldState, state = false; 
byte change, turnedOff = 0;
long goOn, goOff = 0;
long onTimeOld, onTime = 0;

// checkLimits func variables
double maxRate, minRate = 0;

// saveValue func variables
double previousValues[prevValDim];
byte value;

// calibrate func variable
byte calibrateCount = 0;

// findAverage func variable
double recentVals[avgParam];

// writeScreen func variables
String row1TextOld, row2TextOld;
float row1DataOld, row2DataOld;

// getValue func variables
long breath[2] = {0,0};
boolean findAvg = false;
double breathRate, average, breathTime  = 0; 

// main func variables
double difference = 0;

// initializations
void setup() 
{
  lcd.begin(16, 2);
  pinMode(buttonA, INPUT);
  pinMode(buttonB, INPUT);
  pinMode(buttonC, INPUT);
  pinMode(radar,   INPUT);
  pinMode(buzzer,  OUTPUT);
  pinMode(LEDA,    OUTPUT);
  pinMode(LEDB,    OUTPUT);
  pinMode(LEDC,    OUTPUT);
  writeScreen("Breathing Monitor", -1, "Version 1.2", -1);
  delay(2000);
  writeScreen("System Ready", -1, "", -1);
  delay(1000);
  writeScreen("Click ENTER to", -1, "calibrate.", -1);
  attachInterrupt(1, radarRise, RISING);
  calibrate();
}

// ISR for rising input from radar sensor
void radarRise()
{  
  oldState = state;
  onTimeOld = onTime;
  onTime = millis();
  turnedOff = 0; 
  if (onTime - onTimeOld < delayParam)
  {
    state = true;
    if(oldState != state)
    {
      goOn = onTimeOld;
      change = 1;
    }
  }
  if((onTime - onTimeOld >= delayParam))
  {
    state = false;
    if(oldState != state)
    {
      goOff = onTimeOld;
      change = 2;
    }
  }
}

// function for updating max and min rates
void checkLimits(double rate)
{
  if(rate > maxRate)
    maxRate = rate;
    
  if(rate < minRate && rate > 0)
    minRate = rate;
}

void saveValue(double newRate)
{
  previousValues[value] = newRate;
  value++;
  if(value > prevValDim)
    value = 0;
}

// function for finding the updated rolling average
double findAverage(double newData)
{
  double avg, tmpSum;
  byte lengthRecentVals;
  lengthRecentVals = sizeof(recentVals) / sizeof(double);
  tmpSum = 0;
  for(byte a = 0; a < lengthRecentVals - 1; a++)
    recentVals[a + 1] = recentVals[a]; 
  recentVals[0] = newData;
  for(byte b = 0; b < lengthRecentVals; b++)
    tmpSum =  tmpSum + recentVals[b];
  avg = tmpSum / lengthRecentVals;
  return avg;
}

// function for writing data to display only when new data is available
void writeScreen(String row1Text, float row1Data, String row2Text, float row2Data)
{
  long row1DataLength, row2DataLength;
  row1DataLength = String(row1Data).length();
  row2DataLength = String(row2Data).length();
  if(row1Text != row1TextOld || row2Text != row2TextOld || row1Data != row1DataOld || row2Data != row2DataOld)
  {
    if((row1Data == -1) && (row2Data == -1))
    {
      clearScreen();
      lcd.setCursor(0, 0);
      lcd.print(row1Text);
      lcd.setCursor(0, 1);
      lcd.print(row2Text);
    }
    if((row1Data == -1) && (row2Data != -1))
    {
      clearScreen();
      lcd.setCursor(0, 0);
      lcd.print(row1Text);
      lcd.setCursor(0, 1);
      lcd.print(row2Text);
      lcd.setCursor(16 - row2DataLength, 1);
      lcd.print(row2Data);
    }
    if((row2Data == -1) && (row1Data != -1))
    {
      clearScreen();
      lcd.setCursor(0, 0);
      lcd.print(row1Text);
      lcd.setCursor(16 - row1DataLength, 0);
      lcd.print(row1Data);
      lcd.setCursor(0, 1);
      lcd.print(row2Text);
    }
    if((row2Data != -1) && (row1Data != -1))
    {
      clearScreen();
      lcd.setCursor(0, 0);
      lcd.print(row1Text);
      lcd.setCursor(16 - row1DataLength, 0);
      lcd.print(row1Data);
      lcd.setCursor(0, 1);
      lcd.print(row2Text);
      lcd.setCursor(16 - row2DataLength, 1);
      lcd.print(row2Data);
    }
  }
  row1TextOld = row1Text;
  row2TextOld = row2Text;
  row1DataOld = row1Data;
  row2DataOld = row2Data;
}

void clearScreen()
{
  for(byte a = 0; a < 16; a++)
  {
    for(byte b = 0; b < 2; b ++)
    {
      lcd.setCursor(a, b);
      lcd.print(" ");
    }
  }
} 

void calibrate()
{
  while(digitalRead(buttonB) == 0){}
  writeScreen("Calibrating...", -1, "Breath Normally", -1);
  while(calibrateCount < 10)
    getValue(); 
  writeScreen("Calibration", -1, "complete!", -1);
}

void getValue()
{
  if((millis() - onTime >= delayParam) && turnedOff == 0)
  {
    state = false;
    goOff = onTime + delayParam;
    change = 2;
    turnedOff = 1;
  }
  if (change == 1)
  {
    breath[0] = goOn;
    change = 0;
  }
  if (change == 2)
  {
    breath[1] = goOff;
    change = 0;
    findAvg = true;
  }  
  if(breath[1] > breath[0])
  {
    breathTime = breath[1] - breath[0];
    breathTime = breathTime * 2;
    breathTime = breathTime / 60000;
    breathRate = 1 / breathTime;
    if(findAvg == true)
    {
      average = findAverage(breathRate);
      findAvg = false;
      checkLimits(breathRate);
      saveValue(breathRate);
      calibrateCount++;
      if(calibrateCount > 11)
        calibrateCount = 11;
    }   
  } 
}

void greenAlert()
{  
  digitalWrite(LEDA, HIGH);
  digitalWrite(LEDB, LOW);
  digitalWrite(LEDC, LOW);
  writeScreen("Situation Normal", -1, "", -1);
}

void yellowAlert()
{
  digitalWrite(LEDA, LOW);
  digitalWrite(LEDB, HIGH);
  digitalWrite(LEDC, LOW);
  writeScreen("Warning!!", -1, "Check Patient", -1);
  while(digitalRead(buttonB) == 0){}
  
}

void redAlert()
{
  digitalWrite(LEDA, LOW);
  digitalWrite(LEDB, LOW);
  digitalWrite(buzzer, HIGH);
  while(digitalRead(buttonB) == 0)
  {
    writeScreen("Critical Alert!", -1, "Check Patient", -1);
    digitalWrite(LEDC, HIGH);
    delay(1000);
    writeScreen("", -1, "Check Patient", -1);
    digitalWrite(LEDC, LOW);
    delay(1000);
  }
  digitalWrite(buzzer, LOW);
}

String menuOption[12] = {"Pause Operation", "", "View Average", "Breathing Rate", "View Max/Min", "Breathing Rates", "View Previous", "Breathing Rates", "Run", "Recalibration", "Exit Menu", ""};
byte option, recentOption;
boolean exitMenu = false;

void openMenu()
{
  while(exitMenu == false)
  {
    if(digitalRead(buttonA) == 1)
      option--;
    if(digitalRead(buttonC) == 1)
      option++;
    if(option < 0)
      option = 5;
    if(option > 5)
      option = 0;
    writeScreen(menuOption[option * 2], -1, menuOption[option * 2 + 1], -1);
    if((digitalRead(buttonB) == 1) && (option == 1))
    {
      writeScreen("Average Rate", average, "Current Rate", breathRate);
      while(digitalRead(buttonB) == 0){}
    }
    if((digitalRead(buttonB) == 1) && (option == 2))
    {
      writeScreen("Max Rate", maxRate, "Min Rate", minRate);
      while(digitalRead(buttonB) == 0){}
    }
    if((digitalRead(buttonB) == 1) && (option == 3))
    {
      while(digitalRead(buttonB) == 0)
      {
        if(digitalRead(buttonA) == 1)
          recentOption--;
        if(digitalRead(buttonC) == 1)
          recentOption++;
        if(recentOption < 0)
          recentOption = prevValDim;
        if(recentOption > prevValDim)
          recentOption = 0;
        writeScreen("Previous Rate", recentOption + 1, "", previousValues[recentOption]);
      }
    }
    if((digitalRead(buttonB) == 1) && (option == 5))
    {
      writeScreen("Closing Menu", -1, "", -1);
      delay(1000);
      exitMenu = true;
    }
  }
}

void checkAlert()
{
  if(average - breathRate >= 0)
    difference = average - breathRate;
  if(average - breathRate < 0)
    difference = breathRate - average;
  if(difference < yellowThreshold)
    greenAlert();
  if((difference > yellowThreshold) && (difference < redThreshold))
    yellowAlert();
  if(difference > redThreshold)
    redAlert();
}

// main loop
void loop()
{
  getValue();
  checkAlert();  
  if(digitalRead(buttonB) == 1)
    openMenu();
}

