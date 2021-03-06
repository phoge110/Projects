//============================================================================
//Notes
//============================================================================
// 1. Please set UART buffer to 256 before uploading. Refer to
//    http://www.hobbytronics.co.uk/arduino-serial-buffer-size
//    for reference. The files might be in a different location depending on
//    what version of Arduino you use. 


//============================================================================
//Includes
//============================================================================
#include "TimerOne.h"


//============================================================================
//Definitions
//============================================================================

//Fans
#define FAN1_CTRL 3
#define FAN2_CTRL 4
#define FAN3_CTRL 5
#define FAN4_CTRL 6
#define FAN5_CTRL 7
#define FAN6_CTRL 8
#define FAN7_CTRL 9
#define FAN8_CTRL 10

//LEDs
#define LED_POWER 30        //Red LED
#define LED_OVERTEMP 32     //Orange LED
#define LED_CURRENT_ON 34   //Yellow LED 
#define LED_CYCLE_ON 36     //Green LED
#define LED_COOLING 38      //Blue LED
#define LED_SMOKE_ALARM 40  //Purple LED


//EMO
#define EMO_SW 48

//Interlocks
#define POWER_SUPPLY_INTERLOCK 14
#define FAN_POWER_INTERLOCK 15


//UART
#define MINIMUM_UART_BUFFER_LENGTH 5
#define UART_BUFFER 256
#define HEARTBEAT_LENGTH 5
#define BAUD_RATE_THERMAL_CONTROLLER 115200
#define BAUD_RATE_PC 115200

//Misc
#define TIMED_LOOP_DURATION_US 1000000
#define new_max(x,y) ((x) >= (y)) ? (x) : (y)
#define NUMBER_OF_SMOKE_SENSORS 8
#define NUMBER_OF_TEMP_SENSORS 16




//============================================================================
//Declarations
//============================================================================

//Target fan speed
int TargetFanPWMSpeeds[8];

//Master data arrays
float floTemp[NUMBER_OF_TEMP_SENSORS];
float floSmokeLevel[NUMBER_OF_SMOKE_SENSORS];
bool bolSmokeAlarmOn = false;
bool bolOverTempAlarmOn = false;
bool bolCurrentOn = false;
bool bolCycleOn = false;
bool bolCooling = false;
String strSmokeAlarm;
String strOverTempAlarm;
bool bolThermalControllerHeartBeatGood = false;
String strTempSensorToUse;
String strSmokeSensorToUse;
bool bolTempSensorToUse[NUMBER_OF_TEMP_SENSORS];

//Trip levels
float floSmokeSensorTripLevel = 3.0; 
float floOverTempDegC = 100; 

//Calculate PWM
int intBiasCurrentOnTemp = 85;
int intBiasCurrentOffTemp = 25;
int intBiasCurrentStatus = 0;
int intPauseFans = 0;

//UART
char chrPCData[UART_BUFFER];
char chrThermalControllerData[UART_BUFFER];
boolean newData = false;
char startMarker = '<';
char endMarker = '>';
int intPCHeartbeatCounter = 0;
int intThermalControllerHeartbeatCounter = 0;
String strDebug;

//Misc
int intOperating = 0;


//============================================================================
//Function: float floCalculateMaxEnabledTemp (float floArray[], int arraySize )
//Notes: returns the largest number in the temperature array
//============================================================================
int floCalculateMaxEnabledTemp (float floArray[], int arraySize ){
  //Declarations
  float floValue=-1000;

  //Finds the max value in the array
  for(int i=0;i<arraySize;i++) {
    //Checks for max value and if the temp sensor is in use
    if((floArray[i] > floValue) && (bolTempSensorToUse[i] == true)) {
      floValue = floArray[i];
    }  
  }

  //Returns the maximum value
  return floValue;
}
//============================================================================
//Function: void execEmergencyAction (bool isEmergency)
//Notes: executes actions based on whether there is an emergency detected
//============================================================================
void execEmergencyAction (bool bolIsEmergency){
  //Checks to see if there is an emergency
  if (bolIsEmergency == true){
    //Executes emergency action
    intOperating = 0;
    setPowerSupplyInterlock(false);
    setFanPowerInterlock(false);
    SetFanPWM (0,0,0,0,0,0,0,0);
    bolCurrentOn = false;
    bolCycleOn = false;
    bolCooling = false;
  }
}


