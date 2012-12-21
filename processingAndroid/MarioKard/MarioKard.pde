/**
 * <p>Ketai Sensor Library for Android: http://KetaiProject.org</p>
 *
 * <p>KetaiSensor Features:
 * <ul>
 * <li>handles incoming Sensor Events</li>
 * <li>Includes Accelerometer, Magnetometer, Gyroscope, GPS, Light, Proximity</li>
 * <li>Use KetaiNFC for Near Field Communication</li>
 * </ul>
 * <p>Updated: 2012-03-10 Daniel Sauter/j.duran</p>
 */

import ketai.sensors.*;
import android.hardware.SensorManager;

import android.text.TextUtils;
import android.util.Base64;
import android.util.Log;
import org.apache.http.*;
import org.apache.http.client.HttpResponseException;
import org.apache.http.message.BasicLineParser;
import org.apache.http.message.BasicNameValuePair;

import javax.net.SocketFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLException;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import java.io.EOFException;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.URI;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.util.List;

KetaiSensor sensor;
PVector gyro, accelerometer;
float light, proximity;
int lastMillis;

SensorFuse sensorFuse;
WebSocketClient client;

List<BasicNameValuePair> extraHeaders = Arrays.asList(
    new BasicNameValuePair("Cookie", "session=abcd")
);


final String serverURI = "10.118.73.111:3000";

void setup()
{
  
  sensor = new KetaiSensor(this);
  sensor.start();
  sensor.list();
  accelerometer = new PVector(0, 0, 0);
  gyro = new PVector(0, 0, 0);
  orientation(LANDSCAPE);
  textAlign(CENTER, CENTER);
  textSize(28);
  
  sensorFuse = new SensorFuse();
  client = new WebSocketClient(URI.create("wss://irccloud.com"), new WebSocketClient.Handler() {
    @Override
    public void onConnect() {
        //Log.d(TAG, "Connected!");
    }

    @Override
    public void onMessage(String message) {
        //Log.d(TAG, String.format("Got string message! %s", message));
    }

    @Override
    public void onMessage(byte[] data) {
        //Log.d(TAG, String.format("Got binary message! %s", toHexString(data));
    }

    @Override
    public void onDisconnect(int code, String reason) {
        //Log.d(TAG, String.format("Disconnected! Code: %d Reason: %s", code, reason));
    }

    @Override
    public void onError(Exception error) {
        //Log.e(TAG, "Error!", error);
    }
}, extraHeaders);

client.connect();
  
}

void draw()
{
  background(78, 93, 75);
  text(" acc :" + "\n" 
    + "x: " + nfp(accelerometer.x, 1, 2) + "\n" 
    + "y: " + nfp(accelerometer.y, 1, 2) + "\n" 
    + "z: " + nfp(accelerometer.z, 1, 2) + "\n"
    + "gyro :" + "\n" 
    + "x: " + nfp(gyro.x, 1, 2) + "\n"
    + "y: " + nfp(gyro.y, 1, 2) + "\n" 
    + "z: " + nfp(gyro.z, 1, 2) + "\n"
    , 20, 0, width, height);
    
    
   if( millis() - lastMillis > 100) 
   {
     sensorFuse.runFusion();
     lastMillis = millis();
   }
    
}

void onAccelerometerEvent(float x, float y, float z, long time, int accuracy)
{
  accelerometer.set(x, y, z);
  
  // add in gyro:
  float fusionTiming = millis() / 1000.0;
  float[] xyz = {x, y, z};
  sensorFuse.gyroFunction( xyz, fusionTiming);
  calculateAccMagOrientation();  
}

void onGyroscopeEvent(float x, float y, float z)
{
  gyro.set(x, y, z);
  
  // now do fusion
  float fusionTiming = millis() / 1000.0;
  float[] xyz = {x, y, z};
  sensorFuse.gyroFunction( xyz, fusionTiming);
  calculateAccMagOrientation();  
}

public void mousePressed() { 
  if (sensor.isStarted())
    sensor.stop(); 
  else
    sensor.start(); 
  println("KetaiSensor isStarted: " + sensor.isStarted());
}

