//
// Display Temp
//
#include <VirtualWire.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <CapacitiveSensor.h>

#define I2C_ADDR 0x27 // <<----- Add your address here. Find it from I2C Scanner
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7


int rxLed = 13; // RX LED
int rxPin = 11; // RX In

LiquidCrystal_I2C lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

CapacitiveSensor touch = CapacitiveSensor(2,4); 

// Sensor RX Data
typedef struct sensorData_t{
  byte id; // size 1
  float value; // size 4
};

typedef union sensorData_union_t{
  sensorData_t data;
  uint8_t raw[5]; // total size of 5 bytes
};

// RX message
uint8_t message[VW_MAX_MESSAGE_LEN];

// LAst Touch
unsigned long lastTouch;

void setup() {
  // Led Setup
  pinMode(rxLed, OUTPUT);

  // Debug
  Serial.begin(9600);
  Serial.println("setup");

  // Set Up RX
  vw_set_rx_pin(rxPin);
  vw_setup(2000);

  // Set Up LCD
  lcd.begin (16,2); // <<----- My LCD was 16x2
  // Switch on the backlight
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.home (); // go home

    lcd.print("TTK is here !");

  // Start the receiver
  vw_rx_start();      
}

void loop() {

  // Turn the ligth ?
  long touched =  touch.capacitiveSensor(30);
  
  unsigned long currentTime = millis();
  
  if(touched>100)
  {
    Serial.print("Touch ");
     Serial.println(touched);   
    lcd.setBacklight(HIGH); // Backlight on
    lastTouch = currentTime;
  }
  else if( currentTime - lastTouch > 10000)
  {
    lcd.setBacklight(LOW); // Backlight off
  }

  // Wait Message (max 200ms)
  if( vw_wait_rx_max(200))
  {
    // message size
    uint8_t messageLen = VW_MAX_MESSAGE_LEN;

    if (vw_get_message(message, &messageLen)) // Read if OK
    {
      digitalWrite(rxLed, HIGH);

      // DEBUG
      if(Serial)
      {
        Serial.print("Got: ");

        // HEX DUMP
        for (int i = 0; i < messageLen; i++)
        {
          Serial.print(message[i], HEX);
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
          sensor.raw[k] = message[k];
        }

        Serial.print("Sensor=");
        Serial.print(sensor.data.id);
        Serial.print(", Temp=");
        Serial.println(sensor.data.value);

        // LCD
        lcd.setCursor (0,1); // go to start of 2nd line
        lcd.print(sensor.data.value);
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