//============================================================================
//Function: bool isEMOPressed()
//Notes: returns if the emergency power off is pressed
//============================================================================
bool isEMOPressed(){
  //Reads the EMO switch
  if (digitalRead(EMO_SW) == HIGH){
    //This means stop; EMO switch has been pressed
    return false; 
  }
  else{
    //This means run, nothing has been pressed
    return true;
    
  }
}

//============================================================================
//Function: void setPowerSupplyInterlock(bool bolEnablePowerSupply)
//Notes: sets the interlock for the power supplies
//============================================================================
void setPowerSupplyInterlock(bool bolEnablePowerSupply){
  //Turns the power supply interlock depending on the input
  if (bolEnablePowerSupply == false){
    digitalWrite(POWER_SUPPLY_INTERLOCK, LOW);
    }
  else{
    digitalWrite(POWER_SUPPLY_INTERLOCK, HIGH);
  }
  //DEBUG
  digitalWrite(LED_CURRENT_ON,digitalRead(POWER_SUPPLY_INTERLOCK));
}

//============================================================================
//Function: void setFanPowerInterlock(bool bolFanPowerEnable)
//Notes: enable/disable the fan power
//============================================================================
void setFanPowerInterlock(bool bolFanPowerEnable){
  //Enables/disables the fan
  if (bolFanPowerEnable == false){
    digitalWrite(FAN_POWER_INTERLOCK, LOW);
  }
  else{
    digitalWrite(FAN_POWER_INTERLOCK, HIGH);
  }
}

//============================================================================
//Function: void SetFanPWM (int Fan1PWM, int Fan2PWM, int Fan3PWM, int Fan4PWM, int Fan5PWM, int Fan6PWM, int Fan7PWM, int Fan8PWM)
//Notes: FanXPWM, a value between 0 to 100. 8 inputs total
//============================================================================
void SetFanPWM (int Fan1PWM, int Fan2PWM, int Fan3PWM, int Fan4PWM, int Fan5PWM, int Fan6PWM, int Fan7PWM, int Fan8PWM){
  //Performs saturation - 100%
  if (Fan1PWM > 100){Fan1PWM = 100;} 
  if (Fan2PWM > 100){Fan2PWM = 100;} 
  if (Fan3PWM > 100){Fan3PWM = 100;} 
  if (Fan4PWM > 100){Fan4PWM = 100;} 
  if (Fan5PWM > 100){Fan5PWM = 100;} 
  if (Fan6PWM > 100){Fan6PWM = 100;} 
  if (Fan7PWM > 100){Fan7PWM = 100;} 
  if (Fan8PWM > 100){Fan8PWM = 100;} 

  //Performs saturation - 0%
  if (Fan1PWM < 0){Fan1PWM = 0;} 
  if (Fan2PWM < 0){Fan2PWM = 0;} 
  if (Fan3PWM < 0){Fan3PWM = 0;} 
  if (Fan4PWM < 0){Fan4PWM = 0;} 
  if (Fan5PWM < 0){Fan5PWM = 0;} 
  if (Fan6PWM < 0){Fan6PWM = 0;} 
  if (Fan7PWM < 0){Fan7PWM = 0;} 
  if (Fan8PWM < 0){Fan8PWM = 0;} 

  
  //Sets the PWMs
  analogWrite(FAN1_CTRL, (int)((float)Fan1PWM/100*255));
  analogWrite(FAN2_CTRL, (int)((float)Fan2PWM/100*255));
  analogWrite(FAN3_CTRL, (int)((float)Fan3PWM/100*255));
  analogWrite(FAN4_CTRL, (int)((float)Fan4PWM/100*255));
  analogWrite(FAN5_CTRL, (int)((float)Fan5PWM/100*255));
  analogWrite(FAN6_CTRL, (int)((float)Fan6PWM/100*255));
  analogWrite(FAN7_CTRL, (int)((float)Fan7PWM/100*255));
  analogWrite(FAN8_CTRL, (int)((float)Fan8PWM/100*255));

  //Sets the target fan speed global variable
  TargetFanPWMSpeeds[0] = Fan1PWM;
  TargetFanPWMSpeeds[1] = Fan2PWM;
  TargetFanPWMSpeeds[2] = Fan3PWM;
  TargetFanPWMSpeeds[3] = Fan4PWM;
  TargetFanPWMSpeeds[4] = Fan5PWM;
  TargetFanPWMSpeeds[5] = Fan6PWM;
  TargetFanPWMSpeeds[6] = Fan7PWM;
  TargetFanPWMSpeeds[7] = Fan8PWM;
}


