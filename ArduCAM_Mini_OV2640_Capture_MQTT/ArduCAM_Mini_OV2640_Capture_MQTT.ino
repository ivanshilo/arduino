#include <Adafruit_MQTT_Client.h>
#include <Adafruit_MQTT.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>

#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <SD.h>
//#include <PubSubClient.h>
#include "memorysaver.h"

// Enabe debug tracing to Serial port.
#define DEBUGGING

// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64

// Define how many callback functions you have. Default is 1.
#define CALLBACK_FUNCTIONS 1

#if defined(ESP8266)
// set GPIO15 as the slave select :
const int CS = 16;
#else
// set pin 10 as the slave select :
const int CS = 10;
#endif

const int sdChipSelect = 15;

int wifiType = 0; // 0:Station  1:AP
const char* ssid = "sara141"; // Put your SSID here
const char* password = "encr1pt10npa$$"; // Put your PASSWORD here
const char* mqtt_server = "10.20.30.32"; //MQTT server address
/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "10.20.30.32" //"io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "" //"user"
#define AIO_KEY         "" //"key"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client clientMQTT(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feeds
Adafruit_MQTT_Subscribe snapCommand = Adafruit_MQTT_Subscribe(&clientMQTT, "snapPIC", MQTT_QOS_1);

Adafruit_MQTT_Publish publishCommand = Adafruit_MQTT_Publish(&clientMQTT, "camPIC", MQTT_QOS_1);
Adafruit_MQTT_Publish publishLOGCommand = Adafruit_MQTT_Publish(&clientMQTT, "camLOG", MQTT_QOS_1);

//ESP8266WebServer server(80);
//WiFiClient client;
//PubSubClient clientMQTT(client);

File dataFile;

ArduCAM myCAM(OV2640, CS);

bool OpenFileOnSDCard()
{
  char filename[8];
  static int k = 0;
  
  //Construct a file name
  k = k + 1;
  itoa(k, filename, 10);
  strcat(filename, ".jpg");
  //Open the new file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  dataFile = SD.open(filename, O_WRITE | O_CREAT | O_TRUNC);
  if (! dataFile)
  {
    Serial.println("open file failed"); 
    return false;   
  }
  return true;
}

// reads data from camera, creates new file on SD card, calls to send data to SD and MQTT
void read_fifo_burst(ArduCAM myCAM)
{
  uint8_t temp, temp_last;
  static int i = 0;
  static uint8_t first_packet = 1;
  // very important parameter since it defines how much memory is required in buffer to be allocated
  uint16_t bSize = 128;
  byte buf[bSize];
  uint32_t length = 0;

  length = myCAM.read_fifo_length();
  if (length >= 393216 ) // 384kb
  {
    Serial.println("Over size.");
    return;
  }
  if (length == 0 ) //0 kb
  {
    Serial.println("Size is 0.");
    return;
  }

  Serial.print("Size is ");
  Serial.println(length);
  
  // new sd card code ====================
  OpenFileOnSDCard();
  // =====================================
  
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();//Set fifo burst mode
  SPI.transfer(0x00);
  //Read JPEG data from FIFO
  while ( (temp != 0xD9) | (temp_last != 0xFF))
  {
    
    temp_last = temp;
    temp = SPI.transfer(0x00);
    
    
    //Write image data to buffer if not full
    if (i < bSize)
    {
      buf[i++] = temp;
      //yield();
    }
    else
    {
      // turn off the camera transfer
      myCAM.CS_HIGH();
      sendClientData((char*)buf, bSize, 0x02);
      // turn on the camera transfer
      myCAM.CS_LOW();
      myCAM.set_fifo_burst();
      i = 0;
      buf[i++] = temp;
    }
    yield();
  }
  //Write the remain bytes in the buffer
  if (i > 0)
  {
    // turn off the camera transfer
    myCAM.CS_HIGH();
    sendClientData((char*)buf, i, 0x80);
    i = 0;    
  }
  //yield();
}

void start_capture(){
  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
}

