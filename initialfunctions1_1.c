#include <avr/pgmspace.h>
#include <EEPROM.h>

const unsigned char NUM_OF_PARAMETERS = 24;
const unsigned char NUM_OF_ROTARYS = 6;
const unsigned char NUM_OF_PRESETS = 4;
const unsigned char TAPER_RESOLUTION = 128;
const unsigned char INIT_PARA_VALS = 125;
const unsigned char logTaperType[TAPER_RESOLUTION] PROGMEM = {};   // will hold an array for log taper adjustment
const unsigned char anti_logTaperType[TAPER_RESOLUTION] PROGMEM = {};   // will hold an array for anti-log taper adjstment
const unsigned char rotaryNoChange[NUM_OF_ROTARYS] = {0, 0, 0, 0, 0, 0};     // a template to check against the current rotary vals, if they are equal then there was no movement on any of the encoders
const unsigned char ROTARY_STARTUPVALS = 0;
const unsigned char PRESET_SIZE = 74;

signed char rotaryVals[NUM_OF_ROTARYS];   // current incremental info on the rotary encoders(how much they've all turned since last loop iteration) these will be added or subtracted from selected parameters rawValue
unsigned char defVals[NUM_OF_PARAMETERS];
boolean defStates[NUM_OF_PARAMETERS];
unsigned char xyInputVals[2];  // holds the current values of the xy touch pad
unsigned char btnStatesOne;    // holds a byte of data where each bit represents a button state
unsigned char btnStatesTwo;    // holds a byte of data where each bit represents a button state
unsigned char pageNum;         // holds the current page number
unsigned char xAssignment;
unsigned char yAssignment;

int rawVals[NUM_OF_PARAMETERS];     // holds the untapered values of the current parameter[24] values
unsigned char presetNum; // holds the number of the current preset

typedef struct preset {       // holds the nessassary info for changing to differant presets, holds the last saved stateSave of specific preset number
    unsigned char defaultVals[NUM_OF_PARAMETERS];
    boolean defaultStates[NUM_OF_PARAMETERS];
    unsigned char rawValues[NUM_OF_PARAMETERS];
    unsigned char xAssign;
    unsigned char yAssign;
  } preset; 
  
struct preset allPresets[4] PROGMEM;   //presets will actually go to eeprom so they can be altered by the user( will need to use a function to set to and read from eeprom)

void setup() {

  
}

void loop() {

  

}
                                // example serial input {0, 0, 0, 0, -1, 0, 125, 0, b00011000, b10011001 } total of ten bytes
  void parseSerialData() {     // takes incoming serial data and parses it to correct variables for processing
    if (Serial.available() > 0) {
      for(unsigned char i=0; i<NUM_OF_ROTARYS; i++) {
        rotaryVals[i] = Serial.read();    //  takes first 6 bytes and puts them in the rotaryVals array
      };
      for(unsigned char i=0; i<2; i++) {
        xyInputVals[i]= Serial.read();    // takes the 7-8th bytes and puts them in the xyInputsVals array
      };
      btnStatesOne = Serial.read();       // takes the 9th byte and puts it in the btnStatesOne variable 
      btnStatesTwo = Serial.read();       // takes the 10th byte and puts it in the btnStatesTwo variable          
    };
  };
  
  void setPageNum() {
    bitWrite(pageNum, 0, bitRead(btnStatesOne, 0));
    bitWrite(pageNum, 1, bitRead(btnStatesOne, 1));
  };
  
