//ISRU Control (Arduino)

// Last updated: 22/08/2020, 15:21, GC
//-------------------------------------------------------------

//CHECK PIN CAPABILITES
//Pin allocations 
//Digital
#define E_STOP 3
#define RED 6
#define ORANGE 7
#define GREEN 8

#define MAGNET 9
#define HEATER 10
#define ELECTRO 11

#define CAM_SERVO 12

//Analogue
#define REACT_TEMP 2
#define MELT_TEMP 3

#define BATT_CURRENT 4
#define BATT_VOLTAGE 5
#define BUS_VOLTAGE 6
#define ELECTRO_CURRENT 7
#define SOLAR_CURRENT 8
#define LIGHT_LEVEL 11
//#define H2 0
//#define FLOW 1 

//Global Variables ------------------------------------
//-----------------------------------------------------

//Comms using simple command numbers

//Incoming:
int command;
char incomingChar = '0';
String incomingString;
String completeString;
bool newCommand = false;

// Outgoing: XX,[][][][];
static struct returnMessage{
  uint8_t commandRecvd; //8
  float data; //
} myMessageOut;

int msgLen = sizeof(myMessageOut);

//Circuit control:
static bool magnetOn = true;
static bool heaterOn= false;
static bool electroOn = false;

// Temperature - Thermistor
float reactionTemp;
int reactionVIn; // Voltage in from thermistor changes
float reactionR; //thermistor resistance
int R1 = 10000; //Voltage divider resistor value - reaction

//PT100
float heaterTemp;
int R2; //Voltage divider resistor value - heaters (TBC)

// Light Level
float voltageLDR;
int R3 = 10000; // Voltage divider resistor - LDR
float lux;

// Voltage/ current monitoring
float batteryCurrent;
float batteryVoltage;
float busVoltage;
float solarCurrent;
float electroCurrent;

// Setup ---------------------------------------
//----------------------------------------------
void setup(){
  
  //Initialise Digital Pins
  pinMode(E_STOP, INPUT);
  
  pinMode(RED, OUTPUT);
  pinMode(ORANGE, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(MAGNET, OUTPUT);
  pinMode(HEATER, OUTPUT);
  pinMode(ELECTRO, OUTPUT);
  pinMode(CAM_SERVO, OUTPUT);

  //Attach button interrupt
  attachInterrupt(0, eStop_ISR, FALLING);
  
  //Red LED ON indicates power is present
  digitalWrite(RED, HIGH);  
  
  //Attempt communication with Pi
  Serial.begin(115200); //Start Serial comms (baud rate)
  Serial.read(); // to clear buffer

  incomingChar = Serial.read();
  
  while(incomingChar != '!'){
      Serial.print("hello!");
      digitalWrite(ORANGE, HIGH);
      delay(250);
      digitalWrite(ORANGE,LOW);
      incomingChar = Serial.read();
      delay(250);
  }

  //Intiialise struct members
  myMessageOut.commandRecvd = 0;
  myMessageOut.data = -1;
  
  //Gets to here once hello! is received
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, HIGH); 
  Serial.println("Connection ready.");

}


// Loop ---------------------------------------------
//---------------------------------------------------
void loop(){
    //-----Get the most recent sensor values
    UpdateValues();

    //----------Receive Commands
    while(Serial.available() && !newCommand){
       incomingChar = Serial.read();
       
       if (incomingChar == ';'){
         completeString = incomingString;
         command = completeString.toInt();
	  myMessageOut.commandRecvd = command;
          newCommand = true; 
          digitalWrite(ORANGE, HIGH);
          incomingString = "";
        }
       
       else if (incomingChar > 47 && incomingChar < 58) {
          incomingString += (incomingChar - '0');
       }
    }

    //--------Process request
    if(newCommand){
        processCommands(command);
        // Output in format {Command#: ValueAsASCII}
        Serial.write(0x7b);
        Serial.write(myMessageOut.commandRecvd); 
        Serial.write(0x3a);
        Serial.print(myMessageOut.data);
        Serial.write(0x7d);
        newCommand = false;
        command = 0;
        digitalWrite(ORANGE, LOW);
    }
}

// Our functions -----------------------------------
//--------------------------------------------------

