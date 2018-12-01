// Doppler Radar Respiratory Monitor
// Main Code 1.0
// April 15, 2018

#include <LiquidCrystal.h>

// pin declarations for circuit board rev. 1.2
const byte buttonA = 1;
const byte buttonB = 2;
const byte buttonC = 0;
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
const int inputDelay          = 300;

// start display screen
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// global variable declarations
boolean oldState, state = false; 
byte change, turnedOff = 0;
long goOn, goOff = 0;
long onTimeOld, onTime = 0;

// calibrate func variable
byte calibrateCount = 0;

// getValue func variables
long breath[2] = {0,0};
boolean findAvg = false;
double breathRate, average, breathTime  = 0; 

// findAverage func variable
double recentVals[avgParam];

// checkLimits func variables
double maxRate = 0;
double minRate = 1000;

// saveValue func variables
double previousValues[prevValDim];
byte value;

// writeScreen func variables
String row1TextOld, row2TextOld;
float row1DataOld, row2DataOld;

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
  scrollText("Doppler Radar Respiratory Monitor Version 1.2");
  //writeScreen("Doppler Radar", -1, "Respiratory Monitor", -1);
  delay(2000);
  writeScreen("System Ready", -1, "", -1);
  delay(1000);
  writeScreen("Click ENTER to", -1, "calibrate.", -1);
  attachInterrupt(1, radarRise, RISING);
  //calibrate(10);
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

// runs calibration cycle for specified number of data points 
void calibrate(byte qty)
{
  calibrateCount = 0;
  while(digitalRead(buttonB) == 0){}
  writeScreen("Calibrating...", -1, "Breath Normally", -1);
  while(calibrateCount < qty)
    getValue(); 
  writeScreen("Calibration", -1, "complete!", -1);
  delay(1000);
}

// updates breathing information when called
// when a new rate is available, the average, max and min, and previous values are updated
// the new rate values increment the counter used for calibration
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

// finds the updated rolling average based on the newly available rate data
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

// updates the max and min rates
// sending -1 resets the max and min values
void checkLimits(double rate)
{
  if((rate > maxRate) && (rate > 0))
    maxRate = rate; 
  if((rate < minRate) && (rate > 0))
    minRate = rate;
  if(rate == -1)
  {
    maxRate = 0;
    minRate = 1000;
  }
}

// saves every data point, rolling over previous data when limits exceeded
void saveValue(double newRate)
{
  previousValues[value] = newRate;
  value++;
  if(value > prevValDim)
    value = 0;
}

// checks the status based off the current rate and average
void checkAlert(boolean menu)
{
  double difference;
  if(average - breathRate >= 0)
    difference = average - breathRate;
  if(average - breathRate < 0)
    difference = breathRate - average;
  if((difference < yellowThreshold) && (menu == false))
  {
    digitalWrite(LEDA, HIGH);
    digitalWrite(LEDB, LOW);
    digitalWrite(LEDC, LOW);
    writeScreen("Situation Normal", -1, "", -1);
  }
  if((difference > yellowThreshold) && (difference < redThreshold))
  {
    digitalWrite(LEDA, LOW);
    digitalWrite(LEDB, HIGH);
    digitalWrite(LEDC, LOW);
    writeScreen("Warning!!", -1, "Check Patient", -1);
    while(digitalRead(buttonB) == 0)
    {
      getValue();
    }
  }
  if(difference > redThreshold)
  {
    digitalWrite(LEDA, LOW);
    digitalWrite(LEDB, LOW);
    digitalWrite(buzzer, HIGH);
    while(digitalRead(buttonB) == 0)
    {
      getValue();
      writeScreen("Critical Alert!", -1, "Check Patient", -1);
      digitalWrite(LEDC, HIGH);
      delay(1000);
      getValue();
      writeScreen("", -1, "Check Patient", -1);
      digitalWrite(LEDC, LOW);
      delay(1000);
    }
    digitalWrite(buzzer, LOW);
  }
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

// scrolls long string of text across screen to left
void scrollText(String text)
{
  clearScreen();
  lcd.setCursor(0, 0);
  lcd.print(text);
  for(byte a = 0; a < text.length(); a++)
  {
    lcd.scrollDisplayLeft();
    delay(250);
  }
}

//clears the LCD screen
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

String menuOption[12] = {"Pause Operation", "", "View Average", "Breathing Rate", "View Max/Min", "Breathing Rates", "View Previous", "Breathing Rates", "Run", "Recalibration", "Exit Menu", ""};
byte option, recentOption;
boolean exitMenu = false;

void openMenu()
{
  option = 1;
  while(exitMenu == false)
  {
    if(digitalRead(buttonA) == 1)
    {
      option--;
      delay(inputDelay);
    }
    if(digitalRead(buttonC) == 1)
    {
      option++;
      delay(inputDelay);
    }
    option = checkLimits(option, 1, 6);
    writeScreen(menuOption[(option - 1) * 2], -1, menuOption[(option - 1) * 2 + 1], -1);

    if(digitalRead(buttonB) == 1)
    {
      delay(inputDelay);
      switch(option)
      {
        case 1:
          writeScreen("Paused: Click", -1, "ENTER To Resume", -1);
          while(digitalRead(buttonB) == 0){}
          delay(inputDelay);
          //calibrate(10);
          break;
        case 2: 
          while(digitalRead(buttonB) == 0)
          {
            getValue();
            //checkAlert(true);
            writeScreen("Avg Rate", average, "Rate Now", breathRate);
          }
          delay(inputDelay);
          break;
        case 3:
          while(digitalRead(buttonB) == 0)
          {
            getValue();
            //checkAlert(true);
            writeScreen("Max Rate", maxRate, "Min Rate", minRate);
          }
          delay(inputDelay);
          break;
        case 4:
          while(digitalRead(buttonB) == 0)
          {
            getValue();
            //checkAlert(true);
            if(digitalRead(buttonA) == 1)
            {
              delay(inputDelay);
              recentOption--;
            }
            if(digitalRead(buttonC) == 1)
            {
              delay(inputDelay);
              recentOption++;
            }
            recentOption = checkLimits(recentOption, 0, prevValDim);
            writeScreen("Previous Rate", recentOption + 1, "", previousValues[recentOption]);
          }
          delay(inputDelay);
          break;
        case 5:
          //calibrate(10);
          break;
        case 6:
          writeScreen("Closing Menu", -1, "", -1);
          delay(1000);
          exitMenu = true;
      }
    }
  }
}

//checks a byte to ensure it falls within the specified limits
byte checkLimits(byte val, byte minVal, byte maxVal)
{
  byte result;
  result = val;
  if(val < minVal)
    result = maxVal;
  if(val > maxVal)
    result = minVal;
  return result;
}

// main loop
void loop()
{
  getValue();
  //checkAlert(false);  
  writeScreen("Rate:", breathRate, "Open Menu?", -1);
  if(digitalRead(buttonB) == 1)
  {
    delay(inputDelay);
    exitMenu = false;
    openMenu();
  }
}
