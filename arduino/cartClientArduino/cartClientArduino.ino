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
// Color Sensor
//////////////////////////////////////////////////////////////////

// color sensor 
#include <Wire.h>
#include "Adafruit_TCS34725.h"

/* Initialise with specific int time and gain values */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

IPAddress server(192, 168, 1, 1); // this will be our server


float hues[] = { 351.60, 194.81, 66.67, 24.11, 156.00 };
String colorNames[] = {"purple", "blue", "yellow", "orange", "green" };

bool lastReqResponded = false;

char inBuf[9];
char outBuf[128];
uint8_t outBufInd = 0;

uint16_t left = 127, right = 127;

void setup()
{
  // set up the motor driver
  pinMode(PWMA,OUTPUT);
  pinMode(AIN1,OUTPUT);
  pinMode(AIN2,OUTPUT);
  pinMode(PWMB,OUTPUT);
  pinMode(BIN1,OUTPUT);
  pinMode(BIN2,OUTPUT);
  pinMode(STBY,OUTPUT);
  motor_standby(false);        //Must set STBY pin to HIGH in order to move
  

  
}


void loop()
{

  Serial.begin(115200);

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // colors
  ////////////////////////////////////////////////////////////////////////////////////////////////
  uint16_t clear, red, green, blue;
  delay(3);
  tcs.getRawData(&red, &green, &blue, &clear);


  uint32_t sum = clear;
  float r, g, b;
  r = red; r /= sum;
  g = green; g /= sum;
  b = blue; b /= sum;
  r *= 256; g *= 256; b *= 256;
  //    Serial.print("\t");
  //    Serial.print((int)r, HEX); Serial.print((int)g, HEX); Serial.print((int)b, HEX);

  rgb inColor = { (int) r, (int) g, (int) b };

  hsv outColor = rgb2hsv(inColor);

  if ( clear > 100 && clear < 900 && outColor.s > 0.25)
  {
    for ( int i = 0; i < 5; i++)
    {
      if (fabs(hues[i] - outColor.h) < 20.0)
      {
        Serial.println(colorNames[i]);
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // wifi loop
  ////////////////////////////////////////////////////////////////////////////////////////////////
  
  if (Serial.available() > 0) {
      
    String t = Serial.readUntil("\n");
    
    left = strtol(&t[0], NULL, 0);
    right = (int) strtol(&t[4], NULL, 0);
    
    if(left > 127) {
      motor_control( MOTOR_A, FORWARD, (left - 127));
    } else {
      motor_control( MOTOR_A, REVERSE, (127 - left));
    }
    
    if(right > 127) {
      motor_control( MOTOR_B, FORWARD, (right - 127));
    } else {
      motor_control( MOTOR_B, REVERSE, (127 - right));
    }
  }

}

//////////////////////////////////////////////////////////////////
// Motor
//////////////////////////////////////////////////////////////////

//Turns off the outputs of the Motor Driver when true
void motor_standby(char standby)
{
  if (standby == true) digitalWrite(STBY,LOW);
  else digitalWrite(STBY,HIGH);
}

void motor_control(char motor, char direction, int speed)
{ 
  if (motor == MOTOR_A)
  {
    if (direction == FORWARD)
    {
      digitalWrite(AIN1,HIGH);
      digitalWrite(AIN2,LOW);
    } 
    else 
    {
      digitalWrite(AIN1,LOW);
      digitalWrite(AIN2,HIGH);
    }
    analogWrite(PWMA,speed);
  }
  else
  {
    if (direction == FORWARD)  //Notice how the direction is reversed for motor_B
    {                          //This is because they are placed on opposite sides so
      digitalWrite(BIN1,LOW);  //to go FORWARD, motor_A spins CW and motor_B spins CCW
      digitalWrite(BIN2,HIGH);
    }
    else
    {
      digitalWrite(BIN1,HIGH);
      digitalWrite(BIN2,LOW);
    }
    analogWrite(PWMB,speed);
  }
}

