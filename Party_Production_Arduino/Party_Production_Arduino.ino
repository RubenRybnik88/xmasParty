#include <FastLED.h>
#include <OneButton.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_GFX.h>

#define NUM_LEDS 256
#define DATA_PIN 6
#define BRIGHTNESS 50
#define UPDATES_PER_SECOND 25
#define HEADLAMP_PIN 4
#define BUTTON_PIN 2
#define LONG_PRESS_START_MS 500
#define CHANGE_DE_FUEL_RATE_CLICKS 1
#define CHANGE_RE_FUEL_RATE_CLICKS 2
#define PARTY_TIME_CLICKS 3
#define LIGHTS_OUT_CLICKS 4
#define LIGHTS_ON_CLICKS 5
#define FUEL_FULL 8
#define FUEL_EMPTY 0
#define LED_BRIGHTNESS  20  // Don't go up to 100
#define FULL_FUEL_BLINK_RATE  1000
#define UPDATES_PER_SECOND 25
#define NUM_FUEL_LEVELS 8
#define DISPLAYROWS 32
#define DISPLAY_WIDTH 8
#define ARROWWIDTH 6
#define ARROWSPACING 8
#define PIXELSPERROW 8
#define ROWS_PER_SEGMENT 4
#define FRAMES_PER_SECOND  120 

// constants won't change. They're used here to set pin numbers:
const int headLamp = HEADLAMP_PIN;
int longPressCounter = 0;
int longPressStartTime = 0;
int ledState = 0;
int fuelingDISPLAYROWS = 0;

enum deFuelRateEnum {
  slowdefuel = 10000,  // 10s
  mediumdefuel = 5000, // 5s
  fastdefuel = 1000    // 1s
} deFuelRate;

enum reFuelRateEnum {
  slowrefuel = 1000,  // 1s/segment (8 gulps)
  mediumrefuel = 500, // 500ms/segment (4 gulps to full)
  fastrefuel = 250    // 250ms/segment (2 gulps to full)
} reFuelRate;

enum systemStateEnum {
  deFueling = 0,
  reFueling = 1,
  partyTime = 2,
  lightsOut = 3,
  lightsOn = 4,
  transitionToDeFueling = 5
} systemState;

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CHSVPalette16 fuelLevelPalette(
    CHSV(0, 255, 255),
    CHSV(3, 255, 255),
    CHSV(6, 255, 255),
    CHSV(9, 255, 255),
    CHSV(12, 255, 255),
    CHSV(15, 255, 255),
    CHSV(18, 255, 255),
    CHSV(21, 255, 255),
    CHSV(24, 255, 255),
    CHSV(48, 255, 255),
    CHSV(60, 255, 255),
    CHSV(72, 255, 255),
    CHSV(78, 255, 255),
    CHSV(84, 255, 255),
    CHSV(90, 255, 255),
    CHSV(96, 255, 255)
);

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

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 8, DATA_PIN, /*For LED panel animations*/
  NEO_MATRIX_TOP    + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);

int x    = matrix.width();
int pass = 0;
  
const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(255, 255, 0),matrix.Color(0, 0, 255), matrix.Color(255, 0, 255), matrix.Color(0, 255, 255), matrix.Color(255, 255, 255)};

// variables will change:
int buttonState = 0;  // variable for reading the pushbutton status
bool systemStateChanged = true;
unsigned long fuelCounterMillis = 0;
int fuelLevel = FUEL_FULL; // start full

// Setup a new OneButton on button pin.  
OneButton button(BUTTON_PIN, true, true);

