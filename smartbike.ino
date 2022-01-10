/*
  ReadField
  
  Description: Demonstates reading from a public channel which requires no API key and reading from a private channel which requires a read API key.
               The value read from the public channel is the current outside temperature at MathWorks headquaters in Natick, MA.  The value from the
               private channel is an example counter that increments every 10 seconds.
  
  Hardware: ESP32 based boards
  
  !!! IMPORTANT - Modify the secrets.h file for this project with your network connection and ThingSpeak channel details. !!!
  
  Note:
  - Requires installation of EPS32 core. See https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md for details. 
  - Select the target hardware from the Tools->Board menu
  - This example is written for a network using WPA encryption. For WEP or WPA, change the WiFi.begin() call accordingly.
  
  ThingSpeak ( https://www.thingspeak.com ) is an analytic IoT platform service that allows you to aggregate, visualize, and 
  analyze live data streams in the cloud. Visit https://www.thingspeak.com to sign up for a free account and create a channel.  
  
  Documentation for the ThingSpeak Communication Library for Arduino is in the README.md folder where the library was installed.
  See https://www.mathworks.com/help/thingspeak/index.html for the full ThingSpeak documentation.
  
  For licensing information, see the accompanying license file.
  
  Copyright 2020, The MathWorks, Inc.
*/

#include "heltec.h"
#include <WiFi.h>
#include "ThingSpeak.h" // always include thingspeak header file after other header files and custom macros

#define BAND    869E6

int lck = 1;

char ssid[] = "小鳥華生";   // your network SSID (name) 
char pass[] = "qwer1234";   // your network password           // your network key Index number (needed only for WEP)

byte localAddress = 0xBB;     // address of this device
byte destination = 0xFD;
long lastSendTime = 0;
int interval = 2000;

WiFiClient  client;

// Weather station channel details
unsigned long ChannelNumber = 1620834;
unsigned int FieldNumber = 3;

// Counting channel details
const char *ReadAPIKey = "NCFYKMBO44O15L9Y"; 
const char * WriteAPIKey = "GGFTTH1UFCNYT3HU";

void setup() {
  Serial.begin(115200);  //Initialize serial
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo native USB port only
  }
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  WiFi.mode(WIFI_STA);   
  ThingSpeak.begin(client);  // Initialize ThingSpeak
}

void loop() {

  int count = 0;
  
  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected");
  }
  if (millis() - lastSendTime > interval)
  {
    count = readData();
    if(lck == 1 && count == 1){
      sendMessage("1");
      lck = 0;
    }
    if(lck == 0 && count == 0){
      sendMessage("0");
      lck = 1;
    }
    Serial.println("Sending ");
    lastSendTime = millis();            // timestamp the message
    interval = 2000;    // 2-3 seconds
  }
  onReceive(LoRa.parsePacket());
  
}

void sendMessage(String number)
{
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(number.length());        // add payload length
  LoRa.print(number);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
}

void onReceive(int packetSize)
{
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte Latitude = LoRa.read();
  byte Longitude = LoRa.read();
  byte incomingLength = LoRa.read();
 

  String incoming = String(Latitude) + String(Longitude);

  if (incomingLength != incoming.length())
  {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }
  long Lat = long(Latitude);
  long Lng = long(Longitude);
  int i  = readData();
  ThingSpeak.setField(1, Lat);
  ThingSpeak.setField(2, Lng);
  ThingSpeak.setField(3, i);
  int x = ThingSpeak.writeFields(ChannelNumber, WriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  
}

int readData(){
  // Read in field 1 of the private channel which is a counter  
  int count = ThingSpeak.readLongField(ChannelNumber, FieldNumber, ReadAPIKey);  

   // Check the status of the read operation to see if it was successful
  int statusCode = ThingSpeak.getLastReadStatus();
  if(statusCode == 200){
    Serial.println("Data: " + String(count));
    return count;
  }
  else{
    Serial.println("Problem reading channel. HTTP error code " + String(statusCode)); 
  }
}
