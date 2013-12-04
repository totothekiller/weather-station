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
//LiquidCrystal_I2C _lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

// Touched
CapacitiveSensor _touch = CapacitiveSensor(touchCommonPin,touchSensPin); 

// Athmospheric Sensor 
Adafruit_BMP085 _mainSensor = Adafruit_BMP085(0);

// Sensor Defintion
typedef struct SensorStruct {
  char * name; 
  uint8_t id; // Sensor ID
  float value; 
  char * unit; // Unit
} SensorDefinition;

// Rendering
typedef struct RenderStruct {
  LiquidCrystal_I2C lcd; // LCD
  SensorDefinition * sensor; // Sensor to display
  byte tick; // Used for transition
} RenderDefinition;

// Sensor RX Data
typedef struct RxSensorDataStruct {
  byte id; // size 1
  float value; // size 4
} RxSensorDataDefinition;

typedef union RxSensorRawDataStruc{
  RxSensorDataDefinition decoded;
  uint8_t raw[5]; // total size of 5 bytes
} RxSensorRawDataDefinition;

// RX Message
typedef struct ReceiverStruct {
  uint8_t stream[VW_MAX_MESSAGE_LEN]; // Raw Rx Stream
  RxSensorRawDataDefinition value; // Decoded Value
} ReceiverDefinition;

// Main Application Structure
typedef struct ApplicationStruct {
  byte nbrSensor;
  SensorDefinition sensors[2]; // Sensors Definition
  RenderDefinition render; // LCD rendering
  ReceiverDefinition rx; // RX receiver
  unsigned long lastTouched; // Last Touched time
} ApplicationDefinition;

// Static Definition
ApplicationDefinition _app = {
  2, // Total number of sensors
  // Sensors
  {
    // Sensor #1
    {
      "Inside Temp",
      0, // Sensor ID
      -99.0, // Initial Value
      "°C"
    },
    // Sensor #2
    {
      "Outside Temp",
      42, // Sensor ID
      -99.0, // Initial Value
      "°C"
    }
  },
    // Render
  {
    LiquidCrystal_I2C(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin)
    // Init other field with 0
  },
  
  // RX Reveiver
  {
    // Init array with 0
  },
  0 // last touched
};


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
  _app.render.lcd.begin (16,2); // 16x2 LCD
  _app.render.lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  _app.render.lcd.setBacklight(HIGH);
  _app.render.lcd.home();
  _app.render.lcd.print("--- Welcome ---");

  // Start the receiver
  vw_rx_start();

  // Initialise 
  if(!_mainSensor.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print("Ooops, no BMP085 detected ...");
  }
}

void loop() {

  // Turn the ligth ?
  long touched =  _touch.capacitiveSensor(30);

  unsigned long currentTime = millis();

  if(touched>=TOUCH_SENSIBILITY)
  {
    Serial.print("Touch ");
    Serial.println(touched);   
    _app.render.lcd.setBacklight(HIGH); // Backlight on
    _app.lastTouched = currentTime;
  }
  else if( currentTime - _app.lastTouched > BACKLIGHT_DURATION)
  {
    _app.render.lcd.setBacklight(LOW); // Backlight off
  }

  // Check RX
  checkIncomingData();

  // Paint
  render();

  // Wait some time
  delay(20);
}



//
// LCD Render function
//
void render()
{
  // TODO annimation ?


  // Display Sensor
  if(_app.render.sensor!=NULL)
  {
    _app.render.lcd.home();
    _app.render.lcd.print(_app.render.sensor->name);
    _app.render.lcd.setCursor (0,1); // go to start of 2nd line
    _app.render.lcd.print(_app.render.sensor->value);
    _app.render.lcd.print(' ');
    _app.render.lcd.print(_app.render.sensor->unit);


    _app.render.sensor = NULL;
  }

}



//
// Diplay the Next Sensor
//
void selectNextSensor()
{

}


//
// Check if we have a unread message in the air
//
void checkIncomingData()
{
  // Message Received ?
  if( vw_have_message())
  {
    // max message size
    uint8_t messageLen = VW_MAX_MESSAGE_LEN;

    if (vw_get_message(_app.rx.stream, &messageLen)) // Read message
    {
      digitalWrite(rxLed, HIGH);

      // DEBUG
      if(Serial)
      {
        Serial.print("Got: ");

        // HEX DUMP
        for (int i = 0; i < messageLen; i++)
        {
          Serial.print(_app.rx.stream[i], HEX);
        }
        Serial.println("");
      }

      // Check size
      if(messageLen>4)
      {
        // Convert Byte to Sensor Data
        //RxSensorRawDataDefinition sensor;

        for (int k=0; k < 5; k++)
        { 
          _app.rx.value.raw[k] = _app.rx.stream[k];
        }
        
        // Fire this new value        
        fireNewSensorValue(_app.rx.value.decoded.id, _app.rx.value.decoded.value);
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


void fireNewSensorValue(byte sensorID, float newValue)
{
  Serial.print("Sensor=");
  Serial.print(_app.rx.value.decoded.id);
  Serial.print(", Value=");
  Serial.println(_app.rx.value.decoded.value);


  // Find in the sensor the SensorDefinition
  for (int i = 0; i < (_app.nbrSensor); ++i)
  {
    if(_app.sensors[i].id==sensorID)
    {
      // Update value
      _app.sensors[i].value = newValue;

      // Render
      _app.render.sensor = &_app.sensors[i];

      // TODO Ethernet

      break; // Exit loop
    }
  }
}


//
// Update Local sensors
//
void updateLocalSensors()
{

  // Read Pression sensor

  // Read Temp sensor

}

