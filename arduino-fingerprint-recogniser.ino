// INCLUDES
// Install through Arduino IDE
// Sketch -> Include Library -> Manage Libraries -> Adafruit Fingerprint Sensor Library v1.1.3
#include <Adafruit_Fingerprint.h>

// GLOBALS
// Emulate a software serial interface to the fingerprint sensor on specified Rx/Tx pins
// REMEMBER - Rx of Arduino connects to Tx of sensor, and Tx or Arduino connects to Rx of sensor!
SoftwareSerial softSerial(2, 3);
// Create an instance of the fingerprint reader using the software serial connection
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&softSerial);

void setup() {
  // Start the serial connection to enable debug output to be displayed on the Serial Monitor
  Serial.begin(9600);

  Serial.print(F("Connecting to fingerprint sensor..."));
  // Begin serial communication with the fingerprint sensor
  finger.begin(57600);
  // Wait for stabilisation
  delay(100);
  if (finger.verifyPassword()) {
    Serial.println("OK!");
  } else {
    Serial.println("FAILED");
  }

  // Retrieve the count of registered fingerprints
  finger.getTemplateCount();
  Serial.print(F("Sensor database contains ")); 
  Serial.print(finger.templateCount); 
  Serial.println(F(" fingerprint templates"));
  
  Serial.println(F("Ready..."));
}

void loop() {
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
