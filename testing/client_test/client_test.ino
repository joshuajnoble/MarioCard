#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

char ssid[] = "mariocard";  //  your network SSID (name)

IPAddress cartServer(192, 168, 42, 1); // server always lives at this address
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

char lorem[] = "Contrary to popular belief, Lorem Ipsum is not simply random tex";

void setup() {

  Serial.begin(115200);
  delay(1000);

  // put your setup code here, to run once:
  WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("registering");
    delay(500);
  }
  udp.begin(3000);
}

void loop() {


  // put your main code here, to run repeatedly:
  for( int i = 0; i < 128; i++)
  {
    udp.beginPacket(cartServer, 3000);
    udp.write(&lorem[0], 64);
    udp.endPacket();
  }

  udp.beginPacket(cartServer, 3000);
  udp.write("1", 1);
  udp.endPacket();
  
}
