
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson
#include "common/hardwareDefinitions.h"


ioExtender inputControllers[COUNT_IO_EXTENDER];
outputController outputControllers[COUNT_OUTPUT_CONTROLLER];
temperatureSensor temperatureSensors[COUNT_TEMPERATURE_SENSOR];
managerFrontPanelButton frontPanelButton;


#define DEBUG 400


void setup() {

    #ifdef DEBUG
      Serial.begin(115200);
    #endif

    Wire.begin();

    //Configure the peripherals
    frontPanelButton.begin();

    setupInputs();
    setupOutputs();
    setupTemperatures();

    //System has started, show normal state
    frontPanelButton.setLED(managerFrontPanelButton::status::NORMAL);

}


void loop() {

  loopInputs();
  loopTemperatures();

}


/**Instantiate the input objects*/
void setupInputs(){

  //Setup the input controllers
  for(int i = 0; i < COUNT_IO_EXTENDER; i++){

    pinMode(inputControllers[i].interruptPin, INPUT);

    inputControllers[i].interruptPin = PINS_IO_EXTENDER[i];
    inputControllers[i].address = ADDRESSES_IO_EXTENDER[i];
    
    #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9555
      inputControllers[i].hardware.attach(Wire, inputControllers[i].address);
      inputControllers[i].hardware.polarity(PCA95x5::Polarity::INVERTED_ALL);
      inputControllers[i].hardware.direction(PCA95x5::Direction::IN_ALL);

      //Ensure we connected to the controller
      if(inputControllers[i].hardware.i2c_error() != 0){
        handleInputFailure(&inputControllers[i]);
      }

    #endif

    //Get the current input states and ignore the debounce delays
    readInputPins(i, true);
  }

}


/**Instantiates the output objects*/
void setupOutputs(){

  //Setup output controllers
  for(int i = 0; i < COUNT_OUTPUT_CONTROLLER; i++){
    outputControllers[i].address = ADDRESSES_OUTPUT_CONTROLLER[i];

    #if MODEL_OUTPUT_CONTROLLER == ENUM_MODEL_OUTPUT_CONTROLLER_PCA9685
      outputControllers[i].hardware = PCA9685(outputControllers[i].address);
      outputControllers[i].hardware.begin();
      outputControllers[i].hardware.setFrequency(FREQUENCY_PWM);

      //Ensure the controller is online
      if(outputControllers[i].hardware.isConnected() == false){       
        handleOutputFailure(&outputControllers[i]);
      }

    #endif

  }

}


/**Instantiates the temperature sensor objects*/
void setupTemperatures(){

  for(int i = 0; i < COUNT_TEMPERATURE_SENSOR; i++){

      temperatureSensors[i].address = ADDRESSES_TEMPERATURE_SENSORS[i];

      #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
        temperatureSensors[i].hardware = PCT2075(temperatureSensors[i].address);

        if(temperatureSensors[i].hardware.getConfig() !=0){
          handleTemperatureFailure(&temperatureSensors[i]);
        }

      #endif

      #if PRODUCT_ID == 32322211

        switch(i){
          case 0:
            temperatureSensors[i].location = temperatureSensorLocation::CENTER;
            break;
          default:
            temperatureSensors[i].location = temperatureSensorLocation::UNKNOWN;
            break;
        }
       
      #endif

    }
}


/**Checks the temperatures at each sensor*/
void loopTemperatures(){

  //Loop through each temperature sensor on the board
  for(int i = 0; i < COUNT_TEMPERATURE_SENSOR; i++){

    if(temperatureSensors[i].enabled == false){
      continue;
    }

    //If the timer has expired OR if the temperature sensor has never been read, read it
    if((millis() - temperatureSensors[i].timePreviousRead > MILLS_TEMPERATURE_SLEEP_DURATION) || (temperatureSensors[i].timePreviousRead == 0)){

      //Temperatures will be reported in degrees C
      #if MODEL_TEMPERATURE_SENSOR == ENUM_MODEL_TEMPERATURE_SENSOR_PCT2075
        float currentRead = temperatureSensors[i].hardware.getTempC();
      #endif

      //Set the new read time time
      temperatureSensors[i].timePreviousRead = millis();
      
      //Check if the delta between the two reads is more than the DEGREES_TEMPERATURE_VARIATION_ALLOWED
      if(abs(currentRead - temperatureSensors[i].previousRead) > DEGREES_TEMPERATURE_VARIATION_ALLOWED){

        //Store the new temperature reading
        temperatureSensors[i].previousRead = currentRead;

        //Publish an event for the change
        publishNewTemperature(&temperatureSensors[i]);
      }      
    }
  }
}


