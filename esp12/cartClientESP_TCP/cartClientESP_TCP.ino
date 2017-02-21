#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char *ssid = "mariocard";

IPAddress cartServer(192, 168, 42, 1); // server always lives at this address
int port = 3000;
// A UDP instance to let us send and receive packets over UDP
WiFiClient client;

char ID = 'x';

char updateReq[9] = "update:x";
char colorReq[10] = "color:x:x";
char register_cart[14] = "register_cart";

char commandBuffer[8];
bool hasSetUpCart = false;
unsigned long lastUpdate;

void setup() {

  Serial.begin(9600);
  delay(1000);

  // put your setup code here, to run once:
  WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("registering");
    delay(800);
  }

  while (!client.connect(cartServer, port)) {
    Serial.println("connection failed");
    delay(500);
    //return;
  }

  while(reqID() == 0) {
    delay(500);
    Serial.println("can't connect");
  }

  Serial.println(" reading back id");
  if ( client.available() ) {
    ID = client.read();
    Serial.println(ID);
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

    if (!client.connected()) {
      client.connect(cartServer, port);
    }
    
    colorReq[8] = c;
    client.write(&colorReq[0], 8);
  }

  // do a game update
  if (!client.connected()) {
    client.connect(cartServer, port);
  }
  client.write(&updateReq[0], 8);

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      client.stop();
      while (!client.connected()) {
        client.connect(cartServer, port);
        delay(500);
      }
    }
  }

  if ( client.available() > 6) {
    int result = client.read( (unsigned char*) &commandBuffer[0], 7);
    if (result != -1)
    {
      Serial.println(commandBuffer);
    }
  }

  //client.flush();

  lastUpdate = millis();
}

int reqID()
{
  Serial.println(" requesting id");
  client.write(&register_cart[0], 13);

  bool reconnectFlag = false;

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      client.stop();
      client.flush();
      reconnectFlag = true;
      Serial.println(" broken wtf ");
    }
  }

  if(reconnectFlag)
  {
    while (!client.connect(cartServer, port)) {
      Serial.println("connection failed");
      delay(500);
    }
    return 0;
  }
  else
  {
    return 1;
  }
}

