/*

 Udp NTP Client

 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket
 For more on NTP time servers and the messages needed to communicate with them,
 see http://en.wikipedia.org/wiki/Network_Time_Protocol

 created 4 Sep 2010
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 updated for the ESP8266 12 Apr 2015 
 by Ivan Grokhotkov

 This code is in the public domain.

***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Time.h>
#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
//#include "Adafruit_IO_Client.h"


// Configure Adafruit IO access.
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "----"
#define AIO_KEY         "-------------------------------------"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
//for SSL
//WiFiClientSecure client;

// Store the MQTT server, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);

/****************************** Feeds ***************************************/

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char MY_FEED[] PROGMEM = AIO_USERNAME "/feeds/esptestfeed";
Adafruit_MQTT_Publish testfeed = Adafruit_MQTT_Publish(&mqtt, MY_FEED);

// Setup a feed called 'onoff' for subscribing to changes.
//const char ONOFF_FEED[] PROGMEM = AIO_USERNAME "/feeds/photocell";
//Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, ONOFF_FEED);

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();
/**************************************************************************/

// Connect via i2c, default address #0 (A0-A2 not jumpered)
Adafruit_LiquidCrystal lcd(0); 

char ssid[] = "----";            //  your network SSID (name)
char pass[] = "----";     // your network password

// set pin numbers:
const int button1Pin = 15;   // the number of the pushbutton pin
const int button2Pin = 13;   // the number of the pushbutton pin
const int led1Pin =  2;      // the number of LED pin for button 1
const int led2Pin = 0;       // the unmber of LED pin for button 2

int button1State = 0;         // variable for reading the pushbutton status
int button2State = 0;         // variable for reading the pushbutton status 
int button1LastState = 0;     // variable for reading the pushbutton status
int button2LastState = 0;     // variable for reading the pushbutton status

unsigned long startTime = 0;
unsigned long stopTime = 0;
unsigned long timediff = 0;

boolean timerActive = false;
boolean timerStopped = true;

unsigned int localPort = 2390;      // local port to listen for UDP packets of NTP
uint8_t timeZone = 0;

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void HandleIOFeed(uint32_t value) {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here
  /*
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Got: "));
      Serial.println((char *)onoffbutton.lastread);
    }
  }*/

  if(mqtt.ping()) {
      // Now we can publish stuff!
      Serial.print(F("\nSending value to a feed "));
      Serial.print(value);
      Serial.print("...");
      if (!testfeed.publish(value)) {
          Serial.println(F("Failed"));
      } else {
          Serial.println(F("OK!"));
      }
    } else {
      mqtt.disconnect();
  }
  

}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 3 seconds...");
       mqtt.disconnect();
       delay(3000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         //while (1);
         Serial.println("Failed to connect after 3 retries!");
       }
  }
  Serial.println("MQTT Connected!");
}

bool GetTimeFromNTP() {
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
    return false;
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);
    epoch = epoch + timeZone*3600L;
    setTime(epoch);
  
    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print(hour()); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if (minute() < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print(minute()); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( second() < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(second()); // print the second
    return true;

  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  lcd.begin(16,2);
  
  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  byte tries = 4;
  while (tries > 0) {
    if (GetTimeFromNTP()){
      tries = 0;
    } else {
        tries--;
        delay(500);
    }
  }

  // initialize the LED pin as an output:
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);

  // read the state of the pushbutton value:
  button1LastState = digitalRead(button1Pin);
  button2LastState = digitalRead(button2Pin);
  
}

void ShowCurrentTime(){
  
  lcd.setCursor(0,0);
  static char dateStr[16];
  sprintf(dateStr, "%03s %02d/%02d/%04d", dayShortStr(weekday()),day(), month(), year());
  lcd.print(dateStr);

  static char timeStr[17];
  String timeZoneStr = String("(");
  if (timeZone > 0) timeZoneStr = timeZoneStr + '+' + timeZone + ')';
  if (timeZone < 0) timeZoneStr = timeZoneStr + '-' + timeZone + ')';
  if (timeZone == 0) timeZoneStr = timeZoneStr + "UTC)";
  
  sprintf(timeStr, "%02d:%02d:%02d %s", hour(),minute(),second(),timeZoneStr.c_str());
  lcd.setCursor(0,1);
  lcd.print(timeStr);
   
  
  // wait ten seconds before asking for the time again
  delay(500);
}

void ShowTimer(){
  timediff = millis() - startTime;
  Serial.println(timediff);
  int32_t milliseconds = (timediff % 1000);
  int32_t timediff_sec = timediff/1000;
  short hour = (timediff_sec  % 86400L) / 3600;
  short minute = (timediff_sec % 3600) / 60;
  short second = (timediff_sec % 60);
  
  lcd.setCursor(0,0);
  char result[16];
  sprintf(result, "%02d:%02d:%02d.%d",hour,minute,second,milliseconds);
  lcd.print(result);
  delay(50);  
}

void ReadButtons() {
 // read the state of the pushbutton value:
  button1State = digitalRead(button1Pin);
  button2State = digitalRead(button2Pin);
    
  // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH:
  if (button1State == HIGH) {
    // turn LED on:
    digitalWrite(led1Pin, LOW);
    if (button1State != button1LastState) {
       button1LastState = button1State;
       lcd.clear();
       startTime = millis();
       Serial.print(F("Start time: "));
       Serial.println(startTime);
       timerActive = true;
       timediff = 0;
    }
    
  } else {
    button1LastState = 0;
    // turn LED off:
    digitalWrite(led1Pin, HIGH);
  }

  if (button2State == HIGH) {
    // turn LED on:
    digitalWrite(led2Pin, LOW);
    if (button2State != button2LastState) {
       button2LastState = button2State;
       timediff = millis() - startTime;
       ShowTimer();
       
       if (timerActive) {
        stopTime = millis();
        timerActive = false;
        timerStopped = false;
        HandleIOFeed(timediff);
        
        
       } else {
          lcd.clear();
          timerStopped = true;
         
       }
       
    }
    
  } else {
    button2LastState = 0;
    // turn LED off:
    digitalWrite(led2Pin, HIGH);
  } 
}



void loop()
{
  
  ReadButtons();
  if (timerActive) {
    
    ShowTimer();
  } else 
  {
    
    if (timerStopped)
    {
      Serial.println("show current time");
      ShowCurrentTime();
    }
  }

  
  
}