void setup() {
    // open the serial port at 9600 bps:
    Serial.begin(9600);
    
    button.setPressTicks(LONG_PRESS_START_MS);

    // Single Click event attachment
    button.attachClick(singleClickHandler); 
    button.attachDoubleClick(doubleClickHandler);
    button.attachMultiClick(multiClickHandler);
    button.attachLongPressStart(longPressStartHandler);
    //button.attachDuringLongPress(longPressHandler);
    button.attachLongPressStop(longPressStopHandler);


    // initialize digital pin LED_BUILTIN as an output.
    pinMode(headLamp, OUTPUT);

    // Set the initial system state
    systemState = deFueling;
    fuelLevel = FUEL_FULL;
    systemStateChanged = true;
    deFuelRate = mediumdefuel;
    reFuelRate = mediumrefuel;

    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  
    FastLED.setMaxPowerInVoltsAndMilliamps(5,200);	
    currentPalette = fuelLevelPalette;
    currentBlending = NOBLEND;
    FastLED.clear(true);

    matrix.begin();
    matrix.setTextWrap(false);
    matrix.setTextColor(colors[0]);
    matrix.setBrightness(30);
}




void loop() {
  button.tick();

  switch (systemState)
  {
    case deFueling:
      if (systemStateChanged == true) { // First time state entry
        Serial.println("De Fueling!");
        systemStateChanged = false;
        fuelCounterMillis = millis();
        setFuelLevel();
        setHeadLamp();
        drawDropFuelAnimation(true);
        break;
      }

      // determine fuel level
      setFuelLevel();

      // set headlamp
      setHeadLamp();

      // draw defueling animation
      //drawDeFuelingAnimation();
      if (fuelLevel != 0) {
        drawDropFuelAnimation(false);
      }
      else {
        lowFuelFlash();
      }

      break;
    case reFueling:
      if (systemStateChanged == true) {
        Serial.println("Re Fueling!");
        systemStateChanged = false;
        fuelCounterMillis = millis() - 500;
        setFuelLevel();
        pulseHeadLamp();
        Arrow(true);
        break;
      }

      // determine fuel level
      setFuelLevel();

      // pulse headlamp
      pulseHeadLamp();

      // draw refueling animation
      Arrow(false);

      break;

      
    case partyTime:
      if (systemStateChanged == true) {
        Serial.println("Party Time!");
        systemStateChanged = false;
      }

        partyTimeMode();
        
      break;

      
    case lightsOut:
      if (systemStateChanged == true) {
        Serial.println("Lights Off!");
        systemStateChanged = false;
      }
      break;

      
    case lightsOn:
      if (systemStateChanged == true) {
        Serial.println("Lights On!");
        systemStateChanged = false;
      }
      break;
    case transitionToDeFueling:
      if (systemStateChanged == true) {
        Serial.println("De Fueling Transition!");
        systemStateChanged = false;
      }

      // play transition animation

      // if transition animation is finished, go to defueling state

      systemState = deFueling;
      systemStateChanged = true;
      break;
    default:
      break;
  }
}

bool setFuelLevel() {
  unsigned long currentMillis = millis();

  if (systemState == deFueling) {
    if (fuelLevel != 0) {
      if (currentMillis - fuelCounterMillis > deFuelRate) {
        fuelLevel--;
        Serial.println("Fuel Level: ");
        Serial.println(fuelLevel);
        fuelCounterMillis = currentMillis;
        return true;  // fuel level changed, tell calling function
      }
    }
  }
  else if (systemState == reFueling) {
    if (fuelLevel != FUEL_FULL) {
      if (currentMillis - fuelCounterMillis > reFuelRate) {
        fuelLevel++;
        Serial.println("Fuel Level: ");
        Serial.println(fuelLevel);
        fuelCounterMillis = currentMillis;
        return true;  // fuel level changed, tell calling function
      }
    }
  }
  return false; // fuel level didn't change, tell calling function
}

void pulseHeadLamp() {
  static float in = 4.712;
  float out;

  in = in + 0.001;
  if (in > 10.995) {
    in = 4.712;
  }
  out = sin(in) * 127.5 + 127.5;
  analogWrite(headLamp, out);
}

