#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

char ssid[] = "mariocard";  //  your network SSID (name)
//char pass[] = "cartattack";       // your network password

IPAddress cartServer(192, 168, 42, 1); // server always lives at this address
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

char commandBuffer[7];
bool hasSetUpCart = false;

unsigned long keepAlive;

void setup() {

  Serial.begin(115200);
  delay(1000);

  //ESP.eraseConfig();
  //WiFi.setAutoConnect(false);
  //WiFi.disconnect(true);

  // put your setup code here, to run once:
  WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("registering");
    delay(800);
  }
  udp.begin(3000);

  // now register the cart with the server
  char registerCart[] = "register_cart";
  udp.beginPacket(cartServer, 3000);
  udp.write(&registerCart[0], 13);
  udp.endPacket();

  udp.beginPacket(cartServer, 3000);
  udp.write(&registerCart[0], 13);
  udp.endPacket();

  udp.beginPacket(cartServer, 3000);
  udp.write(&registerCart[0], 13);
  udp.endPacket();

  delay(1000);
  
  hasSetUpCart = true;
  keepAlive = millis();
}

void loop() {


  // put your main code here, to run repeatedly:
  if (Serial.available() > 0)
  {
    char c = Serial.read(); // only ever 1 char
    udp.beginPacket(cartServer, 3000);
    char color[7] = "color:";
    color[6] = c;
    udp.write(&color[0]);
    udp.endPacket();
  }

  int cb = udp.parsePacket();
  if (cb > 6)
  {
    // read the packet into the buffer
    int result = udp.read(commandBuffer, 7);
    if (result != -1)
    {
      Serial.print(commandBuffer);
    }
  }

  if(millis() - keepAlive > 5000)
  {

    udp.beginPacket(cartServer, 3000);
    udp.write("keep_alive");
    udp.endPacket();
    
    keepAlive = millis();
  }
}
