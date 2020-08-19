/**********************************************************************************************************************************************************************
 * Disclaimer:
 * This is a modified version of Mark Alexander Barros's Arduino Portal Turret - (v 1.0) source file, mainly used to write my own custom application 
 * for serial communications practice, with C++, as kind of a boot camp project. With the modifications I made, sound is played back through the computer's speakers, 
 * the Adafruit Wave Shield module is no longer needed, and running a Windows application while having the Arduino board plugged in is sufficient. I used my own mix 
 * of sound files as well, and planning to implement features like an additional servo, motion tracking, more user interactions (ability to pick up the turret, 
 * tilt it to the side so it shuts down) and more. Current version of this adaptation is v 0.5. 
 * 
 * Issues: 
 * I use a SG90 micro servo, with the Servo.h library, and servo jittering is an issue for me. There are techniques of course for reducing the jitter, like getting rid
 * of Arduino runtime and writing your own for better PWM signal resolution, limiting the servo's min and max range by a few degrees, detaching the servo after each 
 * increment of movement and so on. Improvments in this aspect may come with a future update.  
 *  
 * License:
 * This work is licenced under Crative Commons Attribution-ShareAlike 3.0 
 * http://creativecommons.org/licenses/by-sa/3.0/
 * THE LICENSOR OFFERS THE WORK AS-IS AND MAKES NO REPRESENTATIONS OR 
 * WARRANTIES OF ANY KIND CONCERNING THE WORK, EXPRESS, IMPLIED, STATUTORY
 * OR OTHERWISE, INCLUDING, WITHOUT LIMITATION, WARRANTIES OF TITLE, 
 * MERCHANTIBILITY, FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT, OR
 * THE ABSENCE OF LATENT OR OTHER DEFECTS, ACCURACY, OR THE PRESENCE OF 
 * ABSENCE OF ERRORS, WHETHER OR NOT DISCOVERABLE.
 * 
 * In terms of parts, I used a 9V adapter, and a buck converter for powering the servo. Instead of a ping sensor, I used two HC-SR04 ultrasonic distance sensors. And, 
 * of course I used electric tape to hold the whole machination together. Nah, only to hold the PIR sensor to the base frame (a slab of wood). The turret frame is just
 * glued to the wood. 
 * 
 * You can find out more information down below.
 * 
 * URL to Mark's project:
 * https://www.themadhermit.net/geek-out-build-your-own-arduino-portal-turret/
 * 
 * My GitHub page:
 * https://github.com/CaTwoPlus
 * 
 * Required external library:
 *  NewPing.h - https://playground.arduino.cc/Code/NewPing/
 * 
 * Other files:
 *  Portal Turret Sound Files were downloaded from: http://theportalwiki.com/wiki/Category:Turret_voice_files
 *********************************************************************************************************************************************************************/
#include <Servo.h>
#include <NewPing.h> 

#define SONAR_NUM                   2  // Number of sensors
#define MAX_DISTANCE                40 // Maximum distance (in cm) to ping

#define INFRARED_MOTION_SENSOR      2 
#define RED_TARGETING_LED           3 

// TIMING (DELAYS AND PAUSES)
#define CALL_OUT_TO_TARGET_DELAY  6000 // Time (in ms) to wait between commands spoken in "SEARCHING_STATE"
#define NO_MOTION_DETECTED        3 * CALL_OUT_TO_TARGET_DELAY
#define PIR_SETUP_TIME            30 // Time in seconds needed for PIR Sensor to stabalize on power-up

// TURRET STATES
#define SLEEPING_STATE            1
#define SEARCHING_STATE           2
#define TARGET_AQUIRED_STATE      3  
#define FIRING_STATE              4
#define TARGET_LOST_STATE         5
#define SLEEP_MODE_ACTIVATED      6

