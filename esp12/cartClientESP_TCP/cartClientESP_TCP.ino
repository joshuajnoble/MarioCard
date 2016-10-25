#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

char ssid[] = "mariocard";

IPAddress cartServer(192, 168, 42, 1); // server always lives at this address
int port = 3000;
// A UDP instance to let us send and receive packets over UDP
WiFiClient client;

char ID = 'x';

char updateReq[9] = "update:x";
char colorReq[10] = "color:x:x";

char commandBuffer[8];
bool hasSetUpCart = false;
unsigned long lastUpdate;

void setup() {

  Serial.begin(115200);
  delay(1000);

  // put your setup code here, to run once:
  WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("registering");
    delay(800);
  }
  
  if (!client.connect(cartServer, port)) {
    Serial.println("connection failed");
    return;
  }

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      client.stop();
      return;
    }
  }

  if( client.available() ) {
    ID = client.read();
  }

  updateReq[7] = ID;
  
  hasSetUpCart = true;
  lastUpdate = millis();
}

void loop() {


  // put your main code here, to run repeatedly:
  if (Serial.available() > 0)
  {
    char c = Serial.read(); // only ever 1 char
    client.connect(cartServer, port);
    colorReq[8] = c;
    client.print(&colorReq[0]);
  }


  if( millis() - lastUpdate > 100)
  {

    // do a game update
    client.connect(cartServer, port);
    client.print(updateReq);
    
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        client.stop();
        return;
      }
    }
  
    if( client.available() > 6) {
      int result = client.read( (unsigned char*) &commandBuffer[0], 7);
      if (result != -1)
      {
        Serial.print(commandBuffer);
      }
    }
    
    client.stop();
    lastUpdate = millis();
  }
}
