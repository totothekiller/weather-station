//
// Display Temp
//

// Use Library from :
// Adafruit BMP085 https://github.com/adafruit/Adafruit_BMP085_Unified
// Adafruit Sensor https://github.com/adafruit/Adafruit_Sensor
// LCD : https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
// Touch Sensor : https://github.com/arduino-libraries/CapacitiveSensor
// Ethercard : https://github.com/jcw/ethercard

#include <Manchester.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <CapacitiveSensor.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085.h>
#include <EtherCard.h>
#include <avr/wdt.h>

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
#define MAIN_SENSOR_UPDATE 240000

// Timeout for HTTP Request
#define HTTP_TIMEOUT 2000

// Touched
CapacitiveSensor _touch = CapacitiveSensor(touchCommonPin,touchSensPin); 

// Athmospheric Sensor 
Adafruit_BMP085 _mainSensor = Adafruit_BMP085();

// MAC Address
static const byte _mymac[] = {0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };

// remote website ip address and port
static const char _website[] PROGMEM = "http://www.example.com/";

// Remote URL
static const char _baseURL[] PROGMEM = "/riot-server/points/add/";

// Main Ethernet Buffer
byte Ethernet::buffer[600];

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
  volatile bool paint; // paint is in progress ?
} RenderDefinition;

// Sensor RX Data
typedef struct RxSensorDataStruct {
  uint8_t id; // size 1
  float value; // size 4
  uint8_t checksum; // size 1
} RxSensorDataDefinition;

typedef union RxSensorRawDataStruc{
  RxSensorDataDefinition decoded;
  uint8_t raw[6]; // total size of 6 bytes
} RxSensorRawDataDefinition;

// Ethernet Structure
typedef struct EthernetStruct {
  BufferFiller buffer;  // used as cursor while filling the buffer
  char url[15]; // HTTP dynamic URL
  volatile boolean isHttpRequest; // true during HTTP request
} EthernetDefinition;

// Main Application Structure
typedef struct ApplicationStruct {
  byte nbrSensors;
  SensorDefinition sensors[4]; // Sensors Definition
  RenderDefinition render; // LCD rendering
  RxSensorRawDataDefinition rx; // RX receiver
  EthernetDefinition eth; // Ethernet
  unsigned long lastTouched; // Last Touched time
  unsigned long lastMainSensorUpdated; // Last time that the main sensor was updated
} ApplicationDefinition;

