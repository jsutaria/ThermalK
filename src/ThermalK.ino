////////////////////////////////////////////////////////////////////////////
/*
    ThermalK: Thermal Conductivity Monitor

    Version 0.6.2 - 20150730
  
    Copyight (C) 2015 Sam Belden, Nicola Ferralis
    sbelden@mit.edu, ferralis@mit.edu
 
 This program (source code and binaries) is free software; 
 you can redistribute it and/or modify it under the terms of the
 GNU General Public License as published by the Free Software 
 Foundation, in version 3 of the License.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You can find a complete copy of the GNU General Public License at:
 
 http://www.gnu.org/licenses/gpl.txt 
*/
//////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------------
// TO BE USED ONLY FOR CALIBRATION OF REAL TIME CLOCK. 
//  The next #define TIMECAL line should remain commented for normal operations
//  Uncomment ONLY for calibration of real time clock. 

//  Instructions:
//    0. Make sure the Real Time Clock hardware setup is in place. 
//    1. Compile this program with the "#define TIMECAL" line uncommented.
//    2. Upload but DO NOT open the Serial monitor.
//    3. Uncomment the line "#define TIMECAL".
//    4. Comment and upload.
//    5. Verify the correct date from the Serial monitor.
//-------------------------------------------------------------------------------
//#define TIMECAL  //Uncomment for calibration of real time clock. Otherwise leave commented.

#include <SD.h> 
#include <Wire.h>
#include <SPI.h>
#include "RTClib.h"
#include <LiquidCrystal.h>

//-------------------------------------------------------------------------------
//  SYSTEM defined variables
//-------------------------------------------------------------------------------
String versProg = "0.6.2 - 20150730";
//String nameProg = "ThermalK: Thermal Conductivity Monitor";
//String developer = "Copyright (C) 2015 Sam Belden, Nicola Ferralis";

String nameProg = "TK";
String developer = "";

float display_delay = 0.2;  //in seconds - refresh time in serial monitor
float TmediumInitial = 0.0;

//-------------------------------------------------------------------------------
//  LCD display 
//-------------------------------------------------------------------------------
//#define LCD       //if commented, runs on regular serial

#ifdef LCD
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#endif

//-------------------------------------------------------------------------------
// Thermal conductivity parameters
//-------------------------------------------------------------------------------
  // parameters of thermal conductivity measurement
  // Q = heat input (watts)
  // L = thickness of sample
  // A = surface area/contact area of sample
  // deltaT = change in Temp from hot to cold side of sample
float Q = 6.2675;
float L_1 = 0.005;
float L_2 = 0.01;
float A = 0.0019635;

//--------------------------------------------------------------------------------
// Change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10 (also change MEGA_SOFT_SPI from 0 to 1)
// Sparkfun SD shield: pin 8
//---------------------------------------------------------------------------------
#define SDshield 10
char cfgFile[]="ThermalK.cfg";
const int chipSelect = SDshield;
char nameFile[6];
char nameFileData[6];
char nameFileSummary[6]; 

//-------------------------------------------------------------------------------v 
// Define pins and parameters for the thermistors
//-------------------------------------------------------------------------------
#define therm1 A0
#define therm2 A1
#define therm3 A2

// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000 

//-------------------------------------------------------------------------------
//Real time chip
//-------------------------------------------------------------------------------

RTC_DS1307 rtc; // define the Real Time Clock object
DateTime now;   // New RTC library

