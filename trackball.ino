#include "PMW3360.h"
#include <Mouse.h>

/*
# PIN CONNECTION
  * MI = MISO
  * MO = MOSI
  * SS = Slave Select / Chip Select
  * SC = SPI Clock
  * MT = Motion (active low interrupt line)
  * RS = Reset
  * GD = Ground
  * VI = Voltage in up to +5.5V

Module   Arduino
  RS --- (NONE)
  GD --- GND
  MT --- (NONE)
  SS --- Pin_10   (use this pin to initialize a PMW3360 instance)
  SC --- SCK
  MO --- MOSI
  MI --- MISO
  VI --- 5V

 */

PMW3360 pmwSensor;

// Slave Select pin. Connect this to SS on the module.
constexpr static uint8_t SS_PIN = 18;
constexpr static uint8_t B1_PIN = 2;
constexpr static uint8_t B2_PIN = 3;
constexpr static uint8_t B3_PIN = 21;   // JOYSTICK BTN
constexpr static uint8_t B4_PIN = 4;
constexpr static uint8_t JOY_X = 20;
constexpr static uint8_t JOY_Y = 19;

// Trackball settings
//constexpr static uint16_t CPI = 800;
//constexpr static uint16_t TIME_TO_SCROLL = 150; // ms
constexpr static uint16_t TIME_TO_BUTTON = 10;  // ms
constexpr static uint16_t SCROLL_SPEED_CLAMP = 8;
//

PMW3360_DATA data;

//volatile int8_t scrollDir = 0;
//uint8_t lastEncoded = 0;
// ATTENTION: Right now there is no overflow guard. It will overflow in around
// 50 days. Who ever has this turned on for that long? If you do, add an
// overflow guard in the main loop
static unsigned long currentTime = 0;
//static unsigned long prevElapsedMillisEncoder = 0;
static unsigned long prevElapsedMillisButton = 0;
static uint8_t currentCPI = 1;

struct ButtonData {
  uint8_t pin;
  uint8_t mButton;
};
constexpr static uint8_t CPI_LEVEL_SIZE = 3;
constexpr static unsigned int CPI_LEVELS[CPI_LEVEL_SIZE] = {400 , 800, 1200};
constexpr static uint8_t NUM_BUTTONS = 4;
constexpr static ButtonData buttons[NUM_BUTTONS] = {
    {B1_PIN, MOUSE_LEFT}, {B2_PIN, MOUSE_RIGHT}, {B3_PIN, MOUSE_MIDDLE}, {B4_PIN, currentCPI}
};
uint8_t previousButtonVals[NUM_BUTTONS] = {false};

/* BUTTONS FUNCTIONS */
void setupButtons() {
  pinMode(B1_PIN, INPUT_PULLUP);
  pinMode(B2_PIN, INPUT_PULLUP);
  pinMode(B3_PIN, INPUT_PULLUP);
  pinMode(B4_PIN, INPUT_PULLUP);
}

setupJoystick() {
  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);
}