//======================================================================
//Function: CalculatePWM
//Inputs: none
//Outputs: none
//Description: Calculates the PWM duty cycle of the fans
//======================================================================
void CalculatePWM(void){
  int Fan1PWM, Fan2PWM, Fan3PWM, Fan4PWM, Fan5PWM, Fan6PWM, Fan7PWM, Fan8PWM;
  float floMaxTemp = 0;

  //Finds the maximum temperature
  floMaxTemp = floCalculateMaxEnabledTemp(floTemp,NUMBER_OF_TEMP_SENSORS);
    
  //Checks to see if the bias current is on or not
  if (intBiasCurrentStatus == 0){
    
    
    //Regulates fans to no current temp
    Fan1PWM = (floMaxTemp - intBiasCurrentOffTemp + 5)*10;
    Fan2PWM = (floMaxTemp - intBiasCurrentOffTemp + 5)*10;
    Fan3PWM = (floMaxTemp - intBiasCurrentOffTemp + 5)*10;
    Fan4PWM = (floMaxTemp - intBiasCurrentOffTemp + 5)*10;
    Fan5PWM = (floMaxTemp - intBiasCurrentOffTemp + 5)*10;
    Fan6PWM = (floMaxTemp - intBiasCurrentOffTemp + 5)*10;
    Fan7PWM = (floMaxTemp - intBiasCurrentOffTemp + 5)*10;
    Fan8PWM = (floMaxTemp - intBiasCurrentOffTemp + 5)*10;
    
  }
  else{
  
    //Regulates fans to bais current temp
    Fan1PWM = (floMaxTemp - intBiasCurrentOnTemp + 5)*10;
    Fan2PWM = (floMaxTemp - intBiasCurrentOnTemp + 5)*10;
    Fan3PWM = (floMaxTemp - intBiasCurrentOnTemp + 5)*10;
    Fan4PWM = (floMaxTemp - intBiasCurrentOnTemp + 5)*10;
    Fan5PWM = (floMaxTemp - intBiasCurrentOnTemp + 5)*10;
    Fan6PWM = (floMaxTemp - intBiasCurrentOnTemp + 5)*10;
    Fan7PWM = (floMaxTemp - intBiasCurrentOnTemp + 5)*10;
    Fan8PWM = (floMaxTemp - intBiasCurrentOnTemp + 5)*10;
  }
  
  //Sets the PWM of all the fans if the fans are not in pause mode and we are operating
  if (intPauseFans == 1) {
    //Pauses the fans
    SetFanPWM(0,0,0,0,0,0,0,0);  
  }
  else{
    //strDebug = "0.00";
    //strDebug = intOperating;
    //Not paused, and check to see if we are operating
    if (intOperating == 1){
      //Set fans to the proper speed
      SetFanPWM(Fan1PWM, Fan2PWM,Fan3PWM,Fan4PWM, Fan5PWM, Fan6PWM, Fan7PWM, Fan8PWM);

      //strDebug = "1.00";
    }
  }
  
}


//============================================================================
//Function: void SendDataToThermalController()
//Notes:  writes the data from the thermal controller
//============================================================================
void SendDataToThermalController(){
  //Sends data to the thermal controller
  Serial2.print(floOverTempDegC);
  Serial2.print(",");
  Serial2.print(floSmokeSensorTripLevel);
  Serial2.print(",");
  Serial2.print(strTempSensorToUse);
  Serial2.print(",");
  Serial2.println(strSmokeSensorToUse);
}

