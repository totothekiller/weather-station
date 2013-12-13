//
// Display Temp
//

// Use Library from :
// Adafruit BMP085 https://github.com/adafruit/Adafruit_BMP085_Unified
// Adafruit Sensor https://github.com/adafruit/Adafruit_Sensor
// LCD : https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
// Touch Sensor : https://github.com/arduino-libraries/CapacitiveSensor
// Ethercard : https://github.com/jcw/ethercard

#include <VirtualWire.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <CapacitiveSensor.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085.h>
#include <EtherCard.h>

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

#define BACKLIGHT_DURATION 20000

// Pin Configuration
#define rxLed 6
#define rxPin 7
#define touchCommonPin 2
#define touchSensPin 4

// Touch sensibility
#define TOUCH_SENSIBILITY 100

// Contact De-Bouncing
#define TOUCH_DEBOUNCING 250

// Main sensor update
#define MAIN_SENSOR_UPDATE 60000

// Touched
CapacitiveSensor _touch = CapacitiveSensor(touchCommonPin,touchSensPin); 

// Athmospheric Sensor 
Adafruit_BMP085 _mainSensor = Adafruit_BMP085();

// MAC Address
static byte _mymac[] = {0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };

// remote website ip address and port
static byte _ttkserver[] = { 192, 168, 0, 1 };

// Arduino IP
static byte _arduinoIP[] = { 192, 168, 0, 42 };

// gateway ip address
static byte _gwip[] = { 192, 168, 0, 254 };

// Main Ethernet Buffer
byte Ethernet::buffer[300];

static BufferFiller _etherBuffFiller;  // used as cursor while filling the buffer

// Page to display to user
char _indexHTML[] PROGMEM =
  "HTTP/1.0 200 OK\r\n"
  "Content-Type: text/html\r\n"
  "\r\n"
  "<html>"
    "<head><title>"
      "--- TTK Weather Station ---"
    "</title></head>"
    "<body>"
      "<h3>I am the Weather Station</h3>"
      "<p>Up Time $D ms</p>"
    "</body>"
  "</html>"
;

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
  byte  currentSensor; // Current sensor to display
  byte tick; // Used for transition
  bool paint; // paint is in progress ?
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
  byte nbrSensors;
  SensorDefinition sensors[3]; // Sensors Definition
  RenderDefinition render; // LCD rendering
  ReceiverDefinition rx; // RX receiver
  unsigned long lastTouched; // Last Touched time
  unsigned long lastMainSensorUpdated; // Last time that the main sensor was updated
  char url[20];
  volatile boolean isHttpRequest; // true during HTTP request
} ApplicationDefinition;