void setHeadLamp() {
  static unsigned long previousMillis = 0;
  unsigned long currentMillis;
  int headLampBrightness;
  static int long flicker = 0;
  static int ledState = 0;

  if (fuelLevel > 5) {      /* Light gets proportionally brighter from 1-9 fuelLevel*/
    headLampBrightness = map(fuelLevel, 1, FUEL_FULL, 50, 255);  /*LED brightness. Maps fuelLevel to pwm Min/Max*/
    analogWrite(headLamp, headLampBrightness);   /*PWM output to trigger transistor circuit*/
  }
  else {
    currentMillis = millis();
    if (currentMillis - previousMillis >= flicker) {   /*IF loop to use millis() without a delay() in the loop*/
      previousMillis = currentMillis;
      if (ledState == 0) {  
        ledState = random(10, map(fuelLevel, 1, FUEL_FULL, 50, 255));/*Turns light on with random brightness, not tied to fuel state*/
        flicker = random(10, map(fuelLevel, 1, FUEL_FULL, 100, 3000));
      } else {
        ledState = 0;
        flicker = random(10, 100);
      }
      analogWrite(headLamp, ledState);
    }
  }
}

void drawDropFuelAnimation(bool stateStart) {
  int topRow;
  CRGB colour;
  static int lastFuelLevel;
  static int startingPixel;
  static bool playDropAnimation;
  static bool dropAnimationIsFinished;
  static int dropCounter;
  static unsigned long previousMillis = 0;
  unsigned long currentMillis; 

  currentMillis = millis();

  if (stateStart == true) {
    lastFuelLevel = fuelLevel;
    playDropAnimation = false;
    dropAnimationIsFinished = true;
    startingPixel = 0;
    dropCounter = 4;
  }

  FastLED.clear();

  if (lastFuelLevel != fuelLevel) {  // fuel level changed
    //playDropAnimation = true;
    //dropAnimationIsFinished = false;

    startingPixel = lastFuelLevel * DISPLAY_WIDTH * ROWS_PER_SEGMENT;
    previousMillis = currentMillis;
    dropCounter = 0; 
    lastFuelLevel = fuelLevel;
  }

  if (dropCounter < 4) {
    if (currentMillis - previousMillis > 250) {
          startingPixel = startingPixel - DISPLAY_WIDTH;
          dropCounter++;
          previousMillis = currentMillis;
    } 

    // Paint level drop
    //startingPixel = lastFuelLevel * DISPLAY_WIDTH * ROWS_PER_SEGMENT;
    colour = ColorFromPalette(currentPalette, startingPixel - 1, BRIGHTNESS, currentBlending);
    //colour = ColorFromPalette(currentPalette, (lastFuelLevel * ROWS_PER_SEGMENT * 8) - 1, BRIGHTNESS, currentBlending);
    //rowColour = ColorFromPalette(currentPalette, 255, BRIGHTNESS, currentBlending);
    
    for (int i = DISPLAY_WIDTH; i > 0; i--) {
      leds[startingPixel - i] = colour;
    }
  }

  // Paint segments according to fuelLevel
  /*if (playDropAnimation == true) {
    if (currentMillis - previousMillis > 250) {
        // Paint level drop
        colour = ColorFromPalette(currentPalette, startingPixel - 1, BRIGHTNESS, currentBlending);
        for (int i = DISPLAY_WIDTH; i > 0; i--) {
          leds[startingPixel - i] = colour;
        }
        startingPixel = startingPixel - DISPLAY_WIDTH;

        // if startingPixel is in current top fuel segment, animation is finished
        dropAnimationIsFinished = true;

        previousMillis = currentMillis;
    } 

    // Paint level drop
    colour = ColorFromPalette(currentPalette, (lastFuelLevel * ROWS_PER_SEGMENT * 8) - 1, BRIGHTNESS, currentBlending);
    //rowColour = ColorFromPalette(currentPalette, 255, BRIGHTNESS, currentBlending);
    int startingPixel = lastFuelLevel * DISPLAY_WIDTH * ROWS_PER_SEGMENT;
    for (int i = DISPLAY_WIDTH; i > 0; i--) {
      leds[startingPixel - i] = colour;
    }

    if (dropAnimationIsFinished == true) {
      playDropAnimation = false;
    }
  }*/

  // draw the drop line
  //colour = ColorFromPalette(currentPalette, startingPixel - 1, BRIGHTNESS, currentBlending);
  //for (int i = DISPLAY_WIDTH; i > 0; i--) {
  //  leds[startingPixel - i] = colour;
  //}

  for (int i = 0; i < fuelLevel; i++) {
    colour = ColorFromPalette(currentPalette,((i + 1) *32)-1, BRIGHTNESS, currentBlending);
    fill_solid(ledarray[i], 32, colour);
  }

  FastLED.show();
}