//SERVO
Servo Y_SERVO;                    // Declare Servo object. I used PWM PIN 5.
#define SERVO_Y_MIN_POSTION       10
#define SERVO_Y_MIDDLE_POSTION    70
#define SERVO_Y_MAX_POSTION       170
#define SERVO_Y_DELTA             25 // The amount we want to increment or decrement the servo
// Was used for chaging PIN mode at certain locations in order to reduce servo jitter. I went with Servo.detach() instead.   
#define Y_SERVO_PWM_PIN           5 

/**********************************************************************
 * VARIABLES
 **********************************************************************/
// FLAGS
boolean motionDetected         = false; // Holds the high/low value of the PIR sensor used to detect IR movement
boolean targetSeenByHRCSensor  = false; // Holds the high/low value of the HRC sensor used to detect whether object is in front of turret
boolean newData                = false;

// OTHER VARIABLES
int currentState               = SLEEPING_STATE; // Keeps track of the current state the portal turret is in.
String incomingData            = {};
int i                          = 0; 
char receivedChar;             // For serial communication through COM port between Arduino and VS  

// TIME TRACKERS
unsigned long currentMillis    = 0;
unsigned long previousMillis   = currentMillis; // You may want to change millis() to micros() instead further down the line, for generating cleaner PWM pulse, and 
                                                // reducing servo buzzing 
unsigned long startOfSearch    = 0; // Used to keep track of when we started our current search                              
                                                                                                                                                           
// SERVO
int yServoPosition             = SERVO_Y_MIDDLE_POSTION;
boolean sweepingRight          = true;

// HC_SR04
long durationL;
long durationR;
int distanceL;
int distanceR;

// Create NewPing object
// NewPing sonar(trigger_pin, echo_pin [, max_cm_distance])
NewPing sonar[SONAR_NUM] = {NewPing(13, 11, MAX_DISTANCE), NewPing(12, 10, MAX_DISTANCE)};

// For using HC-SR04 sensors
boolean searchForObjectInRange()
{
  boolean result{};
  
  for (uint8_t i = 0; i < SONAR_NUM; i++) 
  { // Loop through each sensor and display results.
    delay(25); // Wait x ms between pings (50ms gives about 20 pings/sec, 25ms seems ideal).  
    Serial.print(i); //29ms should be the shortest delay between pings.
    Serial.print("=");
    Serial.print(sonar[i].ping_cm());
    Serial.print("cm ");
    Serial.println(" ");

    if (((sonar[0].ping_cm()) || (sonar[1].ping_cm())) > 0) 
      result = true;
    else
      result = false;
  }
  //Serial.println (result);
  return (result);
}

 /**************************************************************************
 * SERVO FUNCTIONS
 **************************************************************************/ 

void moveTurret()
{ 
  unsigned long tStart, tEnd;
  
  if (sweepingRight)
  {
    if (yServoPosition < SERVO_Y_MAX_POSTION)
    {
      yServoPosition += SERVO_Y_DELTA; 
    }
    else
    {
      sweepingRight = false;
      yServoPosition -= SERVO_Y_DELTA;
    }
  }
  else // Sweep Left
  {
    if (yServoPosition > SERVO_Y_MIN_POSTION)
    { 
      yServoPosition -= SERVO_Y_DELTA;
    }
    else
    {
      sweepingRight = true;
      yServoPosition += SERVO_Y_DELTA;
    } 
  }
  
  if (Serial.available() > 0)
  {
    incomingData = Serial.read();
    if ((Serial.find("Play")) && ((yServoPosition == SERVO_Y_MAX_POSTION) || (yServoPosition == SERVO_Y_MIN_POSTION)))
    {
      Serial.println("Play");
      Serial.println("Ping");
    }
    if (Serial.find("Stop"))
    {
      Serial.println("Stop");
    }
  }
   
  Serial.print("Moving Y Servo To Postion "); 
  //delay(65);
  Serial.print(yServoPosition);
  Serial.println();

  // Move the y servo
  if (Y_SERVO.attached() == false)
    Y_SERVO.attach(5);
  Y_SERVO.write(yServoPosition);
}