//============================================================================
//Function: void ParseDataFromThermalController()
//Notes: parses the data from the thermal controller
//============================================================================
void ParseDataFromThermalController(char chrSerialData[UART_BUFFER]){
  //Declarations
  int x = 0;

  //Uses string tokenizer to parse the character array for temperature
  floTemp[0] = atof(strtok(chrSerialData,","));  
  for (x = 1; x < 16; x++){
     floTemp[x] = atof(strtok(NULL, ",")); 
  }

  //Parses smoke level
  for (x = 0; x < 8; x++){
     floSmokeLevel[x] = atof(strtok(NULL, ",")); 
  }

  //Parses the smoke alarm
  if (atoi(strtok(NULL, ",")) == 0){
    //No smoke alarm
    bolSmokeAlarmOn = false;
  }
  else{
    //Smoke alarm
    bolSmokeAlarmOn = true;

    //Disables HW check for smoke alarm
   // bolSmokeAlarmOn = false;
  }

  //Parses the temperature alarm
  if (atoi(strtok(NULL, ",")) == 0){
    //No temp alarm
    bolOverTempAlarmOn = false;
  }
  else{
    //Temp alarm
    bolOverTempAlarmOn = true;

    //Disables over temp alarm
    //bolOverTempAlarmOn = false;
  }

  //Clears the heart beat
  intThermalControllerHeartbeatCounter = 0;
  bolThermalControllerHeartBeatGood = true;
 }

//============================================================================
//Function: void sendDataToPC ()
//Notes: sends data to the PC
//============================================================================
void sendDataToPC(){
  //Sends all the temperature data
  for (int i = 0; i < NUMBER_OF_TEMP_SENSORS; i++){
    Serial.print(floTemp[i]);
    Serial.print(",");
  }

  //Sends all the smoke data
  for (int i = 0; i < NUMBER_OF_SMOKE_SENSORS; i++){
    Serial.print(floSmokeLevel[i]);
    Serial.print(",");
  }

  //Sends all the booleans
  Serial.print(bolOverTempAlarmOn);
  Serial.print(",");
  Serial.print(bolSmokeAlarmOn);
  Serial.print(",");
  Serial.print(isEMOPressed());
  Serial.print(",");
  Serial.print(bolThermalControllerHeartBeatGood);
  Serial.print(",");
  Serial.print(TargetFanPWMSpeeds[0]);
  Serial.print(",");
  Serial.print(TargetFanPWMSpeeds[1]);
  Serial.print(",");
  Serial.print(TargetFanPWMSpeeds[2]);
  Serial.print(",");
  Serial.print(TargetFanPWMSpeeds[3]);
  Serial.print(",");
  Serial.print(TargetFanPWMSpeeds[4]);
  Serial.print(",");
  Serial.print(TargetFanPWMSpeeds[5]);
  Serial.print(",");
  Serial.print(TargetFanPWMSpeeds[6]);
  Serial.print(",");
  Serial.println(TargetFanPWMSpeeds[7]);
  
  
  //Prints the debug
  /*Serial.print(",");
  Serial.println(strDebug);*/
  

}

//============================================================================
//Function: void CheckAlarmTripped()
//Notes:  Checks to see if alarm is tripped (smoke or overtemp)
//============================================================================
bool CheckAlarmTripped(){
  //Checks for smoke alarm
  if (bolSmokeAlarmOn == true){
    //Disables the power supplies and fans
    setPowerSupplyInterlock(false);
    setFanPowerInterlock(false);
  }

  //Checks for over temperature alarm
  if (bolOverTempAlarmOn == true){
    //Disables the power supplies and fans
    setPowerSupplyInterlock(false);
    setFanPowerInterlock(true);
  }
}

//============================================================================
//Function: void UpdateLEDDisplay()
//Notes:  updates the LED display
//============================================================================
void UpdateLEDDisplay(){
  //Turns on the LEDs based on their state
  digitalWrite(LED_POWER,HIGH);
  digitalWrite(LED_OVERTEMP,bolOverTempAlarmOn);
  digitalWrite(LED_CURRENT_ON,bolCurrentOn);
  digitalWrite(LED_CYCLE_ON,bolCycleOn);
  digitalWrite(LED_COOLING,bolCooling);
  digitalWrite(LED_SMOKE_ALARM,bolSmokeAlarmOn);
}

