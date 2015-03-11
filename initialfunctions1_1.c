#include <avr/pgmspace.h>
#include <EEPROM.h>

const unsigned char NUM_OF_PARAMETERS = 24;
const unsigned char NUM_OF_ROTARYS = 6;
const unsigned char NUM_OF_PRESETS = 4;
const unsigned char TAPER_RESOLUTION = 128;
const unsigned char INIT_PARA_VALS = 125;
const unsigned char LOG_TAPER_TYPE[TAPER_RESOLUTION] PROGMEM = {};   // will hold an array for log taper adjustment
//const unsigned char TAPER_ARRAY[3][256];
const unsigned char ANTI_LOG_TAPER_TYPE[TAPER_RESOLUTION] PROGMEM = {};   // will hold an array for anti-log taper adjstment
const unsigned char ROTARY_NO_CHANGE[NUM_OF_ROTARYS] = {0, 0, 0, 0, 0, 0};     // a template to check against the current rotary vals, if they are equal then there was no movement on any of the encoders
const unsigned char ROTARY_STARTUPVALS = 0;
const unsigned char PRESET_SIZE = NUM_OF_PARAMETERS*3+2;

const unsigned char BTN_CLK_1_BIT = 7;
const unsigned char BTN_CLK_2_BIT = 6;
const unsigned char BTN_CLK_3_BIT = 5;
const unsigned char BTN_CLK_4_BIT = 4;
const unsigned char BTN_CLK_5_BIT = 3;
const unsigned char BTN_CLK_6_BIT = 2;
const unsigned char KILL_BIT = 1;

const unsigned char ANTI_KILL_BIT = 0;
const unsigned char TUNER_BIT = 7;
const unsigned char PAGE_TWO_BIT = 6;
const unsigned char PAGE_ONE_BIT = 5;
const unsigned char SAVE_PRESET_BIT = 4;
const unsigned char SAVE_DEFAULT_BIT = 3;
const unsigned char XY_HOLD_BIT = 2;
const unsigned char ASSIGN_X_BIT = 1;
const unsigned char ASSIGN_Y_BIT = 0;

signed char rotaryVals[NUM_OF_ROTARYS];   // current incremental info on the rotary encoders(how much they've all turned since last loop iteration) these will be added or subtracted from selected parameters rawValue
unsigned char defVals[NUM_OF_PARAMETERS];
boolean defStates[NUM_OF_PARAMETERS];
unsigned char xYInputVals[4];  // holds the current values of the xy touch pad and last
unsigned char btnStatesOne;    // holds a byte of data where each bit represents a button state
unsigned char btnStatesTwo;    // holds a byte of data where each bit represents a button state
unsigned char pageNum;         // holds the current page number
unsigned char xAssignment;
unsigned char yAssignment;
int rawVals[NUM_OF_PARAMETERS];     // holds the untapered values of the current parameter[24] values
unsigned char presetNum; // holds the number of the current preset
unsigned char taperedVals[NUM_OF_PARAMETERS];
unsigned char outputVals[NUM_OF_PARAMETERS];