void UpdateValues(){
  //Get ALL current sensor values, regardless of state
  getMeltingTemp(); 
  getReactionTemp(); 
  getLightLevel(); 
  /*getBatteryCurrent(); 
  getBatteryVoltage(); 
  getBusVoltage(); 
  getSolarCurrent(); 
  getElectroCurrent(); */
}
  
  
void processCommands(int command){
  //Get the command number from the data sent
  myMessageOut.commandRecvd = command;
  switch(command){
    case 0:
       // Whoops, error... flash the orange LED!
       myMessageOut.data = -1;
       flashLED(ORANGE);
       break;

    case 1:
       // Send Command number and TRUE (2 bytes)
       myMessageOut.data = 1;
       break;

     case 2:
       // Get all data available
       myMessageOut.data = 0;
       break;

     case 3:
       // Switch magnet
       switchFunction(MAGNET);
       magnetOn = !magnetOn;
       myMessageOut.data = magnetOn;
       break;

     case 4:
        // Switch heater
        digitalWrite(RED, HIGH);
        switchFunction(HEATER);
        heaterOn = !heaterOn;
        myMessageOut.data = heaterOn;
        break;

      case 5:
        // Switch Electrolysis
        digitalWrite(RED, HIGH);
        switchFunction(ELECTRO);
        electroOn = !electroOn;
        myMessageOut.data = electroOn;
        break;

      case 6:
        // Switch Red LED
        switchFunction(RED);
        myMessageOut.data = digitalRead(RED);
        break;

      case 7:
        //Switch Orange LED
        switchFunction(ORANGE);
        myMessageOut.data = digitalRead(ORANGE);
        break;

      case 8:
        //Switch Green LED
        switchFunction(GREEN);
        myMessageOut.data = digitalRead(GREEN);
        break;

      case 9:
        //Get Magnet status
        myMessageOut.data = magnetOn;
        break;

      case 10:
        //Get heater status
        myMessageOut.data = heaterOn;
        break;

      case 11:
        //Get electrolysis status
        myMessageOut.data = electroOn;
        break;

      case 12:
        //Get reaction temperature
        myMessageOut.data = getReactionTemp();
        break;

      case 13:
        //Get melting temperature
        myMessageOut.data = getMeltingTemp();
        break;

      case 21:
        //Get light level
        myMessageOut.data = getLightLevel();
        break;
    
      default:
      //Unexpected command number -> error
        flashLED(ORANGE);
        myMessageOut.data = -1;
        break;

      /* case 14:
      //Get battery current
      //** TODO
      break;

      case 15:
      //Get battery voltage
      //** TODO
      break;

      case 16:
      //Get bus voltage
      //**
      break;

      case 17:
      //Get electrolysis current
      //**
      break;

      case 18:
      //Get solar panel current
      //**
      break;

      case 19:
      //Get hydrogen ppm - NOT USED
      break;

      case 20:
      //Get gas flow - NOT USED
      break; */
    }
}

// CIRCUIT CONTROL - OUTPUTS --------------------------------------
void flashLED(int pin){
  digitalWrite(pin, HIGH);
  delay(500);
  digitalWrite(pin, LOW);
  delay(500);
  digitalWrite(pin, HIGH);
  delay(500);
  digitalWrite(pin, LOW);
  delay(500);
}

void switchFunction(int pin){
//Invert digital pin state - switch OFF if ON, vice versa
  digitalWrite(pin, !digitalRead(pin));
}

  
// SENSOR DATA - INPUTS ---------------------------------------------------
float getReactionTemp(){
// Read the temperature value of the reaction temperature probe
  //need to convert change in resistance/voltage to temperature
  float coeffA1 = 2.1085081e-03; //Steinhart-Hart A,B,C Coefficients (thermistor)
  float coeffB1 = 0.7979204e-04;
  float coeffC1 = 6.5350763e-07; 

  R1 = 10000; // Other resistor value set to 10kohm
  reactionVIn = analogRead(REACT_TEMP);
  reactionR = R1*(1023.0 / (float)reactionVIn - 1.0);
  float logrR = log(reactionR);
  
  reactionTemp = 1.0 / (coeffA1 + coeffB1*logrR + coeffC1*logrR*logrR*logrR);  // Steinhart and Hart
  reactionTemp = reactionTemp - 273.15; //Kelvin to Celsius
  
  return reactionTemp;
}

float getMeltingTemp(){
// Read the temperature value of the filter temperature probe
    float coeffA2; //Steinhart-Hart A,B,C Coefficients (RTD)
    float coeffB2;
    float coeffC2; 
    R2 = 10000; //10kohm resistor value TBC
    
  return 69.0;
}


float getLightLevel(){
  // Get rough light level from LDR
  voltageLDR = analogRead(LIGHT_LEVEL) * (100 / 1024.0); // Give a percentage input across 1024 ADC levels
}

//eStop Interrupt is called when button is pressed
void eStop_ISR(){
  Serial.println("ESTOP PRESSED!");
  //Switch off heater and electrolysis
  digitalWrite(HEATER, LOW);
  digitalWrite(ELECTRO, LOW); 
  heaterOn= false;
  electroOn = false;
  
  //Indicate emergency on LEDs
  digitalWrite(GREEN, LOW);
  digitalWrite(ORANGE, HIGH); 
}

//No longer used ---------------------
float getHydrogenPPM(){ return 0.0;}
float getGasFlow(){ return 0.0;}
//Not yet implemented ----------------
float getBatteryCurrent(){ return 0.0;}
float getBatteryVoltage(){ return 0.0; }
float getBusVoltage(){ return 0.0; }
float getSolarCurrent(){ return 0.0; }
float getElectroCurrent(){ return 0.0;  }
void sendAllData(){}

  
  
  