void drawDeFuelingAnimation() {
  static unsigned long previousMillis = 0;
  static bool isFullFuelSegOn = false;
  unsigned long currentMillis;
  FastLED.show();

  if (fuelLevel == 0) {      /* Empty Tank!!! Flashing and Hat LED flicker*/
  //hatFlicker();
  //lowFuelFlash();  
    fill_solid(ledarray[0], 32, CRGB::Black);
    fill_solid(ledarray[1], 32, CRGB::Black);
    fill_solid(ledarray[2], 32, CRGB::Black);
    fill_solid(ledarray[3], 32, CRGB::Black);
    fill_solid(ledarray[4], 32, CRGB::Black);
    fill_solid(ledarray[5], 32, CRGB::Black);    
    fill_solid(ledarray[6], 32, CRGB::Black);
    fill_solid(ledarray[7], 32, CRGB::Black);    
    FastLED.show();
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
  /*else if (fuelLevel == 8) {     // Full Gauge. No flash 
    fill_solid(ledarray[0], 32, CRGB::Red);
    fill_solid(ledarray[1], 32, CRGB::DarkOrange);
    fill_solid(ledarray[2], 32, CRGB::Yellow);
    fill_solid(ledarray[3], 32, CRGB::Yellow);
    fill_solid(ledarray[4], 32, CRGB::Green);
    fill_solid(ledarray[5], 32, CRGB::Green);    
    fill_solid(ledarray[6], 32, CRGB::Green);
    fill_solid(ledarray[7], 32, CRGB::Green);    
    FastLED.show();
  }*/          
  else if (fuelLevel == 8) {      /* Full Tank!!! Flashing top segment */
    fill_solid(ledarray[0], 32, CRGB::Red);
    fill_solid(ledarray[1], 32, CRGB::DarkOrange);
    fill_solid(ledarray[2], 32, CRGB::Yellow);
    fill_solid(ledarray[3], 32, CRGB::Yellow);
    fill_solid(ledarray[4], 32, CRGB::Green);
    fill_solid(ledarray[5], 32, CRGB::Green);    
    fill_solid(ledarray[6], 32, CRGB::Green);

    currentMillis = millis();
    if (currentMillis - previousMillis >= FULL_FUEL_BLINK_RATE) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;

      // if the segment is off turn it on and vice-versa:
      if (isFullFuelSegOn == false) { 
        fill_solid(ledarray[7], 32, CRGB::Green);
        isFullFuelSegOn = true;
      } else {
        fill_solid(ledarray[7], 32, CRGB::Black);
        isFullFuelSegOn = false;
      }
      //FastLED.show();
    } 
  }
  FastLED.show();
}

