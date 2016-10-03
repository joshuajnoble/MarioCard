

// color sensor 
#include <Wire.h>
#include "Adafruit_TCS34725.h"

//////////////////////////////////////////////////////////////////
// Motor
//////////////////////////////////////////////////////////////////

#define PWML 14
#define PWMR 15
#define RBACKWARD 9
#define RFORWARD 10
#define LFORWARD 12
#define LBACKWARD 11
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


typedef struct {
    double r;       // percent
    double g;       // percent
    double b;       // percent
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // percent
    double v;       // percent
} hsv;


/* Initialise with specific int time and gain values */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

const int MESSAGE_SIZE = 7;
float hues[] = { 351.60, 194.81, 66.67, 24.11, 156.00 };
String colorNames[] = {"purple", "blue", "yellow", "orange", "green" };

bool lastReqResponded = false;

char inBuf[9];
char outBuf[128];
uint8_t outBufInd = 0;

uint16_t left = 127, right = 127;

void setup()
{

  Serial.begin(115200);
  
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
        Serial1.println(colorNames[i]);
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////////////////////////
  // esp comm loop
  ////////////////////////////////////////////////////////////////////////////////////////////////
  
  if (Serial1.available() > 0) {
      
    // Get next command from Serial (add 1 for final 0)
    char input[MESSAGE_SIZE + 1];
    byte size = Serial.readBytes(input, MESSAGE_SIZE);
    // Add the final 0 to end the C string
    input[size] = 0;

    *separator = 0;
    int servoId = atoi(command);
    ++separator;
    int position = atoi(separator);
  
    char* separator = strchr(input, ':');
    if (separator != 0)
    {
        // Actually split the string in 2: replace ':' with 0
       

        // Do something with servoId and position
    }
  
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

void setMotors(uint8_t left, uint8_t right)
{ 

  // set motor intensity (pwm)
  
  if (left == 0) {
    digitalWrite(PWML, LOW);
  } else {
    analogWrite(PWML, abs(left - 127));
  }

  if (right == 0) {
    digitalWrite(PWMR, LOW);
  } else {
    analogWrite(PWMR, abs(right - 127));
  }

  // set h-bridge direction (bool)
  
  if (left > 0) {
    digitalWrite(LFORWARD, HIGH);
    digitalWrite(LBACKWARD, LOW);
  } else {
    digitalWrite(LFORWARD, LOW);
    digitalWrite(LBACKWARD, HIGH);
  }

  // RIGHT FORWARD
  if (right < 0) {
    digitalWrite(RBACKWARD, LOW);
    digitalWrite(RFORWARD, HIGH);
  } 
  // RIGHT BACKWARD
  else {
    digitalWrite(RBACKWARD, HIGH);
    digitalWrite(RFORWARD, LOW);
  }
  
}



hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0              
            // s = 0, v is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}