//============================================================================
//Function: void InitLED()
//Notes:  tests the LEDs upon power up to make sure they all work good
//============================================================================
void InitLED(){
  //Cute little scrolly sequence
  digitalWrite(LED_POWER,HIGH);
  digitalWrite(LED_OVERTEMP,LOW);
  digitalWrite(LED_CURRENT_ON,LOW);
  digitalWrite(LED_CYCLE_ON,LOW);
  digitalWrite(LED_COOLING,LOW);
  digitalWrite(LED_SMOKE_ALARM,LOW);
  delay(200);
  digitalWrite(LED_OVERTEMP,HIGH);
  delay(200);
  digitalWrite(LED_CURRENT_ON,HIGH);
  delay(200);
  digitalWrite(LED_CYCLE_ON,HIGH);
  delay(200);
  digitalWrite(LED_COOLING,HIGH);
  delay(200);
  digitalWrite(LED_SMOKE_ALARM,HIGH);
  delay(1000);
  digitalWrite(LED_POWER,HIGH);
  digitalWrite(LED_OVERTEMP,LOW);
  digitalWrite(LED_CURRENT_ON,LOW);
  digitalWrite(LED_CYCLE_ON,LOW);
  digitalWrite(LED_COOLING,LOW);
  digitalWrite(LED_SMOKE_ALARM,LOW);
}

//============================================================================
//Function: void ParseDataFromPC(char chrSerialData[UART_BUFFER])
//Notes:  parses data from the PC
//============================================================================
void ParseDataFromPC(char chrSerialData[UART_BUFFER]){
  //Uses string tokenizer to parse the data from the PC (sample: <75.0,3.5,70,50,1,0,0>)
  floOverTempDegC = atof(strtok(chrSerialData,","));  
  floSmokeSensorTripLevel = atof(strtok(NULL, ",")); 
  intBiasCurrentOnTemp = atoi(strtok(NULL, ",")); 
  intBiasCurrentOffTemp = atoi(strtok(NULL, ",")); 
  intBiasCurrentStatus = atoi(strtok(NULL, ",")); 
  intPauseFans = atoi(strtok(NULL, ","));  
  strTempSensorToUse = strtok(NULL, ",");
  strSmokeSensorToUse = strtok(NULL, ",");
  intOperating = atoi(strtok(NULL, ",")); 

  //Parses the temp sensor to use
  for (int i = 0; i < NUMBER_OF_TEMP_SENSORS; i++){
    if (strTempSensorToUse[i] == '1'){
      bolTempSensorToUse[i] = true;
      
    }
    else{
      bolTempSensorToUse[i] = false;
    }
  }

  //Clears the PC heart beat
  intPCHeartbeatCounter = 0;
}


//============================================================================
//Function: void StartStopCycle(void)
//Notes:  checks to see if a start is called from the PC
//============================================================================
void StartStopCycle(void){
  //Checks to see if the PC has initiated a start or a stop
  if (intOperating == 1){
    //Executes start action
    setPowerSupplyInterlock(true);
    setFanPowerInterlock(true);
    bolCurrentOn = false;
    bolCycleOn = false;
    bolCooling = false;
  }
  else{
    //Executes stop action
    setPowerSupplyInterlock(false);
    setFanPowerInterlock(false);
    SetFanPWM (0,0,0,0,0,0,0,0);
    bolCurrentOn = false;
    bolCycleOn = false;
    bolCooling = false;
  }
  
}