void Arrow(bool stateStart) {
  static int frontArrowIndex = 0; // the position of the arrow. starts at the bottom of the array
  static unsigned long previousMillis = 0;
  unsigned long currentMillis; 
  int rowOne;
  int rowTwo;
  int rowThree;
  int rowFour;
  int rowFive;
  int rowSix;
  int basePixel;
  CRGB rowColour;
  int arrowNumber = 0;
  bool stillPaintingArrows = true;

  if (stateStart == true) {
    frontArrowIndex = 0;
    previousMillis = 0;
  }
  
  currentMillis = millis();
  if (currentMillis - previousMillis >= (1000 / UPDATES_PER_SECOND)) {
      // save the last time you updated the animation
      previousMillis = currentMillis;
  } 
  else {  // not yet time to update animation
    return;
  }

  FastLED.clear();
  fuelingDISPLAYROWS = fuelLevel * 4;
  if ((frontArrowIndex - ARROWWIDTH) > fuelingDISPLAYROWS - 1) // If the last row of the future lead arrow would not be on the display set the new lead arrow index to the one before
  {
    frontArrowIndex = frontArrowIndex - ARROWSPACING;
  }
  
  while (stillPaintingArrows) {
    // Calculate the row indexes for this arrow
    rowOne = frontArrowIndex - (arrowNumber * ARROWSPACING);
    rowTwo = rowOne - 1;
    rowThree = rowOne - 2;
    rowFour = rowOne - 3;
    rowFive = rowOne - 4;
    rowSix = rowOne - 5; 
    arrowNumber++; // Advance the arrow counter for next loop

    if ((rowOne >= 0) && (rowOne < fuelingDISPLAYROWS + ARROWWIDTH)) { // Only paint the arrow if one of its lines is on the screen 
      // Paint top row - 00011000
      if ((rowOne >= 0) && (rowOne < 32)) { // Only paint if it is on the display
        rowColour = ColorFromPalette( currentPalette, rowOne * 8, BRIGHTNESS, currentBlending);
        basePixel = rowOne * PIXELSPERROW;
        leds[basePixel+3] = rowColour;
        leds[basePixel+4] = rowColour;
      }
      
      // Paint row 2 - 00111100
      if ((rowTwo >= 0) && (rowTwo < 32)) { // Only paint if it is on the display
        rowColour = ColorFromPalette( currentPalette, rowTwo * 8, BRIGHTNESS, currentBlending);
        basePixel = rowTwo * PIXELSPERROW;
        leds[basePixel+2] = rowColour;
        leds[basePixel+3] = rowColour;
        leds[basePixel+4] = rowColour;
        leds[basePixel+5] = rowColour;
      }

      // Paint row 3 - 01111110
      if ((rowThree >= 0) && (rowThree < 32)) { // Only paint if it is on the display
        rowColour = ColorFromPalette( currentPalette, rowThree * 8, BRIGHTNESS, currentBlending);
        basePixel = rowThree * PIXELSPERROW;
        leds[basePixel+1] = rowColour;
        leds[basePixel+2] = rowColour;
        leds[basePixel+3] = rowColour;
        leds[basePixel+4] = rowColour;
        leds[basePixel+5] = rowColour;
        leds[basePixel+6] = rowColour;
      }

      // Paint row 4 - 11100111
      if ((rowFour >= 0) && (rowFour < 32)) { // Only paint if it is on the display
        rowColour = ColorFromPalette( currentPalette, rowFour * 8, BRIGHTNESS, currentBlending);
        basePixel = rowFour * PIXELSPERROW;
        leds[basePixel] = rowColour;
        leds[basePixel+1] = rowColour;
        leds[basePixel+2] = rowColour;
        leds[basePixel+5] = rowColour;
        leds[basePixel+6] = rowColour;
        leds[basePixel+7] = rowColour;
      }

      // Paint row 5 - 11000011
      if ((rowFive >= 0) && (rowFive < 32)) { // Only paint if it is on the display
        rowColour = ColorFromPalette( currentPalette, rowFive * 8, BRIGHTNESS, currentBlending);
        basePixel = rowFive * PIXELSPERROW;
        leds[basePixel] = rowColour;
        leds[basePixel+1] = rowColour;
        leds[basePixel+6] = rowColour;
        leds[basePixel+7] = rowColour;
      }

      // Paint row 6 - 10000001
      if ((rowSix >= 0) && (rowSix < 32)) { // Only paint if it is on the display
        rowColour = ColorFromPalette( currentPalette, rowSix * 8, BRIGHTNESS, currentBlending);
        basePixel = rowSix * PIXELSPERROW;
        leds[basePixel] = rowColour;
        leds[basePixel+7] = rowColour;
      }
    }
    else {  // None of the arrow was on the display so advance starting position for next call and exit
      frontArrowIndex++; // advance the starting position for the next arrow call
      
      stillPaintingArrows = false;  // exit the function
    }
  }
  FastLED.show();
}