void readButtons() {
  if (currentTime - prevElapsedMillisButton < TIME_TO_BUTTON) {
    return;
  }
  for (int i = 0; i < NUM_BUTTONS - 1; i++) {
    uint8_t bVal = digitalRead(buttons[i].pin);
    if (bVal && previousButtonVals[i]) {
      Mouse.release(buttons[i].mButton);
      previousButtonVals[i] = false;
    } else if (!bVal) {
      Mouse.press(buttons[i].mButton);
      previousButtonVals[i] = true;
    }
  }

  constexpr uint8_t cpiButtonId = NUM_BUTTONS - 1;
  // the CPI button
    uint8_t bVal = digitalRead(buttons[cpiButtonId].pin);
    if (bVal && previousButtonVals[cpiButtonId]) {
      buttons[i].mButton = (buttons[cpiButtonIdi].mButton + 1) % CPI_LEVEL_SIZE;
      pmwSensor.SetCpi(CPI_LEVELS[buttons[i].mButton];
      //Mouse.release(buttons[i].mButton);
      previousButtonVals[cpiButtonId] = false;
    } else if (!bVal) {
      //Mouse.press(buttons[i].mButton);
      previousButtonVals[cpiButtonId] = true;
    }

  prevElapsedMillisButton = currentTime;
}

/* PMW3360 FUNCTIONS */
/*
# PMW3360_DATA struct format and description
  - PMW3360_DATA.isMotion      : bool, True if a motion is detected.
  - PMW3360_DATA.isOnSurface   : bool, True when a chip is on a surface
  - PMW3360_DATA.dx, data.dy   : integer, displacement on x/y directions.
  - PMW3360_DATA.SQUAL         : byte, Surface Quality register, max 0x80
                               * Number of features on the surface = SQUAL * 8
  - PMW3360_DATA.rawDataSum    : byte, It reports the upper byte of an 18‐bit
counter which sums all 1296 raw data in the current frame;
                               * Avg value = Raw_Data_Sum * 1024 / 1296
  - PMW3360_DATA.maxRawData    : byte, Max/Min raw data value in current frame,
max=127 PMW3360_DATA.minRawData
  - PMW3360_DATA.shutter       : unsigned int, shutter is adjusted to keep the
average raw data values within normal operating ranges.
*/
void readPMWSensor() {
  pmwSensor.readBurst(data);
  if (data.isOnSurface && data.isMotion) {
    Mouse.move(data.dy, data.dx, 0);
  }
}

/* ENCODER FUNCTIONS */
//void setupEncoder() {
//  pinMode(JOY_X, INPUT_PULLUP);
//  pinMode(JOY_Y, INPUT_PULLUP);
//  // We will be using interrupts to receive the encoder data
//  attachInterrupt(digitalPinToInterrupt(JOY_X), updateEncoder, CHANGE);
//  attachInterrupt(digitalPinToInterrupt(JOY_Y), updateEncoder, CHANGE);
//}
//
//void updateEncoder() {
//  int MSB = digitalRead(JOY_X); // MSB = most significant bit
//  int LSB = digitalRead(JOY_Y); // LSB = least significant bit
//
//  int encoded = (MSB << 1) | LSB; // Converting the 2 pin value to single number
//  int sum =
//      (lastEncoded << 2) | encoded; // Adding it to the previous encoded value
//
//  if (sum == 0b1101 || sum == 0b1011) {     //|| sum == 0b0100 || sum == 0b0010 
//    // Hard stop when decreasing from a positive scroll
//    if (scrollDir > 0)
//    {
//      scrollDir = 0;
//    }
//    else
//    {
//      --scrollDir;
//    }
//  }
//
//  if (sum == 0b1110 || sum == 0b0111 ){ //|| sum == 0b0001 || sum == 0b1000) {
//    // Hard stop when increasing from a negative scroll
//    if (scrollDir < 0)
//    {
//      scrollDir = 0;
//    }
//    else
//    {
//      ++scrollDir;
//    }
//  }
//  constrain(scrollDir, -SCROLL_SPEED_CLAMP, SCROLL_SPEED_CLAMP);
//  lastEncoded = encoded; // Store this value for next time
//}
//
//void updateScroll() {
//  if (currentTime - prevElapsedMillisEncoder < TIME_TO_SCROLL) {
//    return;
//  }
//  // this guard is for keeping the pro micro light off. It will seem like a
//  // disco ball blinking constantly if we always call Mouse.move
//  if (scrollDir != 0) {
//    Mouse.move(0, 0, scrollDir);
//  }
//  prevElapsedMillisEncoder = currentTime;
//}
void readJoystick()
{
  constexpr int SPEEDS_SIZE = 5;
  constexpr int SCROLL_SPEEDS[SPEEDS_SIZE] = {1,2,3,4,5};
  constexpr int CENTER = 345;
  constexpr int MAX = CENTER * 2;
  constexpr int DEADZONE = 35;
  constexpr int ZONE_SIZE = (CENTER) / SPEEDS_SIZE;
  uint8_t speedStage[2] = {0}; 

  int x = analogRead(JOY_X);
  if(x < CENTER - DEADZONE )
  {

  Serial.print("DOWN ZONE: ");
  Serial.println( (x - CENTER) / ZONE_SIZE  + 1);
  
  }
  else if(x > CENTER + DEADZONE)
  {
  Serial.print("UP ZONE: ");
  Serial.println( ((x - CENTER ) / ZONE_SIZE - 1));
  }

  int y = analogRead(JOY_Y);
  if(y < CENTER - DEADZONE )
  {

  Serial.print("LEFT ZONE: ");
  Serial.println( (y - CENTER) / ZONE_SIZE  + 1);
  
  }
  else if(x > CENTER + DEADZONE)
  {
  Serial.print("RIGHT ZONE: ");
  Serial.println( ((y - CENTER ) / ZONE_SIZE - 1));
  }
}

void setup() {
  setupButtons();
  setupJoystick();
  //setupEncoder();
  Mouse.begin();
  pmwSensor.begin(
      SS_PIN,
      CPI_LEVELS[currentCPI]
      ); // to set CPI (Count per Inch), pass it as the second parameter
}

void loop() {
  currentTime = millis();
  readPMWSensor();
  readButtons();
  readJoystick();
  //updateScroll();
}
