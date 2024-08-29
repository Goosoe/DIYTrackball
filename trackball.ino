#include <Mouse.h>
#include "PMW3360.h"

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
constexpr uint8_t SS_PIN = 18;
constexpr uint16_t CPI = 1000;
constexpr uint8_t B1_PIN = 2;
constexpr uint8_t B2_PIN = 3;
constexpr uint8_t B3_PIN = 21;
constexpr uint8_t ENCODER_A= 7;
constexpr uint8_t ENCODER_B = 8;

PMW3360_DATA data;

uint8_t encoderAVal;
uint8_t encoderPrevAVal;

struct ButtonData
{
    uint8_t pin;
    uint8_t mButton;
};

constexpr uint8_t NUM_BUTTONS = 3;
constexpr ButtonData buttons[NUM_BUTTONS] = {{B1_PIN, MOUSE_LEFT}, {B2_PIN, MOUSE_RIGHT}, {B3_PIN, MOUSE_MIDDLE}};
uint8_t previousButtonVals[NUM_BUTTONS] = {false};

/* BUTTONS FUNCTIONS */
void setupButtons()
{
    pinMode(B1_PIN, INPUT_PULLUP);
    pinMode(B2_PIN, INPUT_PULLUP);
    pinMode(B3_PIN, INPUT_PULLUP);
}

void readButtons()
{
    for(int i = 0; i < NUM_BUTTONS; i++)
    {
        uint8_t bVal = digitalRead(buttons[i].pin);
        if(bVal && previousButtonVals[i])
        {
            Mouse.release(buttons[i].mButton);
            previousButtonVals[i] = false;
        }
        else if(!bVal)
        {
            Mouse.press(buttons[i].mButton);
            previousButtonVals[i] = true;
        }
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
  - PMW3360_DATA.rawDataSum    : byte, It reports the upper byte of an 18‐bit counter 
                               which sums all 1296 raw data in the current frame;
                               * Avg value = Raw_Data_Sum * 1024 / 1296
  - PMW3360_DATA.maxRawData    : byte, Max/Min raw data value in current frame, max=127
    PMW3360_DATA.minRawData
  - PMW3360_DATA.shutter       : unsigned int, shutter is adjusted to keep the average
                               raw data values within normal operating ranges.
*/
void readPMWSensor()
{
	pmwSensor.readBurst(data);
    if(data.isOnSurface && data.isMotion)
	{
        Mouse.move(data.dx, -data.dy, 0);
	}
}


/* ENCODER FUNCTIONS */
void setupEncoder()
{
    pinMode(ENCODER_A, INPUT_PULLUP);
    pinMode(ENCODER_B, INPUT_PULLUP);
    encoderPrevAVal = digitalRead(ENCODER_A);
}

void readEncoder()
{
    encoderAVal = digitalRead(ENCODER_A);
    if(encoderAVal != encoderPrevAVal)
    {
        if(digitalRead(ENCODER_B) != encoderAVal)
        {
            Mouse.move(10, 0, 1);
        }
        else 
        {
            Mouse.move(-10, 0, 1);
        }
        encoderPrevAVal = encoderAVal;
    }
}
/**/

void setup() 
{
    setupButtons();
    setupEncoder();
    Mouse.begin();
    pmwSensor.begin(SS_PIN, CPI); // to set CPI (Count per Inch), pass it as the second parameter
}

void loop() {
    readPMWSensor();
    readButtons();
    readEncoder();
}