void lowFuelFlash() {           /*LOW FUEL !*/
    static unsigned long previousLowFuelMillis = 0;
    static int fuelWarning = 0;
    

    unsigned long currentLowFuelMillis = millis();
    if (currentLowFuelMillis - previousLowFuelMillis >= 500) {
      previousLowFuelMillis = currentLowFuelMillis;
      if (fuelWarning == 0) {
        matrix.clear();
        matrix.fill(CRGB::Red, 0, 32);
        matrix.fill(CRGB::Red, 224, 255);
        matrix.setCursor(7, 1);
        matrix.print(F("LOW"));
        matrix.show();
        fuelWarning = 1;
        Serial.println("Low Fuel Flash 1");
      } else {
        matrix.clear();
        matrix.setCursor(7, 1);
        matrix.print(F("FUEL !"));
        matrix.show();
        fuelWarning = 0;
        Serial.println("Low Fuel Flash 2");
      }
  }
}

// Handler function for a single click:
static void singleClickHandler() {
  if (deFuelRate == mediumdefuel) {
    deFuelRate = fastdefuel;
  }
  else if (deFuelRate == fastdefuel) {
    deFuelRate = slowdefuel;
  }
  else {
    deFuelRate = mediumdefuel;
  }

  Serial.print("Changing DeFuel Rate!: ");
  Serial.println(deFuelRate);
}

// Handler function for a double click:
static void doubleClickHandler() {
  if (reFuelRate == mediumrefuel) {
    reFuelRate = fastrefuel;
  }
  else if (reFuelRate == fastrefuel) {
    reFuelRate = slowrefuel;
  }
  else {
    reFuelRate = mediumrefuel;
  }
  Serial.print("Changing ReFuel Rate!: ");
  Serial.println(reFuelRate);
}

// Handler function for a multi click:
static void multiClickHandler() {
  //Serial.print("Multi Clicked!: ");
  switch(button.getNumberClicks()) {
    case PARTY_TIME_CLICKS:
      if (systemState != partyTime) {
        systemState = partyTime;
      }
      else {
        systemState = deFueling;
        fuelLevel = FUEL_FULL;
      }
      
      systemStateChanged = true;
      break;
      
    case LIGHTS_OUT_CLICKS:
      systemState = lightsOut;
      systemStateChanged = true;
      break;
      
    case LIGHTS_ON_CLICKS:
      systemState = lightsOn;
      systemStateChanged = true;
      break;
  }
}

// Handler function for a single click:
static void longPressStartHandler() {
  Serial.println("Long Press Started!");
  systemState = reFueling;
  systemStateChanged = true;
  longPressStartTime = millis() - LONG_PRESS_START_MS;
}

// Handler function for a single click:
static void longPressHandler() {
  //Serial.println("Long Press In Progress!");
}

static void longPressStopHandler() {
  Serial.println("Long Press Ended!");
  //Serial.println((millis() - longPressStartTime));
  systemState = transitionToDeFueling;
  systemStateChanged = true;
}

  /*/Setting up MuthaTruckin DANCE MODE/*/
  /*////////////////////////////////////*/
  
// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void partyTimeMode(){
   // Flash on and off the HAT LED
  EVERY_N_MILLISECONDS( 250 ) { 
  if (ledState == LOW) {  
      ledState = random(10, 255);/*Turns light on with random brightness, not tied to fuel state*/
    } else {
      ledState = LOW;
    }
    analogWrite(headLamp, ledState);
    }

  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 3 ) { nextPattern(); } // change patterns periodically
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