//-------------------------------------------------------------------------------
//  SETUP routine
//-------------------------------------------------------------------------------
void setup() { 

#ifdef LCD
#else
  Serial.begin(9600); 
#endif  

#ifdef LCD
  lcd.begin(16, 2);
#endif
  //----------------------------------------
  // get the time from the RTC
  //----------------------------------------
  //Wire1.begin(); //For Arduino DUE
  Wire.begin();  //For Arduino AVR  
  rtc.begin();
  
  if (! rtc.isrunning()) {
    //Serial.println("RTC is NOT running!");
  }

#ifdef TIMECAL
    rtc.adjust(DateTime(__DATE__, __TIME__));
    Serial.println("RTC is syncing!");
#endif

  firstRunSerial();

  //----------------------------------------  
  // Initialization SD card
  //----------------------------------------    
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(53, OUTPUT);    //Arduino boards
#ifdef LCD
#else
  Serial.print("Initializing SD card... ");
#endif
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
#ifdef LCD
#else    
    Serial.println("Card failed, or not present.");
    Serial.println("SD card support disabled.");
    Serial.println();
#endif
  }
  else
  {

    // to use today's date as the filename:
    //nameFile2(0).toCharArray(nameFile, 13);
    //Serial.println();
    //Serial.print("Saving full IV data into: ");
    //Serial.println(nameFile);

    nameFile2(1).toCharArray(nameFileData, 13);
    nameFile2(2).toCharArray(nameFileSummary, 13);

#ifdef LCD
#else   
    Serial.println("OK");
    Serial.print("Saving data: ");
    Serial.println(nameFileData);
    Serial.print("Saving summary: ");
    Serial.println(nameFileSummary);
    Serial.println();
#endif  
  }
    
  TmediumInitial = Tread(therm3);
#ifdef LCD
#else   
  Serial.println("(1) Start; (2) Stop; (3) Reset; (4) Info ");
  Serial.println();
#endif
}


//-------------------------------------------------------------------------------
//  Indefinite loop
//-------------------------------------------------------------------------------

void loop() {
   
  int inSerial = 0;    // variable for serial input
  
  if (Serial.available() > 0) { 
    inSerial = Serial.read();
#ifdef LCD
#else
    Serial.println("\"Time\",\"T Lower Cold Plate\",\"T Lower Hot Plate\",\"T Upper Cold Plate\",\"Thermal Conductivity\"");
#endif

      File dataFile = SD.open(nameFileData, FILE_WRITE);
      dataFile.println("\"Time\",\"T Lower Cold Plate\",\"T Lower Hot Plate\",\"T Upper Cold Plate\",\"Thermal Conductivity\"");
      dataFile.close();

#ifdef LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Start");
#else
#endif
    
    if(inSerial==49)
      { float offset = (float) millis()/1000;
      
      File dataFile = SD.open(nameFileData, FILE_WRITE);
  
      while(inSerial!=50)
        {
        inSerial = Serial.read(); 

        Acquisition(offset, dataFile);
        if(inSerial==51) 
           {
              break;
           }
        }        
        dataFile.println();
        dataFile.close();
        summarySD(offset);

#ifdef LCD
#else   
      Serial.println();
#endif
      }
  if(inSerial==52)  
      {
        firstRunSerial();
      }    
  }
}



//-------------------------------------------------------------------------------
// Acquisition Routine
//-------------------------------------------------------------------------------

void Acquisition(float offset, File dataFile) {
 
  float T1 = Tread(therm1);
  float T2 = Tread(therm2);
  float T3 = Tread(therm3);

  // parameters of thermal conductivity measurement
  // Q = heat input (watts)
  // L = thickness of sample
  // A = surface area/contact area of sample
  // deltaT = change in Temp from hot to cold side of sample
  
  float deltaT_1 = T3 - T1;
  float deltaT_2 = T3 - T2;
  float conductivity = (Q)/(A*((deltaT_1/L_1) + (deltaT_2/L_2)));

#ifdef LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TK = ");
  lcd.print(conductivity);

#else
  Serial.print((float) millis()/1000 - offset);
  Serial.print(", ");
  Serial.print(T1);
  Serial.print(", ");
  Serial.print(T2);
  Serial.print(", ");
  Serial.print(T3);
  Serial.print(", ");
  Serial.print(conductivity);
  Serial.println();
#endif

  dataFile.print((float) millis()/1000 - offset);
  dataFile.print(", ");
  dataFile.print(T1);
  dataFile.print(", ");
  dataFile.print(T2);
  dataFile.print(", ");
  dataFile.print(T3);
  dataFile.print(", ");
  dataFile.print(conductivity);
  dataFile.println();
  
  delay(display_delay*1000);              // wait for display refresh
}