//  the two functions below the following two are better I think...
//  
//  bool rotarysChangeState() {
//    
//    for(unsigned char i=0; i<6;i++) {
//      if(rotaryVals[i] != rotaryNoChange[i]) {
//        return true;
//      } else {
//          return false;
//        };
//    };
//  };
//  //  void updateRawVals(unsigned char rawValsArray[]) {
//   //boolean rotaryChangeState = rot
//    if (rotarysChangeState()) {
//      for(unsigned char i=0; i<6; i++) {
//        rawVals[pageNum*6+i] = rotaryVals[i];
//      };
//    };
//  };
   void rotarysChangeState() {    
      for(unsigned char i=0; i<NUM_OF_ROTARYS;i++) {
        if(rotaryVals[i] != rotaryNoChange[i]) {
          setRawVals(i);
        }; 
      };
  };
  void setRawVals(unsigned char counter) {
    int rawValsHolder = rawVals[pageNum*NUM_OF_ROTARYS+counter]; 
    rawValsHolder += rotaryVals[counter];
    if(rawValsHolder > 255) {    // made rawVals an int so if it goes above 255 or below 0 it doesn't do wierd things it just goes above 255 or below 0 
      rawValsHolder = 255;       // and then if it is above of below u.c. char levels it sets it back to limits(0-255)
    } else if(rawValsHolder < 0) {
      rawValsHolder = 0;
    };
    rawVals[pageNum*NUM_OF_ROTARYS+counter] = rawValsHolder; // did this cuz not sure if (rawValsHolder = rawVals[pageNum*6+counter]) just copies rawVals[pageNum*6+counter] or allows direct minipulation of rawVals[pageNum*6+counter]
  };

  
  void xyAssignmentCheck() {
    static unsigned char xOrYFirst = 0;
    static unsigned char lastXYState = 0;
    static unsigned char currentXYState = 0;  // static so it doesn't instanciate each iteration???...maybe...idk
    bitWrite(currentXYState, 0, bitRead(btnStatesTwo, 0)); // 0 bit = xAssign
    bitWrite(currentXYState, 1, bitRead(btnStatesTwo, 1)); // 1 bit = yAssign
    switch (currentXYState) {
      case 0:
      //do nothing
      //xOrYFirst = 0;
      lastXYState = currentXYState;
      break;
      case 1:
      // check for default btn click 
      unsigned char btnClick = checkForBtnClick(); // need to make function
      assignX(btnCick);                             // also need to make function
      xOrYFirst = 1;
      lastXYState = currentXYState;
      break;
      case 2:
      // ckeck for default btn click
      unsigned char btnClick = checkForBtnClick(); // need to make function returns a numeric code(1-6)
      assignY(btnCick);                             // also need to make function
      xOrYFirst = 2;
      lastXYState = currentXYState;

      break;
      case 3:
        //do something when var equals 1
        unsigned char btnClick = checkForBtnClick(); // need to make function returns a numeric code(1-6)
        switch (lastXYState) {
          case 0:
            assignX(btnCick);                             // also need to make function
            xOrYFirst = 1;
            lastXYState = currentXYState;
          break;
          case 1:
            assignY(btnCick)                             // also need to make function
            lastXYState = currentXYState;
            break;
          case 2:
            assignX(btnCick)                             // also need to make function
            lastXYState = currentXYState;
            break;
          case 3:
            if(xOrYFirst=1){
              assignY(btnClick);
            }else if(xOrYFirst = 2){
              assignX(btnCick); 
            };                                          // also need to make function
            lastXYState = currentXYState;
            break;
      };
    };
  };
  
  void setPreset() {
    // defaultVals[24];
   //defaultStates[24];
    // rawValues[24];
    for(unsigned char i=0; i<NUM_OF_PARAMETERS;) {      
        EEPROM.write(presetNum*PRESET_SIZE+i, defVals[i]);
        EEPROM.write(NUM_OF_PARAMETERS+(pesetNum*PRESET_SIZE+i), defStates[i]);
        EEPROM.write((NUM_OF_PARAMETERS*2)+(pesetNum*PRESET_SIZE+i), rawVals[i]);        
    };
        EEPROM.write((NUM_OF_PARAMETERS*3)+(pesetNum*PRESET_SIZE), xAssignment);
        EEPROM.write((NUM_OF_PARAMETERS*3)+(pesetNum*PRESET_SIZE)+1, yAssignment);
        
  };
  
  
  
