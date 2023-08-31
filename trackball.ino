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

PMW3360 sensor;

// Slave Select pin. Connect this to SS on the module.
constexpr uint8_t selectionPin = 18;
constexpr uint8_t button1 = 2;
constexpr uint8_t button2 = 3;
constexpr uint8_t NUM_BUTTONS = 2;
PMW3360_DATA data;

struct ButtonData
{
    uint8_t pin;
    uint8_t mButton;
    bool previousInVal;
};

ButtonData buttons[NUM_BUTTONS] = {{button1, MOUSE_LEFT, false}, {button2, MOUSE_RIGHT, false}};

void readButtons()
{
    for(int i = 0; i < NUM_BUTTONS; i++)
    {
        uint8_t bVal = digitalRead(buttons[i].pin);
        if(bVal && buttons[i].previousInVal)
        {
            Mouse.release(buttons[i].mButton);
            buttons[i].previousInVal = false;
        }
        else if(!bVal)
        {
            Mouse.press(buttons[i].mButton);
            buttons[i].previousInVal = true;
        }
    }
} 

void readSensor()
{
	sensor.readBurst(data);
    if(data.isOnSurface && data.isMotion)
	{
        Serial.print(data.dx);
        Serial.print("\t");
        Serial.print(data.dy);
        Serial.println();
        Mouse.move(data.dx, -data.dy, 0);
	}
}

void setup() 
{
	Serial.begin(9600);
    pinMode(button1, INPUT_PULLUP);
    pinMode(button2, INPUT_PULLUP);
  	Mouse.begin();

  	sensor.begin(selectionPin);
  //sensor.begin(10, 1600); // to set CPI (Count per Inch), pass it as the second parameter
}

void loop() {
    readSensor();
    readButtons();
	delay(10);
}