//-------------------------------------------------------------------------------
// Read temperature from thermistor  
//-------------------------------------------------------------------------------

float Tread(int THERMISTORPIN) { 
  int samples[NUMSAMPLES];
   // take N samples in a row, with a slight delay
  for (int i=0; i< NUMSAMPLES; i++) {
    samples[i] = analogRead(THERMISTORPIN);
    delay(10);
    }

  // average all the samples out
  float average = 0;
  for (int i=0; i< NUMSAMPLES; i++) {
    average += samples[i];
    }
  average /= NUMSAMPLES;

  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;

  float steinhart;
  steinhart = average / THERMISTORNOMINAL; // (R/Ro)
  steinhart = log(steinhart); // ln(R/Ro)
  steinhart /= BCOEFFICIENT; // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart; // Invert
  steinhart -= 273.15; // convert to C
  // steinhart = (steinhart*(9/5) + 32);
  
  return steinhart;
}


///////////////////////////////////////////
// Set filename with the format:
// yearmonthday.csv
///////////////////////////////////////////

String nameFile2(int a) {
  String filename;

  now = rtc.now();
  //filename += now.year();
  if(a==0)
  {
    if(now.month() < 10)
      filename += 0;
    filename += (0 + now.month());
    if(now.day() < 10)
      filename += 0;
    filename += (0 + now.day());
  }
  if(a==1)
  {
    filename += "Data";
    //filename += a;
  }

  if(a==2)
  {
    filename += "Sum";
    //filename += a;
  }
  filename += ".csv";
  return filename;
}

/////////////////////////////////////////////////////
// Initial Display of information and settings  
/////////////////////////////////////////////////////

void firstRunSerial()  { 
#ifdef LCD
  lcd.setCursor(0, 0);   //
  lcd.clear();
  lcd.print(nameProg);
  lcd.setCursor(0, 1);
  lcd.print("v. ");
  lcd.println(versProg);
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("initializing...");
#else
  
  Serial.println();
  Serial.println();
  //Serial.println("----------------------------------------------------------------------");
  Serial.print(nameProg);
  Serial.print(" - v. ");
  Serial.println(versProg);
  Serial.println(developer);
  //Serial.println("----------------------------------------------------------------------");
  Serial.println();
   DateTime now = rtc.now();
    Serial.print("Time: ");
  Serial.print(now.hour(),DEC);
  Serial.print(":");
  if(now.minute() < 10)
    Serial.print('0');
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  if(now.second() < 10)
    Serial.print('0');
  Serial.print(now.second(), DEC);
  Serial.print(" (");
  Serial.print(now.month(), DEC );
  Serial.print("-");
  Serial.print(now.day(),DEC );
  Serial.print("-");
  Serial.print(now.year(), DEC);
  Serial.println(")");
  
  Serial.print("Refresh (s): ");
  Serial.println(display_delay);
  Serial.print("T Lower CP (C): ");
  Serial.println(Tread(therm1));
  Serial.print("T Lower HP (C): ");
  Serial.println(Tread(therm2));
  Serial.print("T Upper CP (C): ");
  Serial.println(Tread(therm3)); 
  Serial.println();
  
  #endif
}


/////////////////////////////////////////////////////
// write summary to SD  
/////////////////////////////////////////////////////

void summarySD(float offset)  {

  File dataFile2 = SD.open(nameFileSummary, FILE_WRITE); 
  dataFile2.println("\"Time\",\"T Lower Cold Plate\",\"T Lower Hot Plate\",\"T Upper Cold Plate\",\"Thermal Conductivity\"");
  Acquisition(offset, dataFile2);
  dataFile2.close();
}