// Static Definition
static ApplicationDefinition _app = {
  4, // Total number of sensors
  // Sensors
  {
    // Sensor #0
    {
      "Inside Temp",
      1, // Sensor ID
      -99.0, // Initial Value
      "C"
    },
    // Sensor #1
    {
      "Outside Temp",
      42, // Sensor ID
      -99.0, // Initial Value
      "C"
    },
    // Sensor #2
    {
      "Pressure",
      2, // Sensor ID
      -99.0, // Initial Value
      "hPa"
    },
    // Sensor #3
    {
      "Power",
      5, // Sensor ID
      -99.0, // Initial Value
      "W"
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
  // omit other fields -> init to 0
};


void setup() {
  //
  // First Activate Watchdog
  wdt_enable(WDTO_8S); // 8 sec
  
  // Led Setup
  pinMode(rxLed, OUTPUT);

  // Debug
  Serial.begin(57600);
  Serial.println(F("--- TTK Weather Station ---"));
  
  // Set Up LCD
  _app.render.lcd.begin (16,2); // 16x2 LCD
  _app.render.lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  _app.render.lcd.setBacklight(HIGH);
  _app.render.lcd.clear();
  _app.render.lcd.print(F("--- Welcome ---"));
  _app.render.lcd.setCursor(0,1);
  _app.render.lcd.print(F("waiting IP..."));

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
  
  //
  // Stop WatchDog during DHCP
  wdt_disable();
  
  Serial.println(F("Setting up Dynamic IP"));

  if (!ether.dhcpSetup())
  {
    Serial.println(F("DHCP failed"));
  }
  
  ether.printIp(F("IP:  "), ether.myip);
  ether.printIp(F("GW:  "), ether.gwip);  
  ether.printIp(F("DNS: "), ether.dnsip);  

  if (!ether.dnsLookup(_website))
  {
    Serial.println(F("DNS failed"));
  }
  
  ether.printIp(F("SRV: "), ether.hisip);
  
  // No Request Yet
  _app.eth.isHttpRequest = false;
  
  //
  // Re-Activate Watchdog
  wdt_enable(WDTO_8S); // 8 sec
  
  // Set Up RX
  man.setupReceive(rxPin);
  
  // Start Receiving
  man.beginReceiveArray(6, _app.rx.raw);
  
  // Display Free RAM
  displayFreeRam();
}

void loop() {
  
  // Restart Watchdog
  wdt_reset();
  
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
}


//
// Touched Detection
//
void detectTouch()
{
  // Turn the ligth ?
  long touched =  _touch.capacitiveSensor(30);

  unsigned long currentTime = millis();

  if(touched >= TOUCH_SENSIBILITY && currentTime - _app.lastTouched > TOUCH_DEBOUNCING)
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
  _app.render.tick++; // useless
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
  if(man.receiveComplete())
  {
    digitalWrite(rxLed, HIGH);
  
    // DEBUG
    if(Serial)
    {
      Serial.print(F("RX incomming data : "));
  
      // HEX DUMP
      for (int i = 0; i < 6; i++)
      {
        Serial.print(_app.rx.raw[i], HEX);
      }
      Serial.println("");
    }
  
    // Checksum Validation
    uint8_t checksum = crc8(_app.rx.raw,6);
    
    if(checksum==0)
    {
      // Fire this new value        
      fireNewSensorValue(_app.rx.decoded.id, _app.rx.decoded.value);
    } 
    else
    {
     // DEBUG
      if(Serial)
      {
        Serial.print(F("Bad CheckSum :"));
        Serial.println(checksum, HEX);
      }
  
      // Try to restart RX module
      man.stopReceive();
      delay(1000);
      man.setupReceive(rxPin);   
    }
    
    // Prepare Next Receiving
    man.beginReceiveArray(6, _app.rx.raw);
    
    digitalWrite(rxLed, LOW);
   }
   else
   {
    // If no message found
   
    // After some time
    // reset then restart reveiving 
    
    //Serial.print(F("RF State = "));Serial.println(rx_mode);
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
  sendDataToWebServer(sensorID, newValue);
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

    // Read Temp sensor
    float temp = _mainSensor.readTemperature();

    // Notify new Value
    fireNewSensorValue(1, temp);

    // Read Pressure sensor (in Pa)
    float pressure = (float) _mainSensor.readPressure();

    // Notify new Value (in hPa)
    fireNewSensorValue(2, pressure/100.0);
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
}


//
// Send Data to WebServer : sensorID/newValue
//
void sendDataToWebServer(byte sensorID, float newValue)
{
  // Convert to String
  sprintf(_app.eth.url, "%d/", sensorID);

  // Convert Float to String
  char newValueString[10];
  dtostrf(newValue, 2, 2, newValueString);
  
  // Concat
  strcat(_app.eth.url,newValueString);
  
  Serial.print(F(">>> HTTP Request : "));Serial.println(_app.eth.url);
  
  // Init Request
  _app.eth.isHttpRequest = true;
  const unsigned long dateHTTPrequest = millis();
  
  // Send Request  
  ether.browseUrl(_baseURL, _app.eth.url, _website, requestUrlCallback);  
  
  //
  // Wait end of HTTP Request
  while(millis() - dateHTTPrequest < HTTP_TIMEOUT && ( _app.eth.isHttpRequest || ether.clientWaitingGw() ))
  {
    ether.packetLoop(ether.packetReceive());
  }
   
  // Verify
  if(_app.eth.isHttpRequest)
  {
    Serial.println(F("Fail Of HTTP Request !"));
     
    // Reset Flag
    _app.eth.isHttpRequest = false;
  }
  
  // Workaround to clean Ethernet ...
  for(int i=0;i<100;i++)
  {
    ether.packetLoop(ether.packetReceive());
  }
}

// 
// End HTTP GET request Callback
//
static void requestUrlCallback (byte status, word off, word len) {
  Serial.println(F("<<< RESPONSE :"));
  
  // Set end of String (for correct display in println)
  Ethernet::buffer[min(599, off + len)] = 0;
  
  // Display response
  Serial.println((const char*) Ethernet::buffer + off);
  
  // Turn off HTTP request flag
  _app.eth.isHttpRequest = false; 
}


//
// Get the Free RAM
static int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

//
// Display to Serial the available RAM memory
void displayFreeRam()
{
  Serial.print(F("Free Ram = "));Serial.println(freeRam());
}

// This table comes from Dallas sample code where it is freely reusable,
// though Copyright (C) 2000 Dallas Semiconductor Corporation
static const uint8_t PROGMEM dscrc_table[] = {
      0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
    157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
     35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
    190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
     70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
    219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
    101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
    248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
    140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
     17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
    175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
     50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
    202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
     87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
    233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
    116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

//
// Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM
// and the registers.  (note: this might better be done without to
// table, it would probably be smaller and certainly fast enough
// compared to all those delayMicrosecond() calls.  But I got
// confused, so I use this table from the examples.)
//
uint8_t crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;

	while (len--) {
		crc = pgm_read_byte(dscrc_table + (crc ^ *addr++));
	}
	return crc;
}

