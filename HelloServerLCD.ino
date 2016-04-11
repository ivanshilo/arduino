#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"

// Connect via i2c, default address #0 (A0-A2 not jumpered)
Adafruit_LiquidCrystal lcd(0);

const char* ssid = "----";
const char* password = "----";

ESP8266WebServer server(80);

const int led = 1;

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp8266!");
  lcd.setCursor(0,0);
  lcd.println("hello in root");
  digitalWrite(led, 0);
}

void handleNotFound(){
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  
  String serverUri = server.uri();
  Serial.println("server URI: " + serverUri);
  int strLength = serverUri.length() + 1;
  char serverUriChar[strLength];
  serverUri.toCharArray(serverUriChar, strLength);
  
  digitalWrite(led, 0);
  lcd.clear();
  lcd.setCursor(0,0);
  
  char convertedText[strLength];
  ConvertUriToText(serverUriChar, convertedText);
  lcd.print(convertedText);
  //lcd.println(serverUriChar);
}

// get the two strings for the LCD from the incoming HTTP GET request
void ConvertUriToText(char uriString[], char current_line[])
{
 
  int txt_index = 0;
  
  
        int str_len = strlen(uriString);
        
        if (str_len <= 2){
          current_line = uriString;
          return;
        }

        // copy the string to the buffer and replace %20 with space ' '
        for (int i = 0; i < str_len; i++) {

          // there should be 3 symbols left in the string to be replaced, othewise just copy
          if ((str_len-i) >=3){
            if (uriString[i] == '%') {
              // replace %20 with a space
              if ((uriString[i + 1] == '2') && (uriString[i + 2] == '0')) {
                current_line[txt_index++] = ' ';
                i += 2;
                }
            }
            else {         
              current_line[txt_index++] = uriString[i];
              }
            }
          else {
              current_line[txt_index++] = uriString[i];
            }
          }
       
        // terminate the string
        current_line[txt_index] = '\0';
        
        
}

void setup(void){
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  
  lcd.begin(16, 2);
  lcd.print(WiFi.localIP());

  server.on("/", handleRoot);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
}
