#pragma once

#include "ofxiOS.h"
#include "ofxiOSExtras.h"

#include "ofxLibwebsockets.h"
#include "Posts.h"

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
    void addPost();
    
    int left, right;
    ofxLibwebsockets::Client client;
    bool connected;
    float speed;
    bool updateFlag;
    
    ofPolyline arc;
    deque<arcPoint> arcPoints;
    
    ofMesh mesh;
    
    
    // websocket methods
    void onConnect( ofxLibwebsockets::Event& args );
    void onOpen( ofxLibwebsockets::Event& args );
    void onClose( ofxLibwebsockets::Event& args );
    void onIdle( ofxLibwebsockets::Event& args );
    void onMessage( ofxLibwebsockets::Event& args );
    void onBroadcast( ofxLibwebsockets::Event& args );

};