// save data to file on SD card and publish through MQTT
void sendClientData(char *data, int size, unsigned char header) {
  
  // if the file is available, write to it:
  if (dataFile) {
     dataFile.write(&data[0], size);
     if (header == 0x80)
        dataFile.close();
     Serial.println("data written");   
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening file"); }

 // array for MQTT transfer
 // first byte is used to singal the end of data
 char dataM[2] = "S";
 if (header == 0x80) {
      Serial.println("last chunk of data");
      dataM[0]='E';
 }
 Serial.print("Copy data before MQTT send, data size: ");
 Serial.println(size);

/*
// for DEBUG
for (int i = 0; i < size; i++)
  data[i] = 'A';*/

  MQTT_connect();
  Serial.print("Sending metadata:");
  Serial.println(dataM);
  if (! publishCommand.publish((uint8_t*)dataM, sizeof(dataM)))
      Serial.println(F("Publish Failed."));
  else {
      Serial.println(F("Publish Success!")); }
    
  Serial.println("Sending data");
  publishCommand.publish((uint8_t*)data, size);
    
 /*if (!clientMQTT.connected()) {
    reconnect();
    }
 if (clientMQTT.connected())
 {
    Serial.print("Sending metadata:");
    Serial.println(dataM);
    clientMQTT.publish("camPIC", dataM);
    Serial.println("Sending data");
    clientMQTT.publish("camPIC", data, size);
 }*/
}

void camCapture(ArduCAM myCAM){
  
  size_t len = myCAM.read_fifo_length();
  if (len >= 393216){
    Serial.println("Over size.");
    return;
  }else if (len == 0 ){
    Serial.println("Size is 0.");
    return;
  }
  
//  if (!client.connected()) {
//    Serial.println("client not connected!");
//    return;
//  }
//  String response = "HTTP/1.1 200 OK\r\n";
//   response += "Content-Type: image/jpeg\r\n";
//  response += "Content-Length: " + String(len) + "\r\n\r\n";
//  server.sendContent(response);
  
  read_fifo_burst(myCAM);

}

// measures time to capture the image from the camera, calls to read image from the camera
void serverCapture(){
  start_capture();
  Serial.println("CAM Capturing");
  //client = server.client();
  int total_time = 0;

  total_time = millis();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  total_time = millis() - total_time;
  Serial.print("capture total_time used (in miliseconds):");
  Serial.println(total_time, DEC);
  
  total_time = 0;
  
  Serial.println("CAM Capture Done!");
  total_time = millis();
  // read image and send it to client
  camCapture(myCAM);
  total_time = millis() - total_time;
  Serial.print("send total_time used (in miliseconds):");
  Serial.println(total_time, DEC);
  Serial.println("CAM send Done!");
  publishLOGCommand.publish("CAM send Done!");
  //Clear the capture done flag
  myCAM.clear_fifo_flag();
}

/*void serverStream(){
  WiFiClient client = server.client();
  
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);
  
  while (1){
    start_capture();
    
    while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
    
    size_t len = myCAM.read_fifo_length();
    if (len >= 393216){
      Serial.println("Over size.");
      continue;
    }else if (len == 0 ){
      Serial.println("Size is 0.");
      continue;
    }
    
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();
    SPI.transfer(0xFF);
    
    if (!client.connected()) break;
    response = "--frame\r\n";
    response += "Content-Type: image/jpeg\r\n\r\n";
    server.sendContent(response);
    
    static const size_t bufferSize = 4096;
    static uint8_t buffer[bufferSize] = {0xFF};
    
    while (len) {
      size_t will_copy = (len < bufferSize) ? len : bufferSize;
      SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
      if (!client.connected()) break;
      client.write(&buffer[0], will_copy);
      len -= will_copy;
    }
    myCAM.CS_HIGH();
    
    if (!client.connected()) break;
  }
}*/
/*
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
  
  if (server.hasArg("ql")){
    int ql = server.arg("ql").toInt();
    myCAM.OV2640_set_JPEG_size(ql);
    Serial.println("QL change to: " + server.arg("ql"));
  }
}*/

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (clientMQTT.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = clientMQTT.connect()) != 0) { // connect will return 0 for connected
       Serial.println(clientMQTT.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       clientMQTT.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }

  Serial.println("MQTT Connected!");
}

// reconnect to MQTT server
/*void reconnect() {
 // Loop until we're reconnected
 while (!clientMQTT.connected()) {
 Serial.print("Attempting MQTT connection...");
 // Attempt to connect
 if (clientMQTT.connect("ESP8266 Client")) {
  Serial.println("connected");
  // ... and subscribe to topic
  clientMQTT.subscribe("snapPIC");
 } else {
  Serial.print("failed, rc=");
  Serial.print(clientMQTT.state());
  Serial.println(" try again in 5 seconds");
  // Wait 5 seconds before retrying
  delay(5000);
  }
 }
}*/

// MQTT callback function
//void callbackMQTT(char* topic, byte* payload, unsigned int length) {
void callbackMQTT(char* payload, uint16_t length) {
 Serial.print("Message arrived [");
 //Serial.print(topic);
 Serial.print("] ");
 Serial.print(length);
 Serial.print(" bytes ");
 char tempStr[length+1];
 int i = 0;
 // copy data in order to add "\0" to the end of string
 for (i=0;i<length;i++) {
    tempStr[i] = (char)payload[i]; 
 }
 tempStr[i]='\0';
 Serial.println(tempStr);
 
 // if first byte is S - start capturing
 if (tempStr[0]=='S')
 {
    publishLOGCommand.publish("S char received - start capturing image"); 
    serverCapture();     
 }
}

void setup() {
  uint8_t vid, pid;
  uint8_t temp;
#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif
  Serial.begin(115200);
  Serial.println("ArduCAM Start!");

  // set the CS as an output:
  pinMode(CS, OUTPUT);

  // initialize SPI:
  SPI.begin();
  //SPI.setFrequency(4000000); //4MHz

  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55){
    Serial.println("SPI1 interface Error!");
    while(1);
  }

  //Check if the camera module type is OV2640
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26) || (pid != 0x42)){
    Serial.println("Can't find OV2640 module!");
    while(1);
  }else{
    Serial.println("OV2640 detected.");
  }

  //Change to JPEG capture mode and initialize the OV2640 module
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.OV2640_set_JPEG_size(OV2640_1600x1200); //(OV2640_640x480);
  myCAM.clear_fifo_flag();
  myCAM.write_reg(ARDUCHIP_FRAMES, 0x00);

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

  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!SD.begin(sdChipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    return;
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  // connect to MQTT server
  //clientMQTT.setServer(mqtt_server, 1883);
  //clientMQTT.setCallback(callbackMQTT);

  snapCommand.setCallback(callbackMQTT);
  clientMQTT.subscribe(&snapCommand);
  
  // Start the server
  //server.on("/capture", HTTP_GET, serverCapture);
//  server.on("/stream", HTTP_GET, serverStream);
 // server.onNotFound(handleNotFound);
  //server.begin();
  Serial.println("Setup finished");
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets and callback em' busy subloop
  // try to spend your time here:
  clientMQTT.processPackets(10000);
  
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  
  if(! clientMQTT.ping()) {
    clientMQTT.disconnect();
  }

  /* old mqtt client loop
  if (!clientMQTT.connected()) {
    reconnect();}
  clientMQTT.loop();
  */
  //server.handleClient();
}

