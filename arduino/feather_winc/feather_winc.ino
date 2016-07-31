// SPI
#include <SPI.h>
#include <Adafruit_WINC1500.h>
#include <Adafruit_WINC1500Udp.h>

// color sensor 
#include <Wire.h>
#include "Adafruit_TCS34725.h"

/* Initialise with specific int time and gain values */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

//////////////////////////////////////////////////////////////////
// Motor
//////////////////////////////////////////////////////////////////

#define PWMA 14
#define PWMB 15
#define AIN1 1
#define AIN2 0
#define BIN1 3
#define BIN2 4
#define STBY 2
#define MOTOR_A 0
#define MOTOR_B 1
#define FORWARD 1
#define REVERSE 0
#define RIGHT 1
#define LEFT 0


//////////////////////////////////////////////////////////////////
// WINC wifi
//////////////////////////////////////////////////////////////////

#define WINC_CS   8
#define WINC_IRQ  7
#define WINC_RST  4
#define WINC_EN   2     // or, tie EN to VCC and comment this out

// Setup the WINC1500 connection with the pins above and the default hardware SPI.
Adafruit_WINC1500 WiFi(WINC_CS, WINC_IRQ, WINC_RST);

int status = WL_IDLE_STATUS;
char ssid[] = "mynetwork";  //  your network SSID (name)
char pass[] = "mypassword";       // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)

unsigned int localPort = 2390;      // local port to listen for UDP packets

IPAddress server(192, 168, 1, 1); // this will be our server

uint16_t r, g, b, c;
uint16_t colorTemp;

// here's our packet out:
const int ID = 1;
const int OUT_PACKET = 2;
const int PACKET_SIZE = 24; // our packet is as follows: LEFT:RIGHT

long lastColorSensorRead = 0;

byte outBuffer[OUT_PACKET];
byte inBuffer[PACKET_SIZE];

// A UDP instance to let us send and receive packets over UDP
Adafruit_WINC1500UDP Udp;

const uint16_t colors[6] = { 0, 50, 100, 150, 200, 250 };


void setup()
{


  //////////////////////////////////////////////////////////////////
  // set up motor

  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT);

  motor_standby(false);        //Must set STBY pin to HIGH in order to move


  pinMode(WINC_EN, OUTPUT);
  digitalWrite(WINC_EN, HIGH);

#ifdef USE_SERIAL
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
#endif

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
#ifdef USE_SERIAL
    Serial.println("WiFi shield not present");
#endif
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
#ifdef USE_SERIAL
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
#endif
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

#ifdef USE_SERIAL
  Serial.println("Connected to wifi");
  printWifiStatus();

  Serial.println("\nStarting connection to server...");
#endif

  Udp.begin(localPort);
}

void loop()
{

  if ( millis() - lastColorSensorRead > 100 ) // read color sensor every 100ms
  {

    lastColorSensorRead = millis();
    tcs.getRawData(&r, &g, &b, &c);
    uint16_t newColorTemp = tcs.calculateColorTemperature(r, g, b);
    if ( abs(newColorTemp - colorTemp) > 2 ) // if sensor is different
    {

      colorTemp = newColorTemp;
      int colorIndex = -1;

      // what color do we have?
      for( int i = 0; i < 6; i++ )
      {
        if( abs(colors[i] - colorTemp) < 5 )
        {
          colorIndex = i;
        }
      }

      // send the color and our ID
      if(colorIndex != -1)
      {
        Udp.beginPacket(server, 4444);
        outBuffer[0] = ID;
        outBuffer[1] = colorIndex;
        
        Udp.write(outBuffer, OUT_PACKET);
        Udp.endPacket();
      }
      
    }
  }

  // is a reply available?
  if ( Udp.parsePacket() ) {
    // We've received a packet, read the data from it
    Udp.read(inBuffer, PACKET_SIZE); // read the packet into the buffer

    parseSpeed(inBuffer[0], 3, &left);
    parseSpeed(inBuffer[4], 3, &right);

    if (left > 127) {
      motor_control( MOTOR_A, FORWARD, left - 127);
    } else {
      motor_control( MOTOR_A, REVERSE, 127 - left);
    }

    if (right > 127) {
      motor_control( MOTOR_B, FORWARD, right - 127);
    } else {
      motor_control( MOTOR_B, REVERSE, 127 - right);
    }

  }
  sendNTPpacket(server); // send an NTP packet to a time server

  delay(20);
}

void parseSpeed ( byte *inBuffer, int length, int *side )
{

  char t[3] = {0};
  
  int i = 0;
  while( inBuffer && i < length )
  {
    char c = (char) inBuf[i];
      if(c != '0'){
        leading = false;
      }
      if(!leading) {
        t[i] = c;
      }
    i++; // keep track of where we are
    ++inBuffer; // increment the buffer
  }

  *side = (int) strtol(&t[0], NULL, 0);
  
}


//////////////////////////////////////////////////////////////////
// Motor
//////////////////////////////////////////////////////////////////

//Turns off the outputs of the Motor Driver when true
void motor_standby(char standby)
{
  if (standby == true) digitalWrite(STBY, LOW);
  else digitalWrite(STBY, HIGH);
}

void motor_control(char motor, char direction, int speed)
{
  if (motor == MOTOR_A)
  {
    if (direction == FORWARD)
    {
      digitalWrite(AIN1, HIGH);
      digitalWrite(AIN2, LOW);
    }
    else
    {
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, HIGH);
    }
    analogWrite(PWMA, speed);
  }
  else
  {
    if (direction == FORWARD)  //Notice how the direction is reversed for motor_B
    { //This is because they are placed on opposite sides so
      digitalWrite(BIN1, LOW); //to go FORWARD, motor_A spins CW and motor_B spins CCW
      digitalWrite(BIN2, HIGH);
    }
    else
    {
      digitalWrite(BIN1, HIGH);
      digitalWrite(BIN2, LOW);
    }
    analogWrite(PWMB, speed);
  }
}
