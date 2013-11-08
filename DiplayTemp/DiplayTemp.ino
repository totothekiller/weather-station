//
// Display Temp
//

// Use Library from :
// Adafruit BMP085 https://github.com/adafruit/Adafruit_BMP085_Unified
// Adafruit Sensor https://github.com/adafruit/Adafruit_Sensor
// Finite State Machine http://arduino-info.wikispaces.com/HAL-LibrariesUpdates
// LCD : https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
// Touch Sensor : https://github.com/arduino-libraries/CapacitiveSensor

#include <VirtualWire.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <CapacitiveSensor.h>
#include <FiniteStateMachine.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085.h>

// LCD I2C Adress
#define I2C_ADDR 0x27

// LCD Static Config
#define BACKLIGHT_PIN 3
#define En_pin 2
#define Rw_pin 1
#define Rs_pin 0
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7

#define BACKLIGHT_DURATION 10000

// Pin Configuration
#define rxLed 13
#define rxPin 11
#define touchCommonPin 2
#define touchSensPin 4

// Touch sensibility
#define TOUCH_SENSIBILITY 100

// Main sensor update
#define MAIN_SENSOR_UPDATE 60000

// LCD
LiquidCrystal_I2C _lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

// Touched
CapacitiveSensor _touch = CapacitiveSensor(touchCommonPin,touchSensPin); 

// Athmospheric Sensor 
Adafruit_BMP085 _bmp = Adafruit_BMP085(1234);

// Sensor RX Data
typedef struct sensorData_t{
  byte id; // size 1
  float value; // size 4
};

typedef union sensorData_union_t{
  sensorData_t data;
  uint8_t raw[5]; // total size of 5 bytes
};

// Main Sensor Definition
typedef struct mainSensorDef_t {
  float pressure;
  float temp;
};

// Remote Sensor Defintion
typedef struct remoteSensorDef_t {
  char * name;
  uint8_t id;
  float temp;
};

// RX message
uint8_t _message[VW_MAX_MESSAGE_LEN];

// LCD State
State _lcdHome = State(updateSensor);

// Remote Sensors
remoteSensorDef_t * _remoteSensors;

// Last Touched
unsigned long _lastTouch;

void setup() {
  // Led Setup
  pinMode(rxLed, OUTPUT);

  // Debug
  Serial.begin(9600);
  Serial.println("--- TTK Weather Station ---");

  // Set Up RX
  vw_set_rx_pin(rxPin);
  vw_setup(2000);

  // Set Up LCD
  _lcd.begin (16,2); // 16x2 LCD
  _lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  _lcd.setBacklight(HIGH);
  _lcd.home();
  _lcd.print("--- Welcome ---");

  // Start the receiver
  vw_rx_start();

  // Initialise 
  if(!_bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print("Ooops, no BMP085 detected ...");
  }

  // Remote sensore Static Def
  _remoteSensors = new remoteSensorDef_t[2];

  _remoteSensors[0].name="Sonde 1";
  _remoteSensors[0].id=42;

  _remoteSensors[0].name="Sonde 2";
  _remoteSensors[0].id=69;
}

void loop() {

  // Turn the ligth ?
  long touched =  _touch.capacitiveSensor(30);

  unsigned long currentTime = millis();

  if(touched>=TOUCH_SENSIBILITY)
  {
    Serial.print("Touch ");
    Serial.println(touched);   
    _lcd.setBacklight(HIGH); // Backlight on
    _lastTouch = currentTime;
  }
  else if( currentTime - _lastTouch > BACKLIGHT_DURATION)
  {
    _lcd.setBacklight(LOW); // Backlight off
  }


  updateSensor();

  // Wait some time
  delay(20);
}


/*
 * Update all sensors
 *
 */
void  updateSensor()
{
  // Message Received ?
  if( vw_have_message())
  {
    // max message size
    uint8_t messageLen = VW_MAX_MESSAGE_LEN;

    if (vw_get_message(_message, &messageLen)) // Read message
    {
      digitalWrite(rxLed, HIGH);

      // DEBUG
      if(Serial)
      {
        Serial.print("Got: ");

        // HEX DUMP
        for (int i = 0; i < messageLen; i++)
        {
          Serial.print(_message[i], HEX);
        }
        Serial.println("");
      }

      // Check size
      if(messageLen>4)
      {
        // Convert Byte to Sensor Data
        sensorData_union_t sensor;

        for (int k=0; k < 5; k++)
        { 
          sensor.raw[k] = _message[k];
        }

        Serial.print("Sensor=");
        Serial.print(sensor.data.id);
        Serial.print(", Temp=");
        Serial.println(sensor.data.value);

        // LCD
        _lcd.setCursor (0,1); // go to start of 2nd line
        _lcd.print(sensor.data.value);
      } 
      else
      {
        Serial.println("Not enought data in transmition");

        // Try to restart RX module
        vw_rx_stop(); 
        delay(1000);
        vw_rx_start();      
      }
      digitalWrite(rxLed, LOW);
    }
  }


}





