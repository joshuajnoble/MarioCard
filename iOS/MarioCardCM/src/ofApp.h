#pragma once

#include "ofxiOS.h"
#include "ofxCoreMotion.h"
#include "ofxiOSExtras.h"
#include "ofxNetwork.h"
#include "SimpleSprite.h"

class arcPoint {
public:
    float position;
    ofColor fill;
};

class ofApp : public ofxiOSApp {
	
    public:
        void setup();
        void update();
        void draw();
        void exit();
	
        void touchDown(ofTouchEventArgs & touch);
        void touchMoved(ofTouchEventArgs & touch);
        void touchUp(ofTouchEventArgs & touch);
        void touchDoubleTap(ofTouchEventArgs & touch);
        void touchCancelled(ofTouchEventArgs & touch);

        void lostFocus();
        void gotFocus();
        void gotMemoryWarning();
        void deviceOrientationChanged(int newOrientation);
    
    // these are comm methods for the server
    void disconnect();
    void reconnect();
    void spin();
    
        ofxCoreMotion coreMotion;
    
    int left, right;
    ofxUDPManager client;
    
    bool connected;
    
    float speed;
    bool updateFlag;
    
    ofPolyline arc;
    deque<arcPoint> arcPoints;
    
    ofMesh mesh;
    ofImage carIcon;
    string udpMessage;
    
    bool isConnected;
    bool mouseDown;
    
    SimpleSprite disconnectSprite, reconnectSprite, spinSprite;
};