void moveTurretHome()
{
  if (Y_SERVO.attached() == false)
    Y_SERVO.attach(5);
  
  Serial.print("Moving Servo Home: ");
  Serial.print(SERVO_Y_MIDDLE_POSTION);
  Serial.print(", ");
  Serial.println();
  
  // Move the servo home
  Y_SERVO.write(SERVO_Y_MIDDLE_POSTION);
  delay(300);

  Y_SERVO.detach();
}

void setup() {
  
  // Set up serial port
  Serial.begin(9600); 
  Serial.println("Portal Turret Initializing");

   // Set the pins to input or output as needed
  pinMode(INFRARED_MOTION_SENSOR, INPUT);
  pinMode(RED_TARGETING_LED, OUTPUT);
  
  // Make sure the turret is facing forward on power-up
  pinMode(Y_SERVO_PWM_PIN, OUTPUT);
  moveTurretHome();
  pinMode(Y_SERVO_PWM_PIN, INPUT);
  
  // PIR Motion sensor needs to sample abient conditions for at least 30 seconds  
  // to establish a baseline before it's ready. This loop will flash an LED to 
  // provide a visible indication to the user. 
  for (int i = 0; i < (PIR_SETUP_TIME * 2); i++)
  { 
    digitalWrite(RED_TARGETING_LED, HIGH);
    delay(250); 
    digitalWrite(RED_TARGETING_LED, LOW);
    delay(250); 
  }
  
  // Back and forth communication between Arduino and the app with "Play" and "Stop"
  // helps to reduce chance of sound files looping on the most recently printed chars. 
  // I could not find a reliable way to erase serial output from Arduino.
  if (Serial.available() > 0)
  {
    incomingData = Serial.read();
    if (Serial.find("Play"))
    {
      Serial.println("Play");
      Serial.println("turretActivation");
    }
    if (Serial.find("Stop"))
    {
      Serial.println("Stop");
    }
  }
}

