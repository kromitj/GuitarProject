#include <avr/pgmspace.h>
#include <EEPROM.h>

///////////////////////////////////////////////////////////////////
///////////////////////////////////////   CONSTANT_VARIABLES
const unsigned char NUM_OF_PARAMETERS = 24;
const unsigned char NUM_OF_ROTARYS = 6;
const unsigned char NUM_OF_PRESETS = 4;
const unsigned char PARA_TAPER_TYPES[NUM_OF_PARAMETERS] = {0, 0, 0, 1, 0, 2, 1, 1, 1, 0, 0, 2, 2, 1, 1, 1, 0, 0, 1, 2, 1, 1, 0, 1}; // 0 = linear 1 = logrithmatic 2 = anti-logrithmatic
const unsigned char TAPER_RESOLUTION = 128;
const unsigned char INIT_PARA_VALS = 125;



const unsigned char ROTARY_NO_CHANGE[NUM_OF_ROTARYS] = {0, 0, 0, 0, 0, 0};     // a template to check against the current rotary vals, if they are equal then there was no movement on any of the encoders
const unsigned char ROTARY_STARTUPVALS = 0;
const unsigned char PRESET_SIZE = NUM_OF_PARAMETERS*3+2;
const unsigned int  PRESET_EEPROM_MEMORY = PRESET_SIZE*NUM_OF_PRESETS;  // probably no use but good to have maybe?

///////////////////////////////////////   btnStatesOne Byte
const unsigned char BTN_CLK_1_BIT = 7;
const unsigned char BTN_CLK_2_BIT = 6;
const unsigned char BTN_CLK_3_BIT = 5;
const unsigned char BTN_CLK_4_BIT = 4;
const unsigned char BTN_CLK_5_BIT = 3;
const unsigned char BTN_CLK_6_BIT = 2;
const unsigned char ANTI_KILL_BIT = 1;
const unsigned char KILL_BIT = 0;

///////////////////////////////////////   btnStatesTwo Byte
const unsigned char TUNER_BIT = 7;
const unsigned char PAGE_TWO_BIT = 6;
const unsigned char PAGE_ONE_BIT = 5;
const unsigned char SAVE_PRESET_BIT = 4;
const unsigned char SAVE_DEFAULT_BIT = 3;
const unsigned char XY_HOLD_BIT = 2;
const unsigned char ASSIGN_X_BIT = 1;
const unsigned char ASSIGN_Y_BIT = 0;

///////////////////////////////////////////////////////////////////
///////////////////////////////////////   Global Variables
 
unsigned char defVals[NUM_OF_PARAMETERS];
unsigned char rGBOutputs[NUM_OF_ROTARYS*3];
unsigned char xYInputVals[4];  // holds the current values of the xy touch pad and last
unsigned char btnStatesOne;    // holds a byte of data where each bit represents a button state
unsigned char btnStatesTwo;    // holds a byte of data where each bit represents a button state
unsigned char pageNum;         // holds the current page number
unsigned char xAssignment;
unsigned char yAssignment;
unsigned char presetNum; // holds the number of the current preset
//  unsigned char taperedVals[NUM_OF_PARAMETERS];
unsigned char outputVals[NUM_OF_PARAMETERS]; // the plus one is a page byte which tells the other arduin 
signed char rotaryVals[NUM_OF_ROTARYS]; // current incremental info on the rotary encoders(how much they've all turned since last loop iteration) these will be added or subtracted from selected parameters rawValue
boolean defStates[NUM_OF_PARAMETERS];
int rawVals[NUM_OF_PARAMETERS];     // holds the untapered values of the current parameter[24] values
unsigned char TAPER_ARRAY[2][256];// this 2d array will hold both the log taper and the anti-log taper which will modify the outputVals to account for differant potentiameter types(linear, audio(log), and anti-log) linear pots dont need to be adjusted so no val array is needed
bool tunerState = false;
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
  for (unsigned char j = 0; j<2; j++) { // this just creates values for all indexs of the arrays
    for (unsigned char i=0; i<256; i++){
          unsigned char temp = i;
          TAPER_ARRAY[j][i] = i;
     };
  };
}

