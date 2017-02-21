#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

char ssid[] = "mariocard";  //  your network SSID (name)
//char pass[] = "cartattack";       // your network password

IPAddress cartServer(192, 168, 42, 1); // server always lives at this address
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

//#define DEBUGGING 

char colorData = ' ';
char *commandBuffer;
char *sendBuffer;
char spin[7] = {'d', 'o', '_', 's', 'p', 'i', 'n'};
char *spinStr = "do_spin";
bool hasSetUpCart = false;
int recvTotal = 0;

unsigned long keepAlive;


void setup() {

  commandBuffer = new char[32];
  sendBuffer = new char[7];

  Serial.begin(115200);
  delay(1000);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    //Serial.println(" can't register ");
    delay(500);
  }
  udp.begin(3000);
 
  // now register the cart with the server
  char registerCart[] = "register_cart";
  udp.beginPacket(cartServer, 3000);
  udp.write(&registerCart[0], 13);
  udp.endPacket();

  hasSetUpCart = true;
  keepAlive = millis();

  // reset pin
  pinMode(12, INPUT);
}

void loop() {

  if(digitalRead(12) == LOW)
  {
    if(hasSetUpCart)
    {
      hasSetUpCart = false;
      udp.beginPacket(cartServer, 3000);
      udp.write("disconnect_cart", 15);
      udp.endPacket();
    }
    else
    {
      hasSetUpCart = true;
      char registerCart[] = "register_cart";
      udp.beginPacket(cartServer, 3000);
      udp.write(&registerCart[0], 13);
      udp.endPacket();
    }
    delay(500); // debounce by delay
  }

  // put your main code here, to run repeatedly:
  if (Serial.available() > 0)
  {

#ifdef DEBUGGING
    
      while(Serial.available()) {
        Serial.read(); 
      }
      Serial.println(recvTotal);

#else

    while(Serial.available())
    {
      if(Serial.readBytesUntil(';', &colorData, 1))
      {
        if(isalpha(colorData))
        {
          udp.beginPacket(cartServer, 3000);
          char color[7] = "color:";
          color[6] = colorData;
          udp.write(&color[0]);
          udp.endPacket();
        }
      }
    }

#endif
   
  }

  int cb = udp.parsePacket();
  if (cb > 6)
  {
    // read the packet into the buffer
    int result = udp.read(commandBuffer, cb);

    recvTotal += cb;

    if (result == -1)
    {
      return; // something went wrong?
    }

    commandBuffer[cb] = '\0';

    int i = 0;
    if (strstr(commandBuffer, spinStr))
    {
      memcpy(&sendBuffer[0], &spin[0], 7);
    }
    else
    {
      int charsRead = 0;
      while (charsRead < cb)
      {
        if (isdigit( (int) commandBuffer[charsRead]) || commandBuffer[charsRead] == ':') {
          sendBuffer[i] = commandBuffer[charsRead];
          i++;
        }
        charsRead++;
      }
    }
    for ( int i = 0; i < 7; i++ ) {
      Serial.print(sendBuffer[i]);
    }
    Serial.print(';'); // delimiter
    memset(&sendBuffer[0], 0, 7);
  }
  udp.flush();

  if (millis() - keepAlive > 5000)
  {

    udp.beginPacket(cartServer, 3000);
    udp.write("keep_alive");
    udp.endPacket();

    keepAlive = millis();
  }
}
