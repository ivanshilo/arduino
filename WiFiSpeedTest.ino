/*
 * sketch for WiFi upload speed test from Arduino to computer
 * how-to use: open serial port monitor while Arduino starts up
 * type in your browser http://[Aduino_ipaddress]/capture
 * enjoy the text till it's finished
 * check out statistics in serial port monitor window
 * 
 * Author: Ivan Shilo. 2016
 */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// Enabe debug tracing to Serial port.
#define DEBUGGING

int wifiType = 0; // 0:Station  1:AP
const char* ssid = "SSID"; // Put your SSID here
const char* password = "password"; // Put your PASSWORD here
const short bufSize = 3000; //best and max for WiFi is 1460
char testBuffer[bufSize];

ESP8266WebServer server(80);
WiFiClient client;


void serverCapture(){
  
  client = server.client();
  long total_time = millis();
  long len = 240000;
  long lenF = len;
  long cycles = 0;
  float speedWiFi = 0.0;
   
  
  if (!client.connected()) {
    Serial.println("client not connected!");
    return;
  }
  
  client.setNoDelay(1);

  //send page header
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: text/plain\r\n";
  response += "Content-Length: " + String(len) + "\r\n\r\n";
  server.sendContent(response);

  Serial.print("Starting transfer of ");
  Serial.print(len);
  Serial.println(" bytes");
  
  while (len > 0){
      
      int toSend = len > bufSize ? bufSize : len;
      client.write(&testBuffer[0], toSend);
      len -= bufSize;
      cycles++;

      // there should be some delay, otherwise WIFI gets stuck
      delay(1);
  }

  // show statistics
  total_time = millis() - total_time;
  Serial.print("send total_time used (in miliseconds):");
  Serial.println(total_time, DEC);
  speedWiFi = (float)lenF / (float)total_time;
  Serial.print("Bytes transferred: ");
  Serial.println(lenF);
  Serial.print("WiFi speed: ");
  Serial.print(speedWiFi);
  Serial.println("KByte/s");
  client.stop();
  
  
}

void handleNotFound(){
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text/plain", message);
  
  
}

void setup() {

  Serial.begin(115200);
  Serial.println("ArduCAM Start!");

  
  if (wifiType == 0){
    // Connect to WiFi network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("WiFi connected");
    Serial.println("");
    Serial.println(WiFi.localIP());
  }else if (wifiType == 1){
    Serial.println();
    Serial.println();
    Serial.print("Share AP: ");
    Serial.println(ssid);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    Serial.println("");
    Serial.println(WiFi.softAPIP());
  }

  

  // prepare our buffer to be transferred
  for (int i = 0; i < bufSize; i++){
    int k = i;
    testBuffer[k] = '1';
    testBuffer[k+1] = '2';
    testBuffer[k+2] = '3';
    testBuffer[k+3] = '4';
    testBuffer[k+4] = '5';
    testBuffer[k+5] = '6';
    testBuffer[k+6] = '7';
    testBuffer[k+7] = '8';
    i = i+7;
  }
  
  // Start the server
  server.on("/capture", HTTP_GET, serverCapture);
//  server.on("/stream", HTTP_GET, serverStream);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Server started");
}

void loop() {
  server.handleClient();
}