void loop() {
  
  parseSerialData();    // takes in serial parses all ten bytes to correct variables
  setPageNum();         // takes current serial information and checks what page the user wants
  rotarysChangeState(); // parses through the rotaryVals array and looks for changes in value(0's equate to no change while negative and positive show how many steps a specific rotary has moved since the last serial iteration
  setXYVals();          // takes values from the xYInputVals array and changes the corresponding index in the rawVals array 
  xyAssignmentCheck();  //  does logic determining if either the assignX or assignY buttons were pressed and calles the proper functions
  setPreset();           // check to see if setPreset button was pressed and if so assigns the current values of the defVals, defStates, rawVals array to correspondoing presetNum eeprom momory space
  setOutputVals();      // transfers rawVals[] to outputVals[]
  handleDefaultStates(); // checks if the user if requesting to change the state of any of the defaultStates
  setDefaultVals();     // checks if the save default button is pressed and if so saves the values of all parameters that are not set to default
  killAntiKillLogic();  // determins if kill or antikill buttons are pressed and sends the correct values to outputVals
  applyTaper();         // takes the outputVals[] and sends it through the TAPER_ARRAY according to each paras taper type(0, 1, 2)
  setTuner();           // determins if the tuner button is engadged and if so sets last iterations values to outputVals
  setRGBVals();         // 
  sendOutputVals();
};

//void handleStateChanges() {
  //setPageNum();
  //xyAssignment();
  //setDefaultVals();
  //setPreset();
  //handleDefaultStates();
//};

