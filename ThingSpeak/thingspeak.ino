/*
 Arduino --> ThingTweet via Ethernet
 
 The ThingTweet sketch is designed for the Arduino + Ethernet Shield.
 This sketch updates a Twitter status via the ThingTweet App
 (https://thingspeak.com/docs/thingtweet) using HTTP POST.
 ThingTweet is a Twitter proxy web application that handles the OAuth.
 
 Getting Started with ThingSpeak and ThingTweet:
 
    * Sign Up for a New User Account for ThingSpeak - https://www.thingspeak.com/users/new
    * Link your Twitter account to the ThingTweet App - Apps / ThingTweet
    * Enter the ThingTweet API Key in this sketch under "ThingSpeak Settings"
  
 Arduino Requirements:
 
   * Arduino with Ethernet Shield or Arduino Ethernet
   * Arduino 1.0 IDE
   * Twitter account linked to your ThingSpeak account
   
  Network Requirements:
   * Ethernet port on Router    
   * DHCP enabled on Router
   * Unique MAC Address for Arduino
 
 Created: October 17, 2011 by Hans Scharler (http://www.nothans.com)
 Updated: December 7, 2012 by Hans Scharler (http://www.nothans.com)
 
 Additional Credits:
 Example sketches from Arduino team, Ethernet by Adrian McEwen
 
*/

#include <SPI.h>
#include <UIPEthernet.h>
#include <HTTPClient.h>

// Local Network Settings
byte mac[] = { 0xD4, 0x28, 0xB2, 0xFF, 0xA0, 0xA1 }; // Must be unique on local network

// ThingSpeak Settings
char thingSpeakAddress[] = "api.thingspeak.com";

//String thingSpeakAPIKey = "XXXXXXXXXXXXXXXX"; //Write
//String thingSpeakAPIKey = "XXXXXXXXXXXXXXXX"; //Read
//String thingtweetAPIKey = "XXXXXXXXXXXXXXXX";
byte server[] = { 184,106,153,149 }; // ThingSpeak

// Variable Setup
long lastConnectionTime = 0; 
boolean lastConnected = false;
int failedCounter = 0;

int number = 0;
char data[10];

#define FEED_URI "/update.json"
#define THINGSPEAK_API_KEY "XXXXXXXXXXXXXXXX"

// Initialize Arduino Ethernet Client
EthernetClient client;
void setup(){
  Ethernet.begin(mac);
  Serial.begin(9600);  
  
  delay(1000);  
  
  Serial.println("connecting...");
  
  // Update Twitter via ThingTweet
  //updateTwitterStatus("My thing is social @thingspeak");
}

void loop(){
  // Print Update Response to Serial Monitor
  /*if (client.available()){
    char c = client.read();
    Serial.print(c);
  }
  Serial.println(Ethernet.localIP());*/
  // Disconnect from ThingSpeak
  if (!client.connected() && lastConnected){
    Serial.println("...disconnected");
    Serial.println();
    client.stop();
  }
  
  lastConnected = client.connected();
  delay(5000);
  number = (number%30) + 10;
  sprintf(data, "field1=%d", number);
  Serial.println(data);
  //field1(number);
  // create HTTPClient
  
  HTTPClient httpClient( thingSpeakAddress, server );

  http_client_parameter thingSpeak_APIHeader[] = {
      { "api_key", THINGSPEAK_API_KEY  }
      ,
      { NULL, NULL }
  };
  
  // FILE is the return STREAM type of the HTTPClient
  FILE* result = httpClient.putURI( FEED_URI, NULL, data, thingSpeak_APIHeader );

  int returnCode = httpClient.getLastReturnCode();
  
  if (result!=NULL) {
    httpClient.closeStream(result);  // this is very important -- be sure to close the STREAM
  } 
  else {
    Serial.println("failed to connect");
  }
  
  if (returnCode==200) {
    Serial.println("data uploaded");
  } 
  else {
    Serial.print("ERROR: Server returned ");
    Serial.println(returnCode);
  }
}
//Valor que puede tener la variable field 1
//reservado 10
//lleno 20
//vacio 30

void field1(int tsData){
  Serial.println(tsData);
  Serial.println("Update field1");
  if (client.connect(thingSpeakAddress, 80)){
    // Create HTTP POST Data
    String data = "api_key=DF0QER2XZ125TD5U&field1="+tsData;
            
    //https://api.thingspeak.com/update.json?api_key=DF0QER2XZ125TD5U&field1=0

    client.print("POST https://api.thingspeak.com/update.json HTTP/1.1\n");
    //client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("Content-Type: application/json\n");
    client.print("Content-Length: ");
    client.print(data.length());
    client.print("\n\n");    
    
    lastConnectionTime = millis();
    
    if (client.connected()){
      Serial.println("Connecting to ThingSpeak...");
      Serial.println();
      
      failedCounter = 0;
    }else{
      failedCounter++;
  
      Serial.println("Connection to ThingSpeak failed ("+String(failedCounter, DEC)+")");   
      Serial.println();
    }
    
  }else{
    failedCounter++;
    
    Serial.println("Connection to ThingSpeak Failed ("+String(failedCounter, DEC)+")");   
    Serial.println();
    
    lastConnectionTime = millis(); 
  }
}
