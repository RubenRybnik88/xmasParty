#include <FastLED.h> /* Arduino library to drive LED panel*/
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include <Fonts/FreeSans9pt7b.h>
#include <avr/pgmspace.h>

#define HATLED 5 /* Arduino pin for the hat lightbulb PWM dimmer*/
#define NUM_LEDS 256 /*Panel Array size*/
#define DATA_PIN 6 /*Arduino pin for LED Array*/
#define ledBrightness 30 /*Glloball brightness, do not use over 100*/
#define COLOR_ORDER GRB /*LED Colour Order*/

  /*/Setting up 8-segment LED Matrix/*/
  /*/////////////////////////////////*/

CRGB rawleds[NUM_LEDS]; /*LED Matrix Array data object*/

CRGBSet leds(rawleds, NUM_LEDS);

CRGBSet seg1(leds(0,31));    /*Splitting into 8 segment display*/
CRGBSet seg2(leds(32,63));  
CRGBSet seg3(leds(64,95));  
CRGBSet seg4(leds(96,127));  
CRGBSet seg5(leds(128,159));  
CRGBSet seg6(leds(160,191));  
CRGBSet seg7(leds(192,223));  
CRGBSet seg8(leds(224,255)); 

struct CRGB * ledarray[] ={seg1, seg2, seg3, seg4, seg5, seg6, seg7, seg8}; 

#define FRAMES_PER_SECOND  120  /*For LED panel animations*/

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 8, DATA_PIN, /*For LED panel animations*/
  NEO_MATRIX_TOP    + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);
  
const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(255, 255, 0),matrix.Color(0, 0, 255), matrix.Color(255, 0, 255), matrix.Color(0, 255, 255), matrix.Color(255, 255, 255)};

  /*/Main code variables/*/
  /*/////////////////////*/

int BUTTON = 3; /* Arduino pin for the push button fuelguage filler*/
int buttonTime = 750; /* Time that will be considered as the long-press time for pushbutton*/
int previousState = LOW; /* Setting the initial state of push button LOW*/
int presentState;  /* Variable that will store the present state of the button*/
unsigned long pressTime = 0; /* Time at which the button was pressed */
unsigned long releaseTime = 0; /* Time at which the button was released */
unsigned long lastPressTime = 0; /* Time at which the button was pressed last time */
int doubleClick = 350; /*Tiem constant for a double-click */
int fuelLevel = 5; /* Level in fuel tank [IMPORTANT] */
int fillRate = 700;  /* How fast the tank fills up per segment */
int drainRate = 1500;  /* How fast the tank drains (controlled by Potentiometer) */
int potValue; /* Potentiometer variable for drainRate potentiometer*/
int hatBright = 0; /* store PWM brightness for HAT*/
int long flicker; /*random number container*/
int ledState = LOW;  /* ledState used to set the HATLED in Flicker*/
unsigned long previousMillis = 0;  /*will store last time HATLED was updated*/
unsigned long previousLowFuelMillis = 0;  /*will store last time Low Fuel LED was updated*/
int lowFuelRate = 500; /*Low Fuel flash rate*/
int fuelWarning = 1; /*Stores LowFuel indicator On or Off?*/
int deviceMode = 0; /*Stores operating mode*/ 
int numModes = 5; /*Stores number of different modes*/ 
int demoMode = 0; /*DEMO mode container*/ 

void setup() {
  
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(HATLED, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(3),cancelDemo,RISING);  /*Interrupt to exit Demo Mode on any button-press*/
  
  Serial.begin(9600);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(rawleds, NUM_LEDS);
  FastLED.setBrightness(ledBrightness);
  FastLED.clear(true);

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setTextColor(colors[0]);
  matrix.setBrightness(30);

}

  /*/Setting up MuthaTruckin DANCE MODE/*/
  /*////////////////////////////////////*/
  
// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { textScroll, rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

int x    = matrix.width();
int pass = 0;