void handleParaValueLogic() {
  rotarysChangeState();
  setXYVales();
  setOutputVals();
  setTuner(); // need to make it so if this is true the kill anti kill dont change the outputVals but still maintain the 6 bit logic num
  killAntiKillLogic();
  applyTaper();
 
};
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
  
   void rotarysChangeState() {    
      for(unsigned char i=0; i<NUM_OF_ROTARYS; i++) {
        if(rotaryVals[i] != ROTARY_NO_CHANGE[i]) {
          setRawVals(i);
        }; 
      };
  };
  
  void setRawVals(unsigned char counter) { // used in rotarysChangeState()
    int rawValsHolder = rawVals[pageNum*NUM_OF_ROTARYS+counter]; 
    rawValsHolder += rotaryVals[counter];
    if(rawValsHolder > 255) {    // made rawVals an int so if it goes above 255 or below 0 it doesn't do wierd things it just goes above 255 or below 0 
      rawValsHolder = 255;       // and then if it is above of below u.c. char levels it sets it back to limits(0-255)
    } else if(rawValsHolder < 0) {
      rawValsHolder = 0;
    };    
    rawVals[pageNum*NUM_OF_ROTARYS+counter] = rawValsHolder; // did this cuz not sure if (rawValsHolder = rawVals[pageNum*6+counter]) just copies rawVals[pageNum*6+counter] or allows direct minipulation of rawVals[pageNum*6+counter]
  };  
  
  boolean isXYTouchActive() {  // used in setXYVals()
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
  

  
  unsigned char checkForBtnClick() {  // used in assignX() and assignY()
    for(unsigned char i = 2; i<8; i++) {
      if(bitRead(btnStatesOne, i)) {
        return i-2;
      };
    };
    return 0;
  };
  
  boolean checkForBtnClickBoolean() {     // used in handleDefaultStates, looks in the btnStatesOne byte which shows rotory btn click info and 
    for(unsigned char i = 2; i<8; i++) {  // returns true if any of the rotorys were clicked in the current iteration
      if(bitRead(btnStatesOne, i)) {
        return i;
      };
    };
    return 0;
  };
  
  void assignX() { // used in xyAssignmentCheck(), it checks the btnStatesOne array for a btn click and assigns the xAssignment variable to the parameter number
    unsigned char btnClk = checkForBtnClick(); // that the rotary click represents
    if(btnClk > 0) {
      xAssignment = pageNum*NUM_OF_ROTARYS+btnClk; // produces 0 -24
    };
  };
  
  void assignY() {  // used in xyAssignmentCheck(), it checks the btnStatesOne array for a btn click and assigns the yAssignment variable to the parameter number
    unsigned char btnClk = checkForBtnClick(); // that the found rotary click represents
    if(btnClk > 0) {
      yAssignment = pageNum*NUM_OF_ROTARYS+btnClk;
    }; 
  }
  
  void xyAssignmentCheck() {
    static unsigned char xOrYFirst = 0; // 0 = neither 1 = xFirst 2 = yFirst
    static unsigned char lastXYState = 0;
    static unsigned char currentXYState = 0;  // static so it doesn't instanciate each iteration???...maybe...idk
    static unsigned char btnCLick = 0;
    bitWrite(currentXYState, 0, bitRead(btnStatesTwo, ASSIGN_X_BIT)); // 0 bit = xAssign
    bitWrite(currentXYState, 1, bitRead(btnStatesTwo, ASSIGN_Y_BIT)); // 1 bit = yAssign
    switch (currentXYState) {
      case 0:
        xOrYFirst = 0;
        lastXYState = currentXYState;
        break;
      case 1:
        assignX();                             
        xOrYFirst = 1;
        lastXYState = currentXYState;
        break;
      case 2:
        // ckeck for default btn click
        assignY();                             
        xOrYFirst = 2;
        lastXYState = currentXYState;
        break;
      case 3:
        switch (lastXYState) {
          case 0:
            assignX();                            
            xOrYFirst = 1;
            lastXYState = currentXYState;
            break;
          case 1:
            assignY();                            
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
  
  boolean checkPresetState() { // used in setPreset(), returns true if the preset btn was pressed 
    return bitRead(btnStatesTwo, SAVE_PRESET_BIT);
  };
  
    void setPreset() {
    static boolean presetLoopFilter = false; // keeps program from iteratind the eeprom writes over repeatedly
    if (checkPresetState()) {               // with this it will only write once per button push
      if(!presetLoopFilter){
        for(unsigned char i=0; i<NUM_OF_PARAMETERS; i++) {      // copies 74 bytes of data to eeprom
          EEPROM.write(presetNum*PRESET_SIZE+i, defVals[i]);
          EEPROM.write(NUM_OF_PARAMETERS+(presetNum*PRESET_SIZE+i), defStates[i]);
          EEPROM.write((NUM_OF_PARAMETERS*2)+(presetNum*PRESET_SIZE+i), rawVals[i]);        
        };
        EEPROM.write((NUM_OF_PARAMETERS*3)+(presetNum*PRESET_SIZE), xAssignment);
        EEPROM.write((NUM_OF_PARAMETERS*3)+(presetNum*PRESET_SIZE)+1, yAssignment);
        presetLoopFilter = true;
      };
    } else {
        presetLoopFilter = false; 
    };
  };
  
  boolean checkSaveDefState() { // used in setDefaultVals();
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
  
  boolean checkXYAsignState() { // used in handleDefaultStates();
    boolean xState = bitRead(btnStatesTwo, ASSIGN_X_BIT);
    boolean yState = bitRead(btnStatesTwo, ASSIGN_Y_BIT);
    if(xState)
      return true;
    else if(yState) 
      return true;
    else 
      return false;   
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
      tunerState = true;                                                  
    } else {
       tunerState = false;
    };
  };
  
  boolean checkXYHoldState() { // used in setXYVals(), simply returns wether the xyHold button is pressed or not
    return bitRead(btnStatesTwo, XY_HOLD_BIT);
  };
  
  void setXYHold() { // used in setXYVals(), used if the xyHold button if engadged, it takes the value of the last iteration of xYInputVals and copies it to the current iteration 
      rawVals[xAssignment] = xYInputVals[2]; // this allows the user to push the xyHold button when they find an xy val sound that they like and be able to put their hand of the xy pad 
      rawVals[yAssignment] = xYInputVals[3]; // while the values stay were the user last had them
  };
  
//  boolean checkKillState() { // used in setOutputVals()
//    return bitRead(btnStatesOne, KILL_BIT);
//  };
//  
//  boolean checkAntiKillState() { // used in setOutputVals()
//    return bitRead(btnStatesOne, ANTI_KILL_BIT);
//  };
  
  void setOutputVals() {
    for(unsigned char i = 0; i<NUM_OF_PARAMETERS; i++) {
      outputVals[i] = rawVals[i];
    };        
  };
  void cleanOutput() {
    for(unsigned char i = 0; i<NUM_OF_PARAMETERS; i++) {
      outputVals[i] = rawVals[i];
    };
  };
 
 void kill() {  // used in killANtiKillLogic()
   for(unsigned char i = 0; i<NUM_OF_PARAMETERS; i++) {
     outputVals[i] = defVals[i];
   };
 };
 
 void antiKill() {  // used in killANtiKillLogic()
   for(unsigned char i = 0; i<NUM_OF_PARAMETERS; i++) {
     outputVals[i] = rawVals[i];
   };
 };
 
 void overRideKill() {  // used in killANtiKillLogic()
   for(unsigned char i = 0; i<NUM_OF_PARAMETERS; i++)  {
     if(!defStates[i]) {
       outputVals[i] = rawVals[i];
     } else {
         outputVals[i] = defVals[i];
       };
   };
 };
 
 void setKillFirst(unsigned char killAntiKillNum, unsigned char state) {  // used in killANtiKillLogic()   
   bitWrite(killAntiKillNum, 4, state);  // not sure how to not sent a copy of killAntiKillNum as opposed to a pointer 
 };
 
 void setAntiFirst(unsigned char killAntiKillNum, unsigned char state) {  // used in killANtiKillLogic()
   bitWrite(killAntiKillNum, 5, state);
 }; 
  
  
  void killAntiKillLogic() {
    static unsigned char killAntiKillNum = 0;
    bitWrite(killAntiKillNum, 1, bitRead(killAntiKillNum, 0));
    bitWrite(killAntiKillNum, 3, bitRead(killAntiKillNum, 2));      
    bitWrite(killAntiKillNum, 0, bitRead(btnStatesOne, 0));
    bitWrite(killAntiKillNum, 2, bitRead(btnStatesOne, 1));
    if(!checkTunerState()) {
      switch (killAntiKillNum) {    // switch case is optimized so most common nums are higher in the switch case order, so like the num 19; which can be thought of as killFirst(true), lastKill(high), currentKill(high) is high up the chain
                                   // because when you push killSwitch on it will likely be on for many iterations so it is a high importance number 
        case 0:
          if(!tunerState) {cleanOutput();};
          break;
        case 19:
          if(!tunerState) {kill();};
          break; 
        case 31:
          if(!tunerState) {overRideKill();};
          break;
        case 47:   
          if(!tunerState) {kill();};   
          break;
        case 1:
          if(!tunerState) {kill();};
          setKillFirst(killAntiKillNum, 1);
          break;
        case 4:
          if(!tunerState) {antiKill();};
          setAntiFirst(killAntiKillNum, 1);
          break;
        case 18:
          if(!tunerState) {cleanOutput();};
          setKillFirst(killAntiKillNum, 0);
          break;
        case 44:
          if(!tunerState) {antiKill();};
          
          break;
        case 23:
          if(!tunerState) {overRideKill();};
          break;
        case 27:   
          if(!tunerState) {kill();};
          break;
        case 30:
          if(!tunerState) {antiKill();};
          setKillFirst(killAntiKillNum, 0);
          setAntiFirst(killAntiKillNum, 1);
          break; 
        case 40:
          if(!tunerState) {cleanOutput();};
          setAntiFirst(killAntiKillNum, 0);
          break;
        case 43:
          if(!tunerState) {kill();};
          setAntiFirst(killAntiKillNum, 0);
          setKillFirst(killAntiKillNum, 1);
          break;
        case 45:   
          if(!tunerState) {kill();};
          break; 
        case 46:
          if(!tunerState) {antiKill();};
          break;  
        case 5:
          if(!tunerState) {kill();};
          setKillFirst(killAntiKillNum, 1);
          break;
        case 22:
          if(!tunerState) {antiKill();};
          setKillFirst(killAntiKillNum, 0);
          setAntiFirst(killAntiKillNum, 1);
          break;
        case 26:
          if(!tunerState) {cleanOutput();};
          setKillFirst(killAntiKillNum, 0);
          break;  
        case 42:
          setAntiFirst(killAntiKillNum, 0);
          break;   
        case 41:
          if(!tunerState) {kill();};
          setAntiFirst(killAntiKillNum, 0);
          setKillFirst(killAntiKillNum, 1);
          break;
         break;      // not sure if this does anything
      };
    };    
  };
  void applyTaper() {
    for(unsigned char i = 0; i<NUM_OF_PARAMETERS;i++) {
      switch(PARA_TAPER_TYPES[i]) {
        case 0:
          break;
        case 1:
          outputVals[i] = TAPER_ARRAY[0][outputVals[i]];
          break;
        case 2:
          outputVals[i] = TAPER_ARRAY[1][outputVals[i]];
          break;
      };
    };
  };
    
    void sendOutputVals() {
      for(unsigned char i = 0; i<NUM_OF_PARAMETERS; i++) {
        Serial.write(outputVals[i]);
      };
    };  
   
    
    const unsigned char rGBTemplateArray[768] = { // 0--------64------128---- 192---->255   PWM Vals
                                                // green----cyan----blue----mag---->red
      0, 255, 0,
      0, 255, 4, 0, 255, 8, 0, 255, 12, 0, 255, 16, 0 , 155, 20, 0, 255, 24, 0, 255, 28, 0, 255, 32, 0, 255, 36, 0, 255, 40,
      0, 255, 42, 0, 255, 46, 0, 255, 50, 0, 255, 54, 0, 255, 58, 0, 255, 62, 0, 255, 66, 0 , 255, 70, 0, 255, 74, 0, 255, 78,  
      0, 255, 82, 0, 255, 86, 0, 255, 90, 0, 255, 94, 0, 255, 98, 0, 255, 102, 0, 255, 106, 0 , 255, 110, 0, 255, 114, 0, 255, 118,
      0, 255, 122, 0, 255, 126, 0, 255, 130, 0, 255, 134, 0, 255, 140, 0, 255, 144, 0, 255, 148, 0 , 255, 152, 0, 255, 156, 0, 255, 160,
      0, 255, 164, 0, 255, 168, 0, 255, 172, 0, 255, 176, 0, 255, 180, 0, 255, 184, 0, 255, 188, 0 , 255, 192, 0, 255, 196, 0, 255, 200,
      0, 255, 204, 0, 255, 208, 0, 255, 212, 0, 255, 216, 0, 255, 220, 0, 255, 224, 0, 255, 228, 0 , 255, 232, 0, 255, 236, 0, 255, 240,
      0, 255, 244, 0, 255, 248, 0, 255, 252, 0, 255, 255, 0, 250, 255, 0, 246, 255, 0, 242, 255, 0 , 238, 255, 0, 234, 255, 0, 230, 255,
      0, 226, 255, 0, 222, 255, 0, 218, 255, 0, 214, 255, 0, 210, 255, 0, 208, 255, 0, 204, 255, 0 , 200, 255, 0, 196, 255, 0, 192, 255,      
      0, 188, 255, 0, 184, 255, 0, 180, 255, 0, 176, 255, 0, 172, 255, 0, 168, 255, 0, 164, 255, 0 , 160, 255, 0, 156, 255, 0, 152, 255,
      0, 148, 255, 0, 144, 255, 0, 140, 255, 0, 136, 255, 0, 132, 255, 0, 128, 255, 0, 124, 255, 0 , 120, 255, 0, 116, 255, 0, 112, 255,
      0, 108, 255, 0, 104, 255, 0, 100, 255, 0, 96, 255, 0, 92, 255, 0, 88, 255, 0, 84, 255, 0 , 80, 255, 0, 76, 255, 0, 72, 255,
      0, 68, 255, 0, 64, 255, 0, 60, 255, 0, 56, 255, 0, 52, 255, 0, 48, 255, 0, 44, 255, 0 , 40, 255, 0, 36, 255, 0, 32, 255,
      0, 28, 255, 0, 24, 255, 0, 20, 255, 0, 16, 255, 0, 12, 255, 0, 8, 255, 0, 4, 255, 0 , 0, 255, 0, 0, 255, 0, 0, 255,
      4, 0, 255, 8, 0, 255, 12, 0, 255, 16, 0, 255, 20, 0, 255, 24, 0, 255, 28, 0, 255, 32 , 0, 255, 36, 0, 255, 40, 0, 255,
      44, 0, 255, 48, 0, 255, 52, 0, 255, 56, 0, 255, 60, 0, 255, 64, 0, 255, 68, 0, 255, 72 , 0, 255, 76, 0, 255, 80, 0, 255,
      84, 0, 255, 88, 0, 255, 92, 0, 255, 96, 0, 255, 100, 0, 255, 104, 0, 255, 108, 0, 255, 112 , 0, 255, 116, 0, 255, 130, 0, 255,
      134, 0, 255, 138, 0, 255, 142, 0, 255, 146, 0, 255, 150, 0, 255, 154, 0, 255, 158, 0, 255, 162 , 0, 255, 166, 0, 255, 170, 0, 255,
      174, 0, 255, 178, 0, 255, 182, 0, 255, 186, 0, 255, 190, 0, 255, 194, 0, 255, 198, 0, 255, 202 , 0, 255, 206, 0, 255, 210, 0, 255,
      214, 0, 255, 218, 0, 255, 222, 0, 255, 226, 0, 255, 230, 0, 255, 234, 0, 255, 238, 0, 255, 242 , 0, 255, 246, 0, 255, 250, 0, 255,
      255, 0, 250, 255, 0, 246, 255, 0, 242, 255, 0, 238, 255, 0, 234, 255, 0, 230, 255, 0, 228, 255 , 0, 224, 255, 0, 220, 255, 0, 216,
      255, 0, 212, 255, 0, 208, 255, 0, 204, 255, 0, 200, 255, 0, 196, 255, 0, 192, 255, 0, 188, 255 , 0, 184, 255, 0, 180, 255, 0, 176,
      255, 0, 172, 255, 0, 168, 255, 0, 164, 255, 0, 160, 255, 0, 156, 255, 0, 152, 255, 0, 148, 255 , 0, 144, 255, 0, 140, 255, 0, 136,
      255, 0, 132, 255, 0, 128, 255, 0, 124, 255, 0, 120, 255, 0, 116, 255, 0, 112, 255, 0, 108, 255 , 0, 104, 255, 0, 100, 255, 0, 96,
      255, 0, 92, 255, 0, 88, 255, 0, 84, 255, 0, 80, 255, 0, 76, 255, 0, 72, 255, 0, 68, 255 , 0, 64, 255, 0, 60, 255, 0, 56,
      255, 0, 52, 255, 0, 48, 255, 0, 44, 255, 0, 40, 255, 0, 36, 255, 0, 32, 255, 0, 28, 255 , 0, 24, 255, 0, 20, 255, 0, 16,
      255, 0, 12, 255, 0, 8, 255, 0, 4, 255, 0, 0, 255, 255, 255
    };
    
     void setRGBVals() {
      for(unsigned char i = 0; i<NUM_OF_ROTARYS;i++) {
        for(unsigned char j = 0; j<3; j++) {
          rGBOutputs[(i*3)+j] = rGBTemplateArray[outputVals[pageNum*6+i]+j];
        };
      };
    };
  