//typedef struct preset {       // holds the nessassary info for changing to differant presets, holds the last saved stateSave of specific preset number
//    unsigned char defaultVals[NUM_OF_PARAMETERS];
//    boolean defaultStates[NUM_OF_PARAMETERS];
//    unsigned char rawValues[NUM_OF_PARAMETERS];
//    unsigned char xAssign;
//    unsigned char yAssign;
//  } preset; 
//  
//struct preset allPresets[4] PROGMEM;   //presets will actually go to eeprom so they can be altered by the user( will need to use a function to set to and read from eeprom)

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
        xYInputVals[i]= Serial.read();    // takes the 7-8th bytes and puts them in the xyInputsVals array
      };
      btnStatesOne = Serial.read();       // takes the 9th byte and puts it in the btnStatesOne variable 
      btnStatesTwo = Serial.read();       // takes the 10th byte and puts it in the btnStatesTwo variable          
    };
  };
  
  void setPageNum() {
    bitWrite(pageNum, 0, bitRead(btnStatesOne, PAGE_ONE_BIT));
    bitWrite(pageNum, 1, bitRead(btnStatesOne, PAGE_TWO_BIT));
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
      for(unsigned char i=0; i<NUM_OF_ROTARYS; i++) {
        if(rotaryVals[i] != ROTARY_NO_CHANGE[i]) {
          setRawVals(i);
        }; 
      };
  };
  
  boolean isXYTouchActive() {
    if(xYInputVals[0]> 2 || xYInputVals[1] > 2){ /// check to see if your touching the touch padf and if not it is disabled and bypassed 
      return true;  // greater then values will be based on what the xypad rest at when not touched
    } else
        return false;    
  };
  
  void setXYVals() {
    if(isXYTouchActive()) {
      if(checkXYHoldState()) {
      setXYHold();
    } else {
      rawVals[xAssignment] = xYInputVals[0];
      rawVals[yAssignment] = xYInputVals[1];
      xYInputVals[2] = xYInputVals[0];
      xYInputVals[3] = xYInputVals[1];
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
  
  unsigned char checkForBtnClick() {
    for(unsigned char i = 2; i<8; i++) {
      if(bitRead(btnStatesOne, i)) {
        return i-1;
      };
    };
    return 0;
  };
  
  boolean checkForBtnClickBoolean() {
    for(unsigned char i = 2; i<8; i++) {
      if(bitRead(btnStatesOne, i)) {
        return i;
      };
    };
    return 0;
  };
  
  void assignX() {
    unsigned char btnClk = checkForBtnClick(); 
    if(btnClk > 0) {
      xAssignment = btnClk;
    };
  };
  
  void assignY() {
    unsigned char btnClk = checkForBtnClick(); 
    if(btnClk > 0) {
      yAssignment = btnClk;
    }; 
  }
  
  boolean checkXYAsignState() {
    boolean xState = bitRead(btnStatesTwo, ASSIGN_X_BIT);
    boolean yState = bitRead(btnStatesTwo, ASSIGN_Y_BIT);
    if(xState)
      return true;
    else if(yState) 
      return true;
    else 
      return false;   
  };
  
  void xyAssignmentCheck() {
    static unsigned char xOrYFirst = 0;
    static unsigned char lastXYState = 0;
    static unsigned char currentXYState = 0;  // static so it doesn't instanciate each iteration???...maybe...idk
    static unsigned char btnCLick = 0;
    bitWrite(currentXYState, 0, bitRead(btnStatesTwo, ASSIGN_X_BIT)); // 0 bit = xAssign
    bitWrite(currentXYState, 1, bitRead(btnStatesTwo, ASSIGN_Y_BIT)); // 1 bit = yAssign
    switch (currentXYState) {
      case 0:
      //do nothing
      //xOrYFirst = 0;
      lastXYState = currentXYState;
      break;
      case 1:
      // check for default btn click 
      assignX();                             // also need to make function
      xOrYFirst = 1;
      lastXYState = currentXYState;
      break;
      case 2:
      // ckeck for default btn click
      assignY();                             // also need to make function
      xOrYFirst = 2;
      lastXYState = currentXYState;

      break;
      case 3:
        //do something when var equals 1
        switch (lastXYState) {
          case 0:
            assignX();                             // also need to make function
            xOrYFirst = 1;
            lastXYState = currentXYState;
          break;
          case 1:
            assignY();                             // also need to make function
            lastXYState = currentXYState;
            break;
          case 2:
            assignX();                             // also need to make function
            lastXYState = currentXYState;
            break;
          case 3:
            if(xOrYFirst=1){
              assignY();
            }else if(xOrYFirst = 2){
              assignX(); 
            };                                          // also need to make function
            lastXYState = currentXYState;
            break;
      };
    };
  };
  
  boolean checkPresetState() {
    return bitRead(btnStatesTwo, SAVE_PRESET_BIT);
  };
  
  void setPreset() {
    if (checkPresetState()) {
      for(unsigned char i=0; i<NUM_OF_PARAMETERS; i++) {      // copies 74 bytes of data to eeprom
        EEPROM.write(presetNum*PRESET_SIZE+i, defVals[i]);
        EEPROM.write(NUM_OF_PARAMETERS+(presetNum*PRESET_SIZE+i), defStates[i]);
        EEPROM.write((NUM_OF_PARAMETERS*2)+(presetNum*PRESET_SIZE+i), rawVals[i]);        
      };
        EEPROM.write((NUM_OF_PARAMETERS*3)+(presetNum*PRESET_SIZE), xAssignment);
        EEPROM.write((NUM_OF_PARAMETERS*3)+(presetNum*PRESET_SIZE)+1, yAssignment);
    };    
  };
  
  boolean checkSaveDefState() {
    return bitRead(btnStatesTwo, SAVE_DEFAULT_BIT);
  };
  
  void setDefaultVals() {
    if(checkSaveDefState()) {
      for(unsigned char i = 0; 0<NUM_OF_PARAMETERS; i++) {
        if(defStates[i] = false) {
          defVals[i] = rawVals[i];
        };
      };
    };
      
  };
  
  void handleDefaultStates() {
    if (checkXYAsignState() != true) {
      if(checkForBtnClickBoolean()) {
        for(unsigned char i = 0; i<NUM_OF_ROTARYS; i++) {
          defStates[pageNum*6+i] = bitRead(btnStatesOne, i+2);
        };
      };
    };
  };
  
  boolean checkTunerState() {
   return bitRead(btnStatesTwo, TUNER_BIT);
  };
  
  void setTuner() {
    if(checkTunerState()) {
      /// what to doooooo?
    };
  };
  
  boolean checkXYHoldState() {
    return bitRead(btnStatesTwo, XY_HOLD_BIT);
  };
  
  void setXYHold() {    
      rawVals[xAssignment] = xYInputVals[2];
      rawVals[yAssignment] = xYInputVals[3];
  };
  
  boolean checkKillState() {
    return bitRead(btnStatesOne, KILL_BIT);
  };
  
  boolean checkAntiKillState() {
    return bitRead(btnStatesOne, ANTI_KILL_BIT);
  };
  
  void setOutputVals() {
    unsigned char killState = checkKillState();
    unsigned char antiKillState = checkAntiKillState();
    for(unsigned char i = 0; i<NUM_OF_PARAMETERS; i++) {
      outputVals[i] = taperedVals[i];
    };    
    killAntiKillLogic(killState, antiKillState);
  };
    void killAntiKillLogic(unsigned char killState, unsigned char antiKillState) {
      static unsigned char killAntiNumCode = 0;
      bitWrite(killAntiNumCode, 2, bitRead(killAntiNumCode, 0));
      bitWrite(killAntiNumCode, 3, bitRead(killAntiNumCode, 1));
      bitWrite(killAntiNumCode, 0, killState);
      bitWrite(killAntiNumCode, 1, antiKillState);
      
    };
  