void loop() {

  /* DEBUGGING SERIAL OUTPUT
  
  Serial.println((String)"Fuel: "+fuelLevel);                               
  Serial.println((String)"DrainRate: "+drainRate);                               
  Serial.println((String)"Button: "+presentState);                        
  Serial.println((String)"Mode: "+deviceMode);
  Serial.println((String)"DemoMode: "+demoMode);
  
  */

  /*/Controlling drainRate with a Potentiometer/*/
  /*/////////////////////////////////////////////*/
  //DISABLED - ENABLE IF USING POTENTIOMETER
  //potValue = (1023- analogRead(A0));                        /*Potentiometer wired to pin A0 to enable analogRead*/ 
  //drainRate = map(potValue, 0, 1023, 200, 10000);           /*Min/Max value for how fast beer empties*/ 

  /*/Controlling drainRate with mode button//////////*/
  /*/////////////////////////////////////////////////*/
  
  drainRate = map(deviceMode, 0, numModes, 350, 10000);           /*Min/Max value for how fast beer empties*/ 

  /*Filling and Emptying the Tank based on push-button/*/       /* Actions for short press, long-press and double-press */
  /*///////////////////////////////////////////////////*/
  
  presentState = (1-digitalRead(BUTTON));                      /* Getting the present state of the push button */

  if (previousState == LOW && presentState == HIGH) {      /* If button is pressed, timestamp the initial press-time and flag button as closed*/
    pressTime = millis();
    previousState = HIGH;
  }
  
  else if (previousState == HIGH && presentState == HIGH && fuelLevel < 9) {    /* If button is closed for >buttonTime, start filling fuelLevel at fillRate up to max 9*/
    if ((millis() - pressTime) > buttonTime) {
      fuelLevel = fuelLevel += 1;
      delay(fillRate);
    }
  }

  else if (previousState == HIGH && presentState == LOW) {  /* When button is released, record the release-time and flag button to open.*/

           if ((pressTime - lastPressTime) < doubleClick) {      /* If a double click, go to DemoMode*/
                demoMode = 1;
                delay(20);
             }
               
           else if ((millis() - pressTime) < buttonTime) {      /* If a short press, cycle the MODE to the next mode up to max N modes.*/
             if (deviceMode < numModes) {
                deviceMode = deviceMode += 1;
                delay(20);
             }              
              else deviceMode = 0;
                delay(20);
           }
            releaseTime = millis();
            previousState = LOW;
            lastPressTime = pressTime;                   /* Comparator to test for double-press */
  }
    
  else if (previousState == LOW && presentState == LOW && fuelLevel > 0) {       /* Once button is open, empty fuelLevel at drainRate down to 0*/
    if ((millis() - releaseTime) > drainRate) {
    fuelLevel = fuelLevel -= 1;
    releaseTime = millis();
    }
  }

  /*///////DEMO MODE LED PANEL outputs////////*/
  /*//////////////////////////////////////////*/

      while (demoMode == 1) {                                /* LIGHTSHOW MODE - Party time */
        partyTime();
      }

  /*/Controlling LED panel based on fuelLevel/*/
  /*//////////////////////////////////////////*/

      if (fuelLevel == 0 && demoMode == 0) {      /* Empty Tank!!! Flashing and Hat LED flicker*/
      hatFlicker();
      lowFuelFlash();  
      }

    else if (fuelLevel == 1) {      /*Segment 1 = RED*/
      fill_solid(ledarray[0], 32, CRGB::Red);
      fill_solid(ledarray[1], 32, CRGB::Black);
      fill_solid(ledarray[2], 32, CRGB::Black);
      fill_solid(ledarray[3], 32, CRGB::Black);
      fill_solid(ledarray[4], 32, CRGB::Black);
      fill_solid(ledarray[5], 32, CRGB::Black);    
      fill_solid(ledarray[6], 32, CRGB::Black);
      fill_solid(ledarray[7], 32, CRGB::Black);    
      FastLED.show();
      }          

    else if (fuelLevel == 2) {    
      fill_solid(ledarray[0], 32, CRGB::Red);
      fill_solid(ledarray[1], 32, CRGB::DarkOrange);
      fill_solid(ledarray[2], 32, CRGB::Black);
      fill_solid(ledarray[3], 32, CRGB::Black);
      fill_solid(ledarray[4], 32, CRGB::Black);
      fill_solid(ledarray[5], 32, CRGB::Black);    
      fill_solid(ledarray[6], 32, CRGB::Black);
      fill_solid(ledarray[7], 32, CRGB::Black);    
      FastLED.show();
      }          

    else if (fuelLevel == 3) {      
      fill_solid(ledarray[0], 32, CRGB::Red);
      fill_solid(ledarray[1], 32, CRGB::DarkOrange);
      fill_solid(ledarray[2], 32, CRGB::Yellow);
      fill_solid(ledarray[3], 32, CRGB::Black);
      fill_solid(ledarray[4], 32, CRGB::Black);
      fill_solid(ledarray[5], 32, CRGB::Black);    
      fill_solid(ledarray[6], 32, CRGB::Black);
      fill_solid(ledarray[7], 32, CRGB::Black);    
      FastLED.show();
      }          

    else if (fuelLevel == 4) {    
      fill_solid(ledarray[0], 32, CRGB::Red);
      fill_solid(ledarray[1], 32, CRGB::DarkOrange);
      fill_solid(ledarray[2], 32, CRGB::Yellow);
      fill_solid(ledarray[3], 32, CRGB::Yellow);
      fill_solid(ledarray[4], 32, CRGB::Black);
      fill_solid(ledarray[5], 32, CRGB::Black);    
      fill_solid(ledarray[6], 32, CRGB::Black);
      fill_solid(ledarray[7], 32, CRGB::Black);    
      FastLED.show();
      }          

    else if (fuelLevel == 5) {     
      fill_solid(ledarray[0], 32, CRGB::Red);
      fill_solid(ledarray[1], 32, CRGB::DarkOrange);
      fill_solid(ledarray[2], 32, CRGB::Yellow);
      fill_solid(ledarray[3], 32, CRGB::Yellow);
      fill_solid(ledarray[4], 32, CRGB::Green);
      fill_solid(ledarray[5], 32, CRGB::Black);    
      fill_solid(ledarray[6], 32, CRGB::Black);
      fill_solid(ledarray[7], 32, CRGB::Black);    
      FastLED.show();
      }          


      
    else if (fuelLevel == 6) {     
      fill_solid(ledarray[0], 32, CRGB::Red);
      fill_solid(ledarray[1], 32, CRGB::DarkOrange);
      fill_solid(ledarray[2], 32, CRGB::Yellow);
      fill_solid(ledarray[3], 32, CRGB::Yellow);
      fill_solid(ledarray[4], 32, CRGB::Green);
      fill_solid(ledarray[5], 32, CRGB::Green);    
      fill_solid(ledarray[6], 32, CRGB::Black);
      fill_solid(ledarray[7], 32, CRGB::Black);    
      FastLED.show();
      }          


    else if (fuelLevel == 7) {     
      fill_solid(ledarray[0], 32, CRGB::Red);
      fill_solid(ledarray[1], 32, CRGB::DarkOrange);
      fill_solid(ledarray[2], 32, CRGB::Yellow);
      fill_solid(ledarray[3], 32, CRGB::Yellow);
      fill_solid(ledarray[4], 32, CRGB::Green);
      fill_solid(ledarray[5], 32, CRGB::Green);    
      fill_solid(ledarray[6], 32, CRGB::Green);    
      fill_solid(ledarray[7], 32, CRGB::Black);    
      FastLED.show();
      }          

    else if (fuelLevel == 8) {     /* Full Gauge. No flash */
      fill_solid(ledarray[0], 32, CRGB::Red);
      fill_solid(ledarray[1], 32, CRGB::DarkOrange);
      fill_solid(ledarray[2], 32, CRGB::Yellow);
      fill_solid(ledarray[3], 32, CRGB::Yellow);
      fill_solid(ledarray[4], 32, CRGB::Green);
      fill_solid(ledarray[5], 32, CRGB::Green);    
      fill_solid(ledarray[6], 32, CRGB::Green);
      fill_solid(ledarray[7], 32, CRGB::Green);    
      FastLED.show();
      }          
      
    else if (fuelLevel == 9) {      /* Full Tank!!! Flashing top segment */
      fill_solid(ledarray[0], 32, CRGB::Red);
      fill_solid(ledarray[1], 32, CRGB::DarkOrange);
      fill_solid(ledarray[2], 32, CRGB::Yellow);
      fill_solid(ledarray[3], 32, CRGB::Yellow);
      fill_solid(ledarray[4], 32, CRGB::Green);
      fill_solid(ledarray[5], 32, CRGB::Green);    
      fill_solid(ledarray[6], 32, CRGB::Green);
      fill_solid(ledarray[7], 32, CRGB::Green);    
      FastLED.show(); 
      delay(500);                                       /*Uses delay(). Probably no issue as this is the end of a chain, so once*/
      fill_solid(ledarray[7], 32, CRGB::Black);         /*fuel <8 it won't interrupt/delay. HatLED is currently just set to max */
      FastLED.show();                                   /*Might need a rewrite if we want a fancier flicker algo for full tanks?*/
      delay(500);
    }
    
  /*/Controlling HAT BULB brightness with PWM based on fuelLevel > 0/*/
  /*/////////////////////////////////////////////////////////////////*/

    if (fuelLevel > 0) {      /* Light gets proportionallly brighter from 1-9 fuelLevel*/
    hatBright = map(fuelLevel, 1, 9, 50, 255);  /*LED brightness. Maps fuelLevel to pwm Min/Max*/
    analogWrite(HATLED, hatBright);   /*PWM output to trigger transistor circuit*/
    }
}

  /*/Setting up MuthaTruckin DANCE MODE/*/
  /*////////////////////////////////////*/
  
void partyTime()
{
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 3 ) { FastLED.clear();  nextPattern(); } // change patterns periodically
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  uint8_t dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void textScroll() {

  matrix.clear();
  matrix.setCursor(2, 1);
  matrix.print(F("DYSON"));
  matrix.show();

}

  /*/Setting up code for LED Flicker etc/*/
  /*////////////////////////////////////*/


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

void lowFuelFlash() {           /*NEED BEER - Same concept to flash the red segment without delay()*/

    unsigned long currentLowFuelMillis = millis();
    if (currentLowFuelMillis - previousLowFuelMillis >= lowFuelRate) {
      previousLowFuelMillis = currentLowFuelMillis;
    if (fuelWarning == 0) {
      matrix.clear();
      matrix.fill(CRGB::Red, 0, 32);
      matrix.fill(CRGB::Red, 224, 255);
      matrix.setCursor(5, 1);
      matrix.print(F("LOW"));
      matrix.show();
      fuelWarning = 1;
    } else {
      matrix.clear();
      matrix.setCursor(5, 1);
      matrix.print(F("FUEL !"));
      matrix.show();
      fuelWarning = 0;
      FastLED.show();
    }
  }
}

void cancelDemo() {
  demoMode = 0;
  FastLED.clear();
}
