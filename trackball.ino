#include "PMW3360.h"
//#include <Mouse.h>
//#include <Keyboard.h>
#include "HID-Project.h"

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
constexpr static uint8_t B1_PIN = 3;   // Switch these as needed - e.g. misoddering
constexpr static uint8_t B2_PIN = 2;
constexpr static uint8_t B3_PIN = 21;  // JOYSTICK BTN
constexpr static uint8_t B4_PIN = 4;
constexpr static uint8_t JOY_X = 20;
constexpr static uint8_t JOY_Y = 19;

// Trackball settings
constexpr static uint16_t TIME_TO_BUTTON = 10;  // ms

PMW3360_DATA data;

// ATTENTION: Right now there is no overflow guard. It will overflow in around
// 50 days. Who ever has this turned on for that long? If you do, add an
// overflow guard in the main loop
static unsigned long currentTime = 0;
static unsigned long prevElapsedMillisJoystickX = 0;
static unsigned long prevElapsedMillisJoystickY = 0;
static unsigned long prevElapsedMillisButton = 0;
static uint8_t currentCPI = 0;

struct ButtonData {
  uint8_t pin;
  uint8_t mButton;
};

constexpr static uint8_t CPI_LEVEL_SIZE = 4;
constexpr static unsigned int CPI_LEVELS[CPI_LEVEL_SIZE] = { 300, 500, 800, 1200 };
constexpr static uint8_t NUM_BUTTONS = 4;
#define NOT_USED 99
constexpr static ButtonData buttons[NUM_BUTTONS] = {
  { B1_PIN, MOUSE_LEFT }, { B2_PIN, MOUSE_RIGHT }, { B4_PIN, MOUSE_MIDDLE }, { B3_PIN, NOT_USED }
};
constexpr static uint16_t CURSOR_DEADZONE = 10;

uint8_t previousButtonVals[NUM_BUTTONS] = { false };

uint8_t previousJoystickVals[2] = { false };

/* BUTTONS FUNCTIONS */
void setupButtons() {
  pinMode(B1_PIN, INPUT_PULLUP);
  pinMode(B2_PIN, INPUT_PULLUP);
  pinMode(B3_PIN, INPUT_PULLUP);
  pinMode(B4_PIN, INPUT_PULLUP);
}

void setupJoystick() {
  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);
}

void readButtons() {
  if (currentTime - prevElapsedMillisButton < TIME_TO_BUTTON) {
    return;
  }
  prevElapsedMillisButton = currentTime;

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
    currentCPI = (currentCPI + 1) % CPI_LEVEL_SIZE;
    pmwSensor.setCPI(CPI_LEVELS[currentCPI]);
    previousButtonVals[cpiButtonId] = false;
  } else if (!bVal) {
    previousButtonVals[cpiButtonId] = true;
  }
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

    if( data.dy != 0 || data.dx != 0)
    {
      Mouse.move(data.dy, data.dx, 0);
    }
  }
}

void readJoystick() {

  constexpr int SPEEDS_SIZE = 5;
  constexpr int SCROLL_SPEEDS[SPEEDS_SIZE] = { 1, 2, 3, 4, 5 };

  constexpr int CENTER = 512;
  constexpr int MAX = CENTER * 2;
  constexpr int DEADZONE = 55;
  constexpr int DEADZONE_X = 25;
  constexpr int ZONE_SIZE = (CENTER) / SPEEDS_SIZE;
  constexpr int ZONE_DIVISION = 128;

  constexpr int MAX_TIME = 300;
  constexpr int MIN_TIME = 25;
  //uint8_t speedStage[2] = {0};

  //The joystick is sideways so X is actually "y"
  int y = analogRead(JOY_Y);

  int deltaY = y - CENTER;
  if (abs(deltaY) > DEADZONE) {

    int speedDelay = map(abs(deltaY), DEADZONE, CENTER, MAX_TIME, MIN_TIME);
    if (currentTime - prevElapsedMillisJoystickY >= (unsigned long)speedDelay) {
      if (deltaY > 0) {
        Mouse.move(0, 0, -1);
      } 
      else {
        Mouse.move(0, 0, 1);
      }
      prevElapsedMillisJoystickY = currentTime;  // Reset the interval timer
    }
  }
  if (currentTime - prevElapsedMillisJoystickX >= TIME_TO_BUTTON) {
    prevElapsedMillisJoystickX = currentTime;
    int x = analogRead(JOY_X);
    bool leftPressed = false;
    bool rightPressed = false;
    if (x < DEADZONE_X) {
      leftPressed = true;
      Mouse.press(MOUSE_PREV);
    } 
    else if (x > MAX - DEADZONE_X) {
      rightPressed = true;
      Mouse.press(MOUSE_NEXT);
    }

    if (previousJoystickVals[0] && !leftPressed) {
      Mouse.release(MOUSE_PREV);
    } 
    else if (previousJoystickVals[1] && !rightPressed) {
      Mouse.release(MOUSE_NEXT);
    }
    previousJoystickVals[0] = leftPressed;
    previousJoystickVals[1] = rightPressed;
  }
}

void setup() {
  setupButtons();
  setupJoystick();
  Mouse.begin();
  Keyboard.begin();
  pmwSensor.begin(
    SS_PIN,
    CPI_LEVELS[currentCPI]);  // to set CPI (Count per Inch), pass it as the second parameter
}

void loop() {
  currentTime = millis();
  readPMWSensor();
  readButtons();
  readJoystick();
}
