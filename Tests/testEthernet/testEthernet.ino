// Arduino demo sketch for testing the DHCP client code
//
// Original author: Andrew Lindsay
// Major rewrite and API overhaul by jcw, 2011-06-07
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html

#include <EtherCard.h>


#define REQUEST_RATE 10000 // milliseconds

byte Ethernet::buffer[300];

static BufferFiller bfill;  // used as cursor while filling the buffer

// MAC Address
static byte mymac[] = {
  0x74,0x69,0x69,0x2D,0x30,0x31 };

// remote website ip address and port
static byte ttkserver[] = { 
  192, 168, 0, 1 };

// Arduino IP
static byte arduinoIP[] = { 
  192, 168, 0, 42 };

// gateway ip address
static byte gwip[] = { 
  192, 168, 0, 254 };

char page[] PROGMEM =
"HTTP/1.0 200 OK\r\n"
"Content-Type: text/html\r\n"
"\r\n"
"<html>"
"<head><title>"
"Plop !"
"</title></head>"
"<body>"
"<h3>I am TTK</h3>"
"<p>Time is $D</p>"
"</body>"
"</html>"
;

unsigned long timer;


void setup () {
  Serial.begin(57600);
  Serial.println("\n[testDHCP]");

  Serial.print("MAC: ");
  for (byte i = 0; i < 6; ++i) {
    Serial.print(mymac[i], HEX);
    if (i < 5)
      Serial.print(':');
  }
  Serial.println();

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");

  Serial.println("Setting up DHCP");

  // Static IP
  ether.staticSetup(arduinoIP,gwip);

  //if (!ether.dhcpSetup())
  //  Serial.println( "DHCP failed");

  ether.printIp("My IP: ", ether.myip);
  ether.printIp("Netmask: ", ether.mymask);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);

  ether.copyIp(ether.hisip, ttkserver);
  ether.printIp("Destination Server: ", ether.hisip);

  while (ether.clientWaitingGw())
    ether.packetLoop(ether.packetReceive());
  Serial.println("Gateway found");

  // call this to report others pinging us
  ether.registerPingCallback(gotPinged);
}

void loop () {
  // check if anything has come in via ethernet
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if (pos) { 

    displayFreeRam();

    bfill = ether.tcpOffset();

    char* data = (char *) Ethernet::buffer + pos;
    Serial.println("data received:");
    Serial.println(data);

    // Populate Page to Buffer
    bfill.emit_p(page,millis());

    ether.httpServerReply(bfill.position()); // send web page data

    Serial.println("End Send");
  }

  //
  if (millis() > timer + REQUEST_RATE) {
    timer = millis();

    Serial.println("\n>>> REQ");


    byte sensorID = 42;
    float newValue = 123.45;
    
    /*
    
    // Convert to String
    char sensorIdString[4]; 
    sprintf(sensorIdString, "%d", sensorID);
  
    // Convert Float to String
    char newValueString[10];
    dtostrf(newValue, 1, 2, newValueString);
    
    displayFreeRam();
    
    Serial.print(F("Sensor ="));Serial.print(sensorIdString);
    Serial.print(F(", Value ="));Serial.println(newValueString);

    
    
    Stash::prepare(PSTR("GET /cakephp/points/add/$S/$S HTTP/1.0" "\r\n"
      "Host: $F" "\r\n"
      "\r\n"),
    sensorIdString, newValueString, PSTR("TTKSERVER"));
    
    */
    Stash stash;
    byte sd = stash.create();
    stash.print(sensorID);
    stash.print('/');
    stash.print(newValue);
    stash.save();

    Stash::prepare(PSTR("GET /cakephp/points/add/$H HTTP/1.0" "\r\n"
      "Host: $F" "\r\n"
      "\r\n"),
    sd, PSTR("TTKSERVER"));
  
    // send the packet - this also releases all stash buffers once done
    ether.tcpSend();

    //char buffer[50];
    //sprintf(buffer, "%d/%ld", 42, timer);
    //Serial.println(buffer);

    //ether.browseUrl(PSTR("/cakephp/points/add/"), buffer, PSTR("TTKSERVER"), my_result_cb);
  }


}


// called when a ping comes in (replies to it are automatic)
static void gotPinged (byte* ptr) {
  ether.printIp(">>> ping from: ", ptr);
}

// called when the client request is complete
void my_result_cb (byte status, word off, word len) {
  //Serial.print("<<< reply ");
  //Serial.print(millis() - timer);
  //Serial.println(" ms");
  Serial.println("<<< reply ");
  Serial.println((const char*) Ethernet::buffer + off);
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void displayFreeRam()
{
  Serial.print(F("Free Ram ="));
  Serial.println(freeRam());
}



