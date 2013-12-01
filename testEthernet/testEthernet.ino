// Arduino demo sketch for testing the DHCP client code
//
// Original author: Andrew Lindsay
// Major rewrite and API overhaul by jcw, 2011-06-07
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html

#include <EtherCard.h>


#define REQUEST_RATE 10000 // milliseconds

static byte mymac[] = { 
  0x74,0x69,0x69,0x2D,0x30,0x31 };

byte Ethernet::buffer[700];

// remote website ip address and port
static byte ttkserver[] = { 192,168,0,1 };

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
  if (!ether.dhcpSetup())
    Serial.println( "DHCP failed");

  ether.printIp("My IP: ", ether.myip);
  ether.printIp("Netmask: ", ether.mymask);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);
  
    // call this to report others pinging us
  ether.registerPingCallback(gotPinged);
}

void loop () {
  // check if anything has come in via ethernet
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  
    if (pos) { // check if valid tcp data is received
    // data received from ethernet

    char* data = (char *) Ethernet::buffer + pos;

    Serial.println("----------------");
    Serial.println("data received:");
    Serial.println(data);
    Serial.println("----------------");
    
    
    
    // Send Back Page
    memcpy_P(ether.tcpOffset(), page, sizeof page);
    
    Serial.println("End MemCpy");
    
    ether.httpServerReply(sizeof page - 1);
    
    Serial.println("End Send");
    
  }
  
  //
  if (millis() > timer + REQUEST_RATE) {
    timer = millis();
    
    Serial.println("\n>>> REQ");
    
    ether.copyIp(ether.hisip, ttkserver);
    ether.printIp("Server: ", ether.hisip);
    
    char buffer[50];
    sprintf(buffer, "%d/%ld", 42, timer);
    Serial.println(buffer);
    
    ether.browseUrl(PSTR("/cakephp/points/add/"), buffer, PSTR("TTKSERVER"), my_result_cb);
  }
  
  
}


// called when a ping comes in (replies to it are automatic)
static void gotPinged (byte* ptr) {
  ether.printIp(">>> ping from: ", ptr);
}

// called when the client request is complete
static void my_result_cb (byte status, word off, word len) {
  Serial.print("<<< reply ");
  Serial.print(millis() - timer);
  Serial.println(" ms");
  Serial.println((const char*) Ethernet::buffer + off);
}