// Static Definition
ApplicationDefinition _app = {
  3, // Total number of sensors
  // Sensors
  {
    // Sensor #1
    {
      "Inside Temp",
      1, // Sensor ID
      -99.0, // Initial Value
      "C"
    },
    // Sensor #2
    {
      "Outside Temp",
      42, // Sensor ID
      -99.0, // Initial Value
      "C"
    },
    // Sensor #3
    {
      "Pressure",
      2, // Sensor ID
      -99.0, // Initial Value
      "hPa"
    }
  },
    // Render
  {
    LiquidCrystal_I2C(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin),
    0,
    0,
    true // force paint
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
  Serial.begin(57600);
  Serial.println(F("--- TTK Weather Station ---"));
  
  displayFreeRam();
  
  // Set Up RX
  vw_set_rx_pin(rxPin);
  vw_setup(2000);

  // Set Up LCD
  _app.render.lcd.begin (16,2); // 16x2 LCD
  _app.render.lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  _app.render.lcd.setBacklight(HIGH);
  _app.render.lcd.clear();
  _app.render.lcd.print(F("--- Welcome ---"));

  // Start the receiver
  vw_rx_start();

  // Initialise 
  if(!_mainSensor.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print(F("Ooops, no BMP085 detected ..."));
  }

  // 
  // Setup Ethernet
  if (ether.begin(sizeof Ethernet::buffer, _mymac) == 0)
  {
    Serial.println(F("Failed to access Ethernet controller"));
  }
  
  displayFreeRam();
  
  Serial.println(F("Setting up static IP"));

  // Static IP
  ether.staticSetup(_arduinoIP,_gwip);

  ether.printIp(F("My IP: "), ether.myip);
  ether.printIp(F("Netmask: "), ether.mymask);
  ether.printIp(F("GW IP: "), ether.gwip);
  ether.printIp(F("DNS IP: "), ether.dnsip);
  
  // Set Up destination IP
  ether.copyIp(ether.hisip, _ttkserver);
  ether.printIp(F("Destination Server: "), ether.hisip);
  
  // Wait Gatway
  Serial.print(F("Gateway ..."));
  while (ether.clientWaitingGw())
  {
    ether.packetLoop(ether.packetReceive());
  }
  Serial.println("found !");
  
  // Register Ping Callback
  ether.registerPingCallback(pingCallback);
  
  // No Request Yet
  _app.isHttpRequest = false;
}

void loop() {

  // Detech user touch
  detectTouch();

  // Check RX
  checkIncomingData();
  
  // Check Ethernet
  checkEthernet();

  // Update Local Sensor
  updateLocalSensors();

  // Paint
  render();

  // Wait some time
  //delay(20);

}


//
// Touched Detection
//
void detectTouch()
{
  // Turn the ligth ?
  long touched =  _touch.capacitiveSensor(30);

  unsigned long currentTime = millis();

  if(touched>=TOUCH_SENSIBILITY && currentTime - _app.lastTouched > TOUCH_DEBOUNCING)
  {
    Serial.print(F("!! Touched !! with sensibility of "));Serial.println(touched);   

    _app.lastTouched = currentTime;

    // Handle Event
    onTouchEvent();
  }
  else if( currentTime - _app.lastTouched > BACKLIGHT_DURATION)
  {
    // Backlight off
    _app.render.lcd.setBacklight(LOW); 
  }
}


//
// Handle touch Event
//
void onTouchEvent()
{
   // Backlight on
    _app.render.lcd.setBacklight(HIGH);

    // Display next sensor
    selectNextSensor();
}


//
// LCD Render function
//
void render()
{
  // TODO annimation ?

  // Paint ?
  if(_app.render.paint)
  {
    Serial.print(F("Repaint sensor #"));Serial.println(_app.render.currentSensor);

    _app.render.lcd.clear();
    _app.render.lcd.print(_app.sensors[_app.render.currentSensor].name);
    _app.render.lcd.setCursor (0,1); // go to start of 2nd line
    _app.render.lcd.print(_app.sensors[_app.render.currentSensor].value);
    _app.render.lcd.print(' ');
    _app.render.lcd.print(_app.sensors[_app.render.currentSensor].unit);

    // End of Paint
   _app.render.paint = false;
  }


  // Increment
  _app.render.tick++;
}

 
//
// Diplay the Next Sensor
//
void selectNextSensor()
{
  // Change current sensor
  _app.render.currentSensor = (_app.render.currentSensor + 1) % _app.nbrSensors;

  Serial.print(F("Select Next Sensor #"));Serial.println(_app.render.currentSensor);

  // Force Repaint
  _app.render.paint = true;
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
        Serial.print(F("RX incomming data : "));

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
        Serial.println(F("Not enought data in transmition"));

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
  Serial.print(F("Sensor ID = "));Serial.print(sensorID);
  Serial.print(F(", new value = "));Serial.println(newValue);

  // Find in the sensor the SensorDefinition
  for (int i = 0; i < (_app.nbrSensors); ++i)
  {
    if(_app.sensors[i].id==sensorID)
    {
      Serial.print(F(" -> sensor is #"));Serial.println(i);

      // Update value
      _app.sensors[i].value = newValue;

      // Repaint if we display the current value
      if(i==_app.render.currentSensor)
      {
        _app.render.paint = true;
        Serial.println(F(" -> Repaint is requested"));
      }

      break; // Exit loop
    }
  }
  
  //
  // Send value to Web Server
  
  // Convert to String
  sprintf(_app.url, "%d/", sensorID);

  // Convert Float to String
  char newValueString[10];
  dtostrf(newValue, 2, 2, newValueString);
  
  strcat(_app.url,newValueString);
  
  Serial.print(F("Full path = "));Serial.println(_app.url);
  
  
  _app.isHttpRequest = true;
  
   ether.browseUrl(PSTR("/cakephp/points/add/"), _app.url, PSTR("TTKSERVER"), requestUrlCallback);
  
 
  //
  // Wait end of HTTP Request
  while(_app.isHttpRequest)
  {
      word len = ether.packetReceive();
      word pos = ether.packetLoop(len);
      //checkEthernet();
  }
  
  Serial.println(F("End Of Request"));
  
  // Workaround to clean Ethernet ...
  for(int i=0;i<100;i++)
  {
    word len = ether.packetReceive();
    word pos = ether.packetLoop(len);
  }
}


//
// Update Local sensors
//
static void updateLocalSensors()
{
  // Get time
  unsigned long currentTime = millis();

  // Check if we have to update the main sensor
  if(currentTime - _app.lastMainSensorUpdated > MAIN_SENSOR_UPDATE)
  {
    Serial.println(F("Update Local Sensor"));

    _app.lastMainSensorUpdated = currentTime;

    float buffer;

    // Read Temp sensor
    _mainSensor.getTemperature(&buffer);

    // Notify new Value
    fireNewSensorValue(1,buffer);

    // Read Pressure sensor (in Pa)
    _mainSensor.getPressure(&buffer);

    // Notify new Value (in hPa)
    fireNewSensorValue(2,buffer/100);
  }
}


//
// Check Ethernet bus
//
void checkEthernet()
{
  // check if anything has come in via ethernet
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  
  // check if valid tcp data is received
  if (pos) {
    
    // Init Buffer
    _etherBuffFiller = ether.tcpOffset();

    char* data = (char *) Ethernet::buffer + pos;
    Serial.println(F("Ethernet data received :"));
    Serial.println(data);
       
    // Populate Page to Buffer
    _etherBuffFiller.emit_p(_indexHTML, millis());
    
    ether.httpServerReply(_etherBuffFiller.position()); // send web page data
  }
}


//
// Ping Callback
//
static void pingCallback (byte* ptr) {
  ether.printIp(">>> ping from: ", ptr);
}

// 
// End HTTP GET request
//
// called when the client request is complete
static void requestUrlCallback (byte status, word off, word len) {
  Serial.println("<<< reply ");
  Serial.println((const char*) Ethernet::buffer + off);
  
  _app.isHttpRequest = false; // Turn off HTTP request flag
}


static int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void displayFreeRam()
{
  Serial.print(F("Free Ram ="));Serial.println(freeRam());
}
  
