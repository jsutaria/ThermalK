//define the pins for the thermistors
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

/////////////////////////////////////
//  SYSTEM defined variables
/////////////////////////////////////

float display_delay = 0.2;  //in seconds - refresh time in serial monitor
float TmediumInitial = 0.0;


/////////////////////////////////////
//  SETUP routine
/////////////////////////////////////
void setup() { 
  Serial.begin(9600);
    
  TmediumInitial = Tread(therm3);
  Serial.println("(1) Start Acquisition; (2) Stop acquisition; (3) reset ambient DeltaT ambient ");
  Serial.println();
}


/////////////////////////////////////
//  Indefinite loop
/////////////////////////////////////

void loop() {
   
  int inSerial = 0;    // variable for serial input
  
  if (Serial.available() > 0) {
    
    Serial.println("\"Time\",\"T Lower Cold Plate\",\"T Lower Hot Plate\",\"T Upper Cold Plate\",\"Thermal Conductivity\""); 
    inSerial = Serial.read();

    if(inSerial==49)
      { float offset = (float) millis()/1000;
      while(inSerial!=50)
        {
        inSerial = Serial.read(); 

        Acquisition(offset);
        if(inSerial==51) 
           {
              break;

           }
        }   
       Serial.println();
      Serial.println(); 
      }
  }
}



/////////////////////////////////////////////////////
// Acquisition Routine
/////////////////////////////////////////////////////

void Acquisition(float offset) {
   
  Serial.print((float) millis()/1000 - offset);
  Serial.print(", ");
 
  float T1 = Tread(therm1);
  float T2 = Tread(therm2);
  float T3 = Tread(therm3);
  
  Serial.print(T1);
  Serial.print(", ");
  Serial.print(T2);
  Serial.print(", ");
  Serial.print(T3);
  //Serial.println();
  
  
  // parameters of thermal conductivity measurement
  // Q = heat input (watts)
  // L = thickness of sample
  // A = surface area/contact area of sample
  // deltaT = change in Temp from hot to cold side of sample
  float Q = 10.11;
  float L = 0.00635;
  float A = 0.00316692;
  float deltaT = T3 - T1;
  float conductivity = (Q*L)/(A*deltaT);
  
  Serial.print(", ");
  Serial.print(conductivity);
  Serial.println();
  
   delay(display_delay*1000);              // wait for display refresh
}


/////////////////////////////////////////////////////
// Read temperature from thermistor  
/////////////////////////////////////////////////////

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
