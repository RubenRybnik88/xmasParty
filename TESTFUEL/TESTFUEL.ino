
  /*/TEST CODE/*/
  /*/WORKS ON TINKERCAD/*/
  /*https://www.tinkercad.com/things/6FD2fLezVeb-terrific-bruticus/editel?sharecode=VXmbfdcxpYd90npp6UMVxAZ0ydWylsiB-fND4g302Gs*/


#include <Adafruit_NeoPixel.h>
#define HATLED 3 /* Arduino pin for the hat lightbulb PWM dimmer*/
#define NUM_LEDS 256 /*Panel Array size*/
#define DATA_PIN 6 /*Arduino pin for LED Array*/
#define ledBrightness 50 /*Global brightness, do not use over 100*/

  /*/Setting up 8-segment LED Matrix/*/
  /*/////////////////////////////////*/

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);


  /*/Main code variables/*/
  /*/////////////////////*/

int BUTTON = 5; /* Arduino pin for the push button fuelguage filler*/
int buttonTime = 50; /* Time that will be considered as the long-press time for pushbutton*/
int previousState = LOW; /* Setting the initial state of push button LOW*/
int presentState;  /* Variable that will store the present state of the button*/
unsigned long press_Time = 0; /* Time at which the button was pressed */
unsigned long release_Time = 0; /* Time at which the button was released */
int fuelLevel = 5; /* Level in fuel tank [IMPORTANT] */
int fillRate = 10;  /* How fast the tank fills up per segment */
int drainRate = 2000;  /* How fast the tank drains (controlled by Potentiometer) */
int potValue; /* Potentiometer variable for drainRate potentiometer*/
int hatBright = 0; /* store PWM brightness for HAT*/
int long flicker; /*random number container*/
int ledState = LOW;  /* ledState used to set the HATLED in Flicker*/
unsigned long previousMillis = 0;  /*will store last time HATLED was updated*/
unsigned long previousLowFuelMillis = 0;  /*will store last time Low Fuel LED was updated*/
int lowFuelRate = 500; /*Low Fuel flash rate*/
int fuelWarning = 1; /*Stores LowFuel indicator On or Off?*/
 

void setup() {
  
  pinMode(BUTTON, INPUT);
  pinMode(HATLED, OUTPUT);
  Serial.begin(9600);
  pixels.begin();
  
}