void loop() {

  // Check the status of the PIR sensor
  motionDetected = digitalRead(INFRARED_MOTION_SENSOR);

  // Check HC-SR04 sensors to see if an object is in range wile the turret is awake
  if (currentState != SLEEPING_STATE)
  {
      delay(40); // Make sure servo has stopped before checking otherwise we will get a false detection
      targetSeenByHRCSensor = searchForObjectInRange();
  }
  
  // Enter the machine state where the turret performs specific actions depending on the state it is in
  switch (currentState)
  {
    case SLEEPING_STATE:

      // Just stay sleeping unless motion is detected
      //Serial.println("SLEEPING_STATE");

      if (Serial.available() > 0)
      {
        incomingData = Serial.read();
        if (Serial.find("Play"))
        {
          Serial.println("Play"); 
          Serial.println("SLEEPING_STATE");
        }
        if (Serial.find("Stop"))
        {
          Serial.println("Stop");
        }        
      }
    
      if(motionDetected)
      {
        Serial.println("SLEEPING_STATE - Motion Detected");
        
        // Turn on LED  
        digitalWrite(RED_TARGETING_LED , HIGH);
       
        if (Serial.available() > 0)
        {
          incomingData = Serial.read();
          if (Serial.find("Play"))
          {
            Serial.println("Play"); 
            Serial.println("turretSearchingGuns");
            // Let's get the current time so we can periodically call out to the user as long as his/her presence is "sensed"
            previousMillis = millis();
            // Go to next state
            currentState = SEARCHING_STATE;
          }  
          if (Serial.find("Stop"))
          {
            Serial.println("Stop");
          }
        }
      }    
      break;

    case SEARCHING_STATE:
      
      // Let's get the current time in milliseconds so we know when to abandon our search.
      // NOTE: Need to make sure that we set this variable (startOfSearch to zero prior to 
      // leaving this state so the next time we enter it we can get the time again).
      if (startOfSearch == 0)
        startOfSearch = millis();
        
      Serial.println("SEARCHING_STATE");
       
      if (targetSeenByHRCSensor)
      {
        Serial.println("SEARCHING_STATE - Target Seen");
        currentState = TARGET_AQUIRED_STATE;
        startOfSearch = 0;
      }
      else // Continue to search for a little while
      {
        Serial.println("SEARCHING_STATE - Searching");
        
        // Figure out how much time has passed
        currentMillis = millis();

        // moveTurret(servo, servoPosition, sweepingRight);
        moveTurret();
        
        if (currentMillis > previousMillis + CALL_OUT_TO_TARGET_DELAY)
        {
          // Decide what we are going to say 
          if (!motionDetected && currentMillis > startOfSearch + NO_MOTION_DETECTED)
          {
            Serial.println("SEARCHING_STATE - Switch to Sleep Mode");
            currentState = SLEEP_MODE_ACTIVATED;
            startOfSearch = 0;  // Since we are changing state we need to reset this
          } 
          else
          {
            Serial.println("SEARCHING_STATE - Call out to intruder");
            if (Serial.available() > 0)
            {
              incomingData = Serial.read();
              if (Serial.find("Play"))
              {
                Serial.println("Play"); 
                Serial.println("turretSearching");
              }
              if (Serial.find("Stop"))
              {
                Serial.println("Stop");
              }        
            }
            previousMillis = millis();  
          }  
        }
      }
        
      break;

    case TARGET_AQUIRED_STATE:
      
      Serial.println("TARGET_AQUIRED_STATE - Acknowledge Object Before Firing");

      if (Serial.available() > 0)
      {
        incomingData = Serial.read();
        if (Serial.find("Play"))
        {
          Serial.println("Play"); 
          Serial.println("turretDetection");
          currentState = FIRING_STATE;
        }
        if (Serial.find("Stop"))
        {
          Serial.println("Stop");
        }
      }
     
      break;
    
    case FIRING_STATE: 

    pinMode(Y_SERVO_PWM_PIN, INPUT);
    
      if (targetSeenByHRCSensor) 
      {  
        Serial.println("FIRING_STATE");

        if (Serial.available() > 0)
        {
          incomingData = Serial.read();
          if (Serial.find("Play"))
          {
            Serial.println("Play"); 
            Serial.println("turretAttack");
          }
          if (Serial.find("Stop"))
          {
            Serial.println("Stop");
          }
        }
      }
 
      else
        currentState = TARGET_LOST_STATE;

    case TARGET_LOST_STATE:
    
     Serial.println("TARGET_LOST_STATE");

      if (Serial.available() > 0)
      {
        incomingData = Serial.read();
        if (Serial.find("Play"))
        {
          Serial.println("Play"); 
          Serial.println("turretSearching");
          currentState = SEARCHING_STATE;
        }
        if (Serial.find("Stop"))
        {
          Serial.println("Stop");
        }
      }
        
    break;

    case SLEEP_MODE_ACTIVATED:
    
      Serial.println("SLEEPING_MODE_ACTIVATED");
  
      pinMode(Y_SERVO_PWM_PIN, OUTPUT);
      moveTurretHome();
      pinMode(Y_SERVO_PWM_PIN, INPUT);
      
      // Turn off wake LED
      digitalWrite(RED_TARGETING_LED , LOW);
  
      if (Serial.available() > 0)
      {
        incomingData = Serial.read();
        if (Serial.find("Play"))
        {
          Serial.println("Play"); 
          Serial.println("turretPowerDown");
        }
        if (Serial.find("Stop"))
        {
          Serial.println("Stop");
        }
      }
      
      currentState = SLEEPING_STATE;
    break;
    
    default:
    Serial.println("DEFAULT");
    
  }// End Switch/Case 
}// End loop()
