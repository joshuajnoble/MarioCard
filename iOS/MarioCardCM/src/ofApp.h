
#pragma once

#include "ofxNetwork.h"
#include "ofMain.h"

#ifdef ANDROID

#include "ofxAndroid.h"
#include "ofxAccelerometer.h"

#else

#include "ofxiOS.h"
#include "ofxCoreMotion.h"
#include "ofxiOSExtras.h"
#include "ofxNetwork.h"
#include "SimpleSprite.h"

#endif

class arcPoint {
public:
    float position;
    ofColor fill;
};


#ifdef ANDROID
class ofApp : public ofxAndroidApp{
#else
class ofApp : public ofxiOSApp {
#endif
    
public:
    
    void setup();
    void update();
    void draw();
    
    
    // these are comm methods for the server
    void disconnect();
    void reconnect();
    void spin();
    
#ifdef ANDROID
    
    void keyPressed(int key);
    void keyReleased(int key);
    void windowResized(int w, int h);
    
    void touchDown(int x, int y, int id);
    void touchMoved(int x, int y, int id);
    void touchUp(int x, int y, int id);
    void touchDoubleTap(int x, int y, int id);
    void touchCancelled(int x, int y, int id);
    
    void pause();
    void stop();
    void resume();
    void reloadTextures();
    void swipe(ofxAndroidSwipeDir swipeDir, int id);
    bool backPressed();
    void okPressed();
    void cancelPressed();
    
#else
    
    ofxCoreMotion coreMotion;
    void touchDown(ofTouchEventArgs & touch);
    void touchMoved(ofTouchEventArgs & touch);
    void touchUp(ofTouchEventArgs & touch);
    void touchDoubleTap(ofTouchEventArgs & touch);
    void touchCancelled(ofTouchEventArgs & touch);
    
    void lostFocus();
    void gotFocus();
    void gotMemoryWarning();
    void deviceOrientationChanged(int newOrientation);
    void exit();
    
#endif
    
    void sendCMMessage();
    void sendNonCMMessage();
    
    int left, right;
    
    ofxUDPManager client;
    
    uint64_t lastSend;
    bool connected;
    float steer;
    
    float speed;
    bool updateFlag;
    
    ofPolyline arc;
    deque<arcPoint> arcPoints;
    
    bool isUsingCM;
    
    ofMesh mesh;
    ofImage carIcon;
    string udpMessage;
    
    bool isConnected;
    bool mouseDown;
    
    SimpleSprite disconnectSprite, reconnectSprite, spinSprite, coremotionSprite;
    
    float keepAliveTimer;
    
};