/**Checks each interrupt pin between the IO Extender and the ESP32*/
void loopInputs(){

  for(int i = 0; i < COUNT_IO_EXTENDER; i++){

    //Ignore disabled input controllers
    if(inputControllers[i].enabled == false){
      continue;
    }
    
    //Need to read each input pin for LOW so we can detect intra-PCA9555 button press changes
    if(digitalRead(inputControllers[i].interruptPin) == LOW){
      readInputPins(i);
    }
  }

}


/** Checks the pins on the ioExtenderIndex IO extender for changes */
void readInputPins(int ioExtenderIndex){
  readInputPins(ioExtenderIndex, false);
}


/** Checks the pins on the ioExtenderIndex IO extender for changes, optionally ignoring the debounce delay */
void readInputPins(int ioExtenderIndex, boolean ignoreDebounceDelay){

  #ifdef DEBUG
    #if DEBUG > 50
      if(ignoreDebounceDelay == true){
        Serial.println("(readInputPins) IO Extender: 0x" + String(inputControllers[ioExtenderIndex].address, HEX) + " Ignoring debounce delay.");
      }
    #endif
  #endif

  uint16_t pinRead = 0;

  #if MODEL_IO_EXTENDER == ENUM_MODEL_IO_EXTENDER_PCA9555

    pinRead = inputControllers[ioExtenderIndex].hardware.read();

  #endif

  //Exit if the value returned from the controller is the same as the value that was previously read
  if(pinRead == inputControllers[ioExtenderIndex].previousRead){
    return;
  }

  //Process each pin on the specified IO extender
  for(int i = 0; i < COUNT_PINS_IO_EXTENDER; i++){

    inputState currentState = bitToInputState(bitRead(pinRead, i));

    //Check if the value returned in the read is the same as the last read
    if(inputControllers[ioExtenderIndex].inputs[i].state == currentState){
      #ifdef DEBUG
        #if DEBUG > 500
          Serial.println("(readInputPins) IO Extender: 0x" + String(inputControllers[ioExtenderIndex].address, HEX) + " Pin: " + String(i) + " input states match. Previous: " + String(inputControllers[ioExtenderIndex].inputs[i].state, HEX) + " Current: " + String(currentState, HEX));
        #endif
      #endif
      continue;
    }

    //Values are different; Check if we are within the debounce delay
    if(millis() - inputControllers[ioExtenderIndex].inputs[i].timePreviousChange < DEBOUNCE_DELAY){

      //Check if the debounce delay should be checked (ignored on startup)
      if(ignoreDebounceDelay == false){
        #ifdef DEBUG
          #if DEBUG > 50
            Serial.println("(readInputPins) IO Extender: 0x" + String(inputControllers[ioExtenderIndex].address, HEX) + " Pin: " + String(i) + " DEBOUNCE_DELAY (" + String(DEBOUNCE_DELAY) + ") not satisfied. Time Previous Change: " + String(inputControllers[ioExtenderIndex].inputs[i].timePreviousChange) + " Current Time: " + String(millis()) + " Difference: " + String(millis() - inputControllers[ioExtenderIndex].inputs[i].timePreviousChange));
          #endif
        #endif

        continue;
      }

    }

    switch(inputControllers[ioExtenderIndex].inputs[i].type){

      case inputType::NORMALLY_OPEN:

        //Check if input is in an abnormal state
        if(currentState == inputState::STATE_CLOSED){

          inputControllers[ioExtenderIndex].inputs[i].timePreviousChange = millis();

          #ifdef DEBUG
            #if DEBUG > 50
              Serial.println("(readInputPins) IO Extender: 0x" + String(inputControllers[ioExtenderIndex].address, HEX) + " Pin: " + String(i) + " Type: " + String(inputControllers[ioExtenderIndex].inputs[i].type) + " New State: " + String(currentState) + " (Abnormal)");
            #endif
          #endif          
       
          //do something
          break;
        }

        //Check if input is in normal state
        if(currentState == inputState::STATE_OPEN){
          
          inputControllers[ioExtenderIndex].inputs[i].timePreviousChange = 0;

          #ifdef DEBUG
            #if DEBUG > 50
              Serial.println("(readInputPins) IO Extender: 0x" + String(inputControllers[ioExtenderIndex].address, HEX) + " Pin: " + String(i) + " Type: " + String(inputControllers[ioExtenderIndex].inputs[i].type) + " New State: " + String(currentState) + " (Normal)");
            #endif
          #endif
         
          //do something
          break;
        }

      case inputType::NORMALLY_CLOSED:

        //Check if input is in an abnormal state
        if(currentState == inputState::STATE_OPEN){

          inputControllers[ioExtenderIndex].inputs[i].timePreviousChange = millis();
          
          #ifdef DEBUG
            #if DEBUG > 50
              Serial.println("(readInputPins) IO Extender: " + String(inputControllers[ioExtenderIndex].address, HEX) + " Pin: " + String(i) + " Type: " + String(inputControllers[ioExtenderIndex].inputs[i].type) + " New State: " + String(currentState) + " (Abnormal)");
            #endif
          #endif
          
          //do something
          break;
        }

        //Check if input is in normal state
        if(currentState == inputState::STATE_CLOSED){
          
          inputControllers[ioExtenderIndex].inputs[i].timePreviousChange = 0;

          #ifdef DEBUG
            #if DEBUG > 50
              Serial.println("(readInputPins) IO Extender: " + String(inputControllers[ioExtenderIndex].address, HEX) + " Pin: " + String(i) + " Type: " + String(inputControllers[ioExtenderIndex].inputs[i].type) + " New State: " + String(currentState) + " (Normal)");
            #endif
          #endif

          //do something
          break;
        }

    }

    //Set the value of the pin to the current state
    inputControllers[ioExtenderIndex].inputs[i].state = currentState;

  }


  //Set the controller's value to the updated value
  inputControllers[ioExtenderIndex].previousRead = pinRead;
}


