/*
   TX Node for Teleinformation EDF (compteur type A14C5)

   Info from http://blog.cquad.eu/2012/02/02/recuperer-la-teleinformation-avec-un-arduino/
   Doc from EDF http://norm.edf.fr/pdf/HN44S812emeeditionMars2007.pdf

   New Implementation based on Mini Ultra 8 MHz
   http://www.rocketscream.com/shop/mini-ultra-8-mhz-arduino-compatible

   Select boad : "Arduino Pro or Pro Mini"
   Select processor : "ATmega328 3.3V 8 Mhz"

   Low Power Library by RocketScream
   http://www.rocketscream.com/blog/2011/07/04/lightweight-low-power-arduino-library/

*/
#include <LowPower.h>
#include <Manchester.h>

#define MAX_BUFFER_SIZE 200
//#define DEBUG
#define SLEEP_LOOP  2 // Deep Sleep total time =  SLEEP_LOOP * 8 seconds
#define sensID 5  // Sensor ID

#define txPin 8  // TX Out
#define ledPin 13 // LED

/*
   ---- Global Variables ----
*/

// We should have enough space for full packet.
char _packetBuffer[MAX_BUFFER_SIZE];

// Sensor Message Structure
typedef struct sensorData_t {
  uint8_t id; // size 1
  float value; // size 4
  uint8_t checksum; // size 1
};

typedef union sensorData_union_t {
  sensorData_t data;
  uint8_t raw[6]; // total size of 6 bytes
};

// Sensor Message
sensorData_union_t _message;

/*
   ---- Arduino Setup ----
*/
void setup()
{
  // Write Sensor ID
  _message.data.id = sensID;
}


/*
   ---- Arduino Loop ----
*/
void loop()
{
  // LED On
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  
  // Read Power and send to main station
  acquirePower();

  // LED Off
  digitalWrite(ledPin, LOW);
  pinMode(ledPin, INPUT);  // set pin mode to input (better for low power)

  // Go to sleep !
  deepSleep();
}



void acquirePower()
{
  // Setup Serial communication with EDF box.
  Serial.begin(1200);

#ifdef DEBUG
  Serial.println(F("Begin serial acquisition"));
#endif

  // End of packet
  char ETX = 0x03;

  // Read Serial
  int nbrRead = Serial.readBytesUntil(ETX, _packetBuffer, MAX_BUFFER_SIZE);

  if (nbrRead > 0)
  {
    // Force end of string
    _packetBuffer[min(nbrRead, MAX_BUFFER_SIZE - 1)] = '\0';


#ifdef DEBUG
    Serial.print(F("Read ")); Serial.print(nbrRead); Serial.print(F(" chars : "));
    Serial.print('<'); Serial.print(_packetBuffer); Serial.println('>');
#endif

    // Find Instant Power information
    // This message have at least 10 char
    // pattern : PAPP XXXXX {checksum}

    int pappIndex = indexOf(_packetBuffer, "PAPP");

    if (pappIndex >= 0 && nbrRead - pappIndex >= 10)
    {
#ifdef DEBUG
      Serial.print(F("PAPP index = ")); Serial.print(pappIndex);
#endif
      // Copy into pappBuffer
      char pappBuffer[11];

      // we don't want to share same buffer
      strncpy(pappBuffer, _packetBuffer + pappIndex, 10);

      // Force end of string
      pappBuffer[10] = '\0';

#ifdef DEBUG
      Serial.print(F(", Buffer = <")); Serial.print(pappBuffer); Serial.print('>');
#endif

      // Convert to Float
      float pappValue = atof(pappBuffer + 5);

#ifdef DEBUG
      Serial.print(F(", Value = <")); Serial.print(pappValue); Serial.println('>');
#endif
    
      // New Value Event
      fireNewValueEvent(pappValue);
    }
  }


#ifdef DEBUG
  Serial.println(F("End serial acquisition"));
#endif

  // Close Serial
  Serial.end();
}

/*
   Send new value to main station
*/
void fireNewValueEvent(float value)
{
  // Save Power
  _message.data.value = value;

  // Compute Checksum : CRC8 on ID+Value (Checksum is computed on the first 5 bytes)
  _message.data.checksum = crc8(_message.raw, 5);

#ifdef DEBUG
  Serial.print(F("TX Send = <"));
  for (int i = 0 ; i < 6 ; i ++)
  {
    Serial.print(_message.raw[i], HEX);
    Serial.print('x');
  }
  Serial.println('>');
#endif

  // TX Setup
  man.setupTransmit(txPin);

  // Send All Data
  man.transmitArray(6, _message.raw);

  // set pin mode to input (better for low power)
  pinMode(txPin, INPUT);
}


/*
   Enter in deep sleep
*/
void deepSleep()
{
  for (int i = 0 ; i < SLEEP_LOOP ; i++)
  {
    // Enter power down state with ADC and BOD module disabled
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
}


/*
    ---- Utilities functions ----
*/

/*
    String Index Of
*/
int indexOf(char * string, char * sub)
{
  const char *found = strstr(string, sub);

  if (found == NULL) return -1;

  return found - string;
}

// This table comes from Dallas sample code where it is freely reusable,
// though Copyright (C) 2000 Dallas Semiconductor Corporation
static const uint8_t PROGMEM dscrc_table[] = {
  0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
  157, 195, 33, 127, 252, 162, 64, 30, 95,  1, 227, 189, 62, 96, 130, 220,
  35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93,  3, 128, 222, 60, 98,
  190, 224,  2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
  70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89,  7,
  219, 133, 103, 57, 186, 228,  6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
  101, 59, 217, 135,  4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
  248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91,  5, 231, 185,
  140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
  17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
  175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
  50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
  202, 148, 118, 40, 171, 245, 23, 73,  8, 86, 180, 234, 105, 55, 213, 139,
  87,  9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
  233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
  116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
};

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



