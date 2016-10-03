#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

char ssid[] = "mariocard";  //  your network SSID (name)
//char pass[] = "cartattack";       // your network password

IPAddress cartServer(192, 168, 1, 1); // time.nist.gov NTP server
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

char commandBuffer[7];

void setup() {

  Serial.begin(115200);
  
  // put your setup code here, to run once:
  WiFi.begin(ssid);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  udp.begin(3000);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available() > 0)
  {

    char c = Serial.read(); // only ever 1 char
    udp.beginPacket(cartServer, 3000);
    udp.write(c);
    udp.endPacket();
  }

  int cb = udp.parsePacket();
  if (!cb) {
    udp.read(commandBuffer, 7); // read the packet into the buffer
    Serial.println(commandBuffer);
  }
}
