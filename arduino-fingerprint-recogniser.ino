// INCLUDES
// Install through Arduino IDE
// Sketch -> Include Library -> Manage Libraries -> Adafruit Fingerprint Sensor Library (tested with v2.1.0)
#include <Adafruit_Fingerprint.h>
#include <Bounce2.h>
#include "src/flasher/flasher.h"

// CONSTANTS
byte ledPins[2] = {12, 13};
byte buttonPin = A0;

// GLOBALS
Flasher flasher[2];
Bounce buttonSwitch = Bounce();
// Emulate a software serial interface to the fingerprint sensor on specified Rx/Tx pins
// REMEMBER - Rx of Arduino connects to Tx of sensor, and Tx or Arduino connects to Rx of sensor!
SoftwareSerial softSerial(2, 3);
// Create an instance of the fingerprint reader using the software serial connection
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&softSerial);
enum State {Inactive, Learning, Ready, Waiting, Authorised};
State state = State::Inactive;

void setup() {
  // Start the serial connection to enable debug output to be displayed on the Serial Monitor
  Serial.begin(115200);
  Serial.println(__FILE__ __DATE__);
  
  for(int i=0; i<2; i++) {
    pinMode(ledPins[i], OUTPUT);
    flasher[i].begin(ledPins[i]);
    //flasher[i].flash(5);
  }
  pinMode(buttonPin, INPUT_PULLUP);
  buttonSwitch.attach(buttonPin, INPUT_PULLUP);


  Serial.print(F("Connecting to fingerprint sensor..."));
  // Begin serial communication with the fingerprint sensor
  finger.begin(57600);
  // Wait for stabilisation
  delay(100);
  if (finger.verifyPassword()) {
    Serial.println(F("OK!"));
  }
  else {
    Serial.println(F("FAILED"));
    return;
  }

  
  // TO DO - this doesn#t seem to work! Stuck to 57600 baud
  // Set baud rate to slowest since we'll be using software emulated serial connection.
  // According to the documentation, corresponding baud rate is 9600*N bps
  // https://cdn-shop.adafruit.com/datasheets/ZFM+user+manualV15.pdf
  int result = finger.setBaudRate(FINGERPRINT_BAUDRATE_9600);
  //Serial.println(result);
  // Set minimum security level => i.e. lowest rejection rate (though highest false accept)
  finger.setSecurityLevel(FINGERPRINT_SECURITY_LEVEL_1);

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  // Retrieve the count of registered fingerprints
  finger.getTemplateCount();
  Serial.print(F("Sensor database contains ")); 
  Serial.print(finger.templateCount); 
  Serial.println(F(" fingerprint templates"));

  // Enroll fingerprint
  // while (!enroll());
  
  Serial.println(F("Ready!"));
}

void SetState(State newState){
  switch(newState){
    case State::Learning:
      flasher[0].flash();
      flasher[1].flash();
      state = State::Learning;
      break;
    case State::Ready:
      for(int i=0; i<2; i++){
        flasher[i].stop();
        digitalWrite(ledPins[i], HIGH);
      }
      state = State::Ready;
      break;
    case State::Waiting:
      flasher[1].flash();
      state = State::Waiting;
      break;      
    case State::Authorised:
      for(int i=0; i<2; i++){
        flasher[i].stop();
        digitalWrite(ledPins[i], HIGH);
      }    
      state = State::Authorised;
      break;
    default:
      break;
  }
}

void scan() {
    // Step One - if finger is detected, attempt to take a photo
  int status = finger.getImage();
  if(status == FINGERPRINT_OK) { 
    Serial.print(F("Finger detected... "));
  }
  else { return; }
  
  // Step Two - attempt to extract features from the image
  status = finger.image2Tz();
  if(status == FINGERPRINT_OK) { 
    Serial.print(F("Features extracted... "));
  }
  else {
    Serial.println(F("Could not extract features."));
    return; 
  }
  
  // Step Three - search for matching features in the database of registered fingerprints
  status = finger.fingerFastSearch();
  if(status == FINGERPRINT_OK) { 
    Serial.print(F("Match found! ID:"));
    Serial.print(finger.fingerID);
    Serial.print(F(", confidence level:")); 
    Serial.println(finger.confidence); 
  }
  else {
    Serial.println(F("No match found."));
    return;
  }

  // DO ANY LOGIC USING finger.fingerID HERE!
  //delay(50);

}

void loop() {

  for(int i=0; i<2; i++) {  
    flasher[i].update();
  }
  buttonSwitch.update();

    switch(state){
      case State::Inactive:
        if(buttonSwitch.fell()){  
          // Advance to next state
          SetState(State::Learning);
        }
        break;
      case State::Learning:
        enroll();
        if(buttonSwitch.fell()){  
          // Advance to next state
          SetState(State::Ready);
        }
        break;
      case State::Ready:
        scan();
        if(buttonSwitch.fell()){  
          // Advance to next state
          SetState(State::Waiting);
        }
        break;
      case State::Waiting:
        scan();
        if(buttonSwitch.fell()){  
          // Advance to next state
          SetState(State::Authorised);
        }
        break;      
      case State::Authorised:
        if(buttonSwitch.fell()){  
          // Advance to next state
          SetState(State::Learning);
        }
        break;
      default:
        break;
    }
  }



void clearDatabase() {
  finger.emptyDatabase();
}


uint8_t enroll() {
  // The id of the finger to enroll (1-127)
  static uint8_t id = 1;
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); 
  Serial.println(id);
  while (p != FINGERPRINT_OK) {

    // Abort sequence
    buttonSwitch.update();
    if(buttonSwitch.fell()){  
      // Advance to next state
      SetState(State::Ready);
      return;
    }
    
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        // Serial.print(".");
        // delay(100);
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!
  // Convert image in slot 1 to feature template
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    // Abort sequence
    buttonSwitch.update();
    if(buttonSwitch.fell()){
      // Advance to next state
      SetState(State::Ready);
      return;
    }
    
    p = finger.getImage();
  }
  // Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {


    // Abort sequence
    buttonSwitch.update();
    if(buttonSwitch.fell()){
      // Advance to next state
      SetState(State::Ready);
      return;
    }

    
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      // Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!
  // Convert image in slot 2 to feature template
  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  // Create model from the two scanned feature templates
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // Store the model to enable it to be used for future matching
  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    id++;
    return true;
  } 
  else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } 
  else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } 
  else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } 
  else {
    Serial.println("Unknown error");
    return p;
  }
}