/** Convert a bit to inputState */
inputState bitToInputState(boolean value){

  if(value == LOW){
    return inputState::STATE_OPEN;
  }else{
    return inputState::STATE_CLOSED;
  }

}


/** Convert a temperatureSensorLocation to string */
String temperatureSensorLocationToString(temperatureSensorLocation value){

  switch(value){

    case temperatureSensorLocation::CENTER:
      return "CENTER";
      break;

    default:
      return "UNKNOWN";
      break;
  }
}


/** Publish a notification to MQTT about the temperature sensor change */
void publishNewTemperature(temperatureSensor *sensor){

  #ifdef DEBUG
    Serial.println("New Temperature: " + String(sensor->previousRead) + " at " + temperatureSensorLocationToString(sensor->location));
  #endif

  //TODO: Add MQTT Event
}

/** Handles failures of input controllers */
void handleInputFailure(ioExtender *controller){

  #ifdef DEBUG
    Serial.println("Input controller at 0x" + String(controller->address, HEX) + " is offline.");
  #endif

  //Disable the controller
  controller->enabled = false;

  //Set the LED to trouble
  frontPanelButton.setLED(managerFrontPanelButton::status::FAILURE);


  //TODO: Add MQTT and stuff

}

/** Handles failures of output controllers */
void handleOutputFailure(outputController *controller){

  #ifdef DEBUG
    Serial.println("Output controller at 0x" + String(controller->address, HEX) + " is offline.");
  #endif

  //Disable the controller
  controller->enabled = false;

  //Set the LED to trouble
  frontPanelButton.setLED(managerFrontPanelButton::status::FAILURE);


  //TODO: Add MQTT and stuff

}

/** Handles failures of temperature sensors */
void handleTemperatureFailure(temperatureSensor *sensor){

  #ifdef DEBUG
    Serial.println("Temperature sensor at 0x" + String(sensor->address, HEX) + " is offline.");
  #endif

  //Disable the sensor
  sensor->enabled = false;

  //Set the LED to trouble
  frontPanelButton.setLED(oledLEDButton::FAILURE);


  //TODO: Add MQTT Event

}