void loop() {

  /*/Controlling drainRate with a Potentiometer/*/
  /*////////////////////////////////////////////*/

  potValue = (1023- analogRead(A0));                        /*Potentiometer wired to pin A0 to enable analogRead*/ 
  drainRate = map(potValue, 0, 1023, 1, 500);           /*Min/Max value for how fast beer empties*/ 
  Serial.println((String)"Fuel: "+fuelLevel);                               /*Debugging*/ 
  Serial.println((String)"DrainRate: "+drainRate);                               /*Debugging*/ 
  Serial.println((String)"Button: "+presentState); 

  /*/Filling and Emptying the Tank based on push-button/*/
  /*////////////////////////////////////////////////*/
  
  presentState = digitalRead(BUTTON);                      /* Getting the present state of the push button */

  if (previousState == LOW && presentState == HIGH) {      /* If button is pressed, timestamp the initial press-time and flag button as closed*/
    press_Time = millis();
    previousState = HIGH;
  }
  
  else if (previousState == HIGH && presentState == HIGH && fuelLevel < 9) {    /* If button is closed for >buttonTime, start filling fuelLevel at fillRate up to max 9*/
    if ((millis() - press_Time) > buttonTime) {
      fuelLevel = fuelLevel += 1;
      delay(fillRate);
    }
  }

  else if (previousState == HIGH && presentState == LOW) {  /* When button is released, record the release-time and flag button to open*/
    release_Time = millis();
    previousState = LOW;
  }
    
  else if (previousState == LOW && presentState == LOW && fuelLevel > 0) {       /* Once button is open, empty fuelLevel at drainRate down to 0*/
    if ((millis() - release_Time) > drainRate) {
    fuelLevel = fuelLevel -= 1;
    release_Time = millis();
    }
  }

  /*/Controlling LED panel based on fuelLevel/*/
  /*//////////////////////////////////////////*/

      if (fuelLevel == 0) {      /* Empty Tank!!! Flashing and Hat LED flicker*/
      hatFlicker();
      lowFuelFlash();  
      }

    else if (fuelLevel == 1) {      /*Segment 1 = RED*/
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.setPixelColor(1, pixels.Color(0, 0, 0));
    pixels.setPixelColor(2, pixels.Color(0, 0, 0));
    pixels.setPixelColor(3, pixels.Color(0, 0, 0));
    pixels.setPixelColor(4, pixels.Color(0, 0, 0));
    pixels.setPixelColor(5, pixels.Color(0, 0, 0));
    pixels.setPixelColor(6, pixels.Color(0, 0, 0));
    pixels.setPixelColor(7, pixels.Color(0, 0, 0));
    pixels.show();
      }          

    else if (fuelLevel == 2) {    
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.setPixelColor(1, pixels.Color(255, 128, 0));
    pixels.setPixelColor(2, pixels.Color(0, 0, 0));
    pixels.setPixelColor(3, pixels.Color(0, 0, 0));
    pixels.setPixelColor(4, pixels.Color(0, 0, 0));
    pixels.setPixelColor(5, pixels.Color(0, 0, 0));
    pixels.setPixelColor(6, pixels.Color(0, 0, 0));
    pixels.setPixelColor(7, pixels.Color(0, 0, 0));
    pixels.show();
      }          

    else if (fuelLevel == 3) {      
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.setPixelColor(1, pixels.Color(255, 128, 0));
    pixels.setPixelColor(2, pixels.Color(255, 255, 0));
    pixels.setPixelColor(3, pixels.Color(0, 0, 0));
    pixels.setPixelColor(4, pixels.Color(0, 0, 0));
    pixels.setPixelColor(5, pixels.Color(0, 0, 0));
    pixels.setPixelColor(6, pixels.Color(0, 0, 0));
    pixels.setPixelColor(7, pixels.Color(0, 0, 0));
    pixels.show();
      }          

    else if (fuelLevel == 4) {    
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.setPixelColor(1, pixels.Color(255, 128, 0));
    pixels.setPixelColor(2, pixels.Color(255, 255, 0));
    pixels.setPixelColor(3, pixels.Color(255, 255, 0));
    pixels.setPixelColor(4, pixels.Color(0, 0, 0));
    pixels.setPixelColor(5, pixels.Color(0, 0, 0));
    pixels.setPixelColor(6, pixels.Color(0, 0, 0));
    pixels.setPixelColor(7, pixels.Color(0, 0, 0));
      }          

    else if (fuelLevel == 5) {     
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.setPixelColor(1, pixels.Color(255, 128, 0));
    pixels.setPixelColor(2, pixels.Color(255, 255, 0));
    pixels.setPixelColor(3, pixels.Color(255, 255, 0));
    pixels.setPixelColor(4, pixels.Color(0, 255, 0));
    pixels.setPixelColor(5, pixels.Color(0, 0, 0));
    pixels.setPixelColor(6, pixels.Color(0, 0, 0));
    pixels.setPixelColor(7, pixels.Color(0, 0, 0));
      }          


      
    else if (fuelLevel == 6) {     
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.setPixelColor(1, pixels.Color(255, 128, 0));
    pixels.setPixelColor(2, pixels.Color(255, 255, 0));
    pixels.setPixelColor(3, pixels.Color(255, 255, 0));
    pixels.setPixelColor(4, pixels.Color(0, 255, 0));
    pixels.setPixelColor(5, pixels.Color(0, 255, 0));
    pixels.setPixelColor(6, pixels.Color(0, 0, 0));
    pixels.setPixelColor(7, pixels.Color(0, 0, 0));
      }          


    else if (fuelLevel == 7) {     
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.setPixelColor(1, pixels.Color(255, 128, 0));
    pixels.setPixelColor(2, pixels.Color(255, 255, 0));
    pixels.setPixelColor(3, pixels.Color(255, 255, 0));
    pixels.setPixelColor(4, pixels.Color(0, 255, 0));
    pixels.setPixelColor(5, pixels.Color(0, 255, 0));
    pixels.setPixelColor(6, pixels.Color(0, 255, 0));
    pixels.setPixelColor(7, pixels.Color(0, 0, 0));
    pixels.show();
      }          

    else if (fuelLevel == 8) {     /* Full Gauge. No flash */
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.setPixelColor(1, pixels.Color(255, 128, 0));
    pixels.setPixelColor(2, pixels.Color(255, 255, 0));
    pixels.setPixelColor(3, pixels.Color(255, 255, 0));
    pixels.setPixelColor(4, pixels.Color(0, 255, 0));
    pixels.setPixelColor(5, pixels.Color(0, 255, 0));
    pixels.setPixelColor(6, pixels.Color(0, 255, 0));
    pixels.setPixelColor(7, pixels.Color(0, 255, 0));
    pixels.show();
      }          
      
    else if (fuelLevel == 9) {      /* Full Tank!!! Flashing top segment */
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.setPixelColor(1, pixels.Color(255, 128, 0));
    pixels.setPixelColor(2, pixels.Color(255, 255, 0));
    pixels.setPixelColor(3, pixels.Color(255, 255, 0));
    pixels.setPixelColor(4, pixels.Color(0, 255, 0));
    pixels.setPixelColor(5, pixels.Color(0, 255, 0));
    pixels.setPixelColor(6, pixels.Color(0, 255, 0));
    pixels.setPixelColor(7, pixels.Color(0, 255, 0));
    pixels.show();
    delay(500);                                       
    pixels.setPixelColor(7, pixels.Color(0, 0, 0));     
    pixels.show();                                  
    delay(500);
      }
    
  /*/Controlling HAT BULB brightness with PWM based on fuelLevel > 0/*/
  /*/////////////////////////////////////////////////////////////////*/

    if (fuelLevel > 0) {      /* Light gets proportionallly brighter from 1-9 fuelLevel*/
    hatBright = map(fuelLevel, 1, 9, 50, 255);  /*LED brightness. Maps fuelLevel to pwm Min/Max*/
    analogWrite(HATLED, hatBright);   /*PWM output to trigger transistor circuit*/
    }
}

void hatFlicker() {           /*First go at a flicker algo - only called currently for 0 fuel*/

  flicker = random(5,1000);  /*random flicker intervals*/
  unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= flicker) {   /*IF loop to use millis() without a delay() in the loop*/
      previousMillis = currentMillis;
    if (ledState == LOW) {  
      ledState = random(10, 50 );     /*Turns light on with random brightness, not tied to fuel state*/
    } else {
      ledState = LOW;
    }
    analogWrite(HATLED, ledState);
  }
}

void lowFuelFlash() {           /*Same concept to flash the red segment without delay()*/

  unsigned long currentLowFuelMillis = millis();
    if (currentLowFuelMillis - previousLowFuelMillis >= lowFuelRate) {
      previousLowFuelMillis = currentLowFuelMillis;
    if (fuelWarning == 0) {
      pixels.setPixelColor(0, pixels.Color(255, 0, 0));  
      fuelWarning = 1;
      pixels.show();  
    } else {
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));  
      fuelWarning = 0;
      pixels.show();  
    }
  }
}