//============================================================================
//Function: void ReadDatafromPC()
//Notes:  reads data from the PC
//============================================================================
void ReadDatafromPC() {

    static boolean recvInProgress = false;
    static byte ndx = 0;
    char rc;

    //Reads the data from the serial port if there is data
    while (Serial.available() > 0 && newData == false) {
        //Gets a byte back from the serial port
        rc = Serial.read();

        //Checks to see if we are at a new line or in progress of constructing a line
        if (recvInProgress == true) {
            //Checks for end marker
            if (rc != endMarker) {
                chrPCData[ndx] = rc;
                ndx++;
                if (ndx >= UART_BUFFER) {
                    ndx = UART_BUFFER - 1;
                }
            }
            else {
                chrPCData[ndx] = '\0'; 
                recvInProgress = false;
                ndx = 0;

                //Parses the data
                ParseDataFromPC(chrPCData);
            }
        }
        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

//============================================================================
//Function: void ReadDatafromThermalController()
//Notes:  reads data from the thermal controller
//============================================================================
void ReadDatafromThermalController() {

    static boolean recvInProgress = false;
    static byte ndx = 0;
    char rc;

    //Reads the data from the serial port if there is data
    while (Serial2.available() > 0 && newData == false) {
        //Gets a byte back from the serial port
        rc = Serial2.read();

        //Checks to see if we are at a new line or in progress of constructing a line
        if (recvInProgress == true) {
            //Checks for end marker
            if (rc != endMarker) {
                chrThermalControllerData[ndx] = rc;
                ndx++;
                if (ndx >= UART_BUFFER) {
                    ndx = UART_BUFFER - 1;
                }
            }
            else {
                chrThermalControllerData[ndx] = '\0'; 
                recvInProgress = false;
                ndx = 0;

                //Parses the data
                ParseDataFromThermalController(chrThermalControllerData);
            }
        }
        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}


//======================================================================
//Function: TestHeartBeat
//Description: This tests the heart beat 
//======================================================================
void TestHeartBeat(){

  //Increments the heart beat
  intPCHeartbeatCounter++;
  intThermalControllerHeartbeatCounter++;

  //Checks to see if we have an expired heart beat for the thermal controller
  if (intThermalControllerHeartbeatCounter > (HEARTBEAT_LENGTH)){
    //Heartbeat fails - disables power supplies and fans
    bolThermalControllerHeartBeatGood = false;
    execEmergencyAction(true);
  }

  //Checks to see if we have an expired heart beat for the PC
  if (intPCHeartbeatCounter > HEARTBEAT_LENGTH){
    //Heartbeat fails - disables power supplies and fans
    //Serial.println("Heartbeat fail");
    execEmergencyAction(true);
  }
}


//======================================================================
//Function: TimedLoop
//Description: This function runs at the specified timer interval
//======================================================================
void TimedLoop(){
    //Tests the heart beat
    TestHeartBeat();

    //Checks to see if E-Stop is pressed
    execEmergencyAction(isEMOPressed());
    
    //Writes the data from the thermal controller
    SendDataToThermalController();

    //Checks to see if user is requesting a start or a stop
    StartStopCycle();

    //Calculates the fan PWM speed
    CalculatePWM();

    //Turns on the LEDs based on their state
    UpdateLEDDisplay();    

    //Sends the data to the PC
    sendDataToPC();

}




//============================================================================
//Function: void setup()
//Notes: code runs once
//============================================================================
void setup() {
  //Serial ports
  Serial.begin(BAUD_RATE_PC);
  Serial2.begin(BAUD_RATE_THERMAL_CONTROLLER);

  //Data direction
  pinMode(EMO_SW,INPUT);
  pinMode(POWER_SUPPLY_INTERLOCK,OUTPUT);
  pinMode(FAN_POWER_INTERLOCK,OUTPUT);
  pinMode(LED_POWER,OUTPUT);
  pinMode(LED_OVERTEMP,OUTPUT);
  pinMode(LED_CURRENT_ON,OUTPUT);
  pinMode(LED_CYCLE_ON,OUTPUT);
  pinMode(LED_COOLING,OUTPUT);
  pinMode(LED_SMOKE_ALARM,OUTPUT);

  //Makes sure the fan turns off
  SetFanPWM(0,0,0,0,0,0,0,0);

  //Initializes the LEDs
  InitLED();

  //Timer that calls function TimedLoop every TIMED_LOOP_DURATION_US uS 
  Timer1.initialize(TIMED_LOOP_DURATION_US);
  Timer1.attachInterrupt(TimedLoop);

}

//============================================================================
//Function: void loop()
//Notes: main loop, continually loops
//============================================================================
void loop() {
  //Recieve data from PC  
  ReadDatafromPC();

  //Recieve data from the thermal controller  
  ReadDatafromThermalController();
}

 
