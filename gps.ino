#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include "heltec.h"
/*
   This sample sketch demonstrates the normal use of a TinyGPS++ (TinyGPSPlus) object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
*/
#define BAND    869E6
char ssid[] = "ICMNLAB"; //SSID
char password[] = "ndhuicmn";
String url = "http://api.thingspeak.com/update?api_key=WCVSW6OA6B0Y23XA";
static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;
byte localAddress = 0xFD;
byte destination = 0xBB;
int interval = 10000;
long lastSendTime = 0;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial SerialGPS(RXPin, TXPin);

byte Lat;
byte Lng;


void setup()
{
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Serial.begin(115200);
  SerialGPS.begin(9600);
}

void loop()
{
  // This sketch displays information every time a new sentence is correctly encoded.
  while (SerialGPS.available() > 0){
    if (gps.encode(SerialGPS.read())){
       if (millis() - lastSendTime > interval)
       {
          sendMessage();
          Serial.println("Sending ");
          lastSendTime = millis();            // timestamp the message
          interval = 15000;    // 2-3 seconds
        }
    }
    onReceive(LoRa.parsePacket());
  }
}

void sendMessage()
{
  if (gps.location.isValid())
  {
     Lat = gps.location.lat();
     Lng = gps.location.lng();
  }
  else
  {
    Lat = 0;
    Lng = 0;
    Serial.print("InValid");
  }
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(Lat);
  LoRa.write(Lng);
  String outgoing = (String)Lat + (String)Lng;
  LoRa.write(outgoing.length());
  LoRa.endPacket();                     // finish packet and send it
}

void onReceive(int packetSize)
{
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingLength = LoRa.read();

  String incoming = "";

  while (LoRa.available())
  {
    incoming += (char)LoRa.read();
  }

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

  // if message is for this device, or broadcast, print details:
  if(incoming == "1"){
    Serial.println("開鎖");
  }

  else if(incoming == "0"){
    Serial.println("關鎖");
  }
  Serial.print("recdata: ");
  Serial.println(incoming);
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
}
