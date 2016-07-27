#include "ofApp.h"

const int frequency = 10;


//--------------------------------------------------------------
void ofApp::setup(){	
    connected = false;
    
    client.connect("echo.websocket.org"); // would be great to do this with mDNS
    client.addListener(this);
    
    switch (ofGetOrientation()) {
        case OF_ORIENTATION_DEFAULT:
            ofSetOrientation(OF_ORIENTATION_90_LEFT);
            break;
        case OF_ORIENTATION_180:
            ofSetOrientation(OF_ORIENTATION_90_LEFT);
            break;
        case OF_ORIENTATION_90_LEFT:
            ofSetOrientation(OF_ORIENTATION_90_LEFT);
            break;
        case OF_ORIENTATION_90_RIGHT:
            ofSetOrientation(OF_ORIENTATION_90_RIGHT);
            break;
        default:
            break;
    }
    
}

//--------------------------------------------------------------
void ofApp::update(){
    if(ofGetElapsedTimeMillis() % 100 == 0) // 10hz refresh?
    {
        stringstream message;
        message << left << ":" << right;
        client.send(message.str());
        updateFlag = false;
    }
    
    speed = ofMap( left + right, -254, 254, 0.03, -0.03);
    cout << speed << endl;
    
    for ( int i = 0; i < posts.size(); i++ )
    {
        posts.at(i).update(4, speed);
        if (posts.at(i).remove == true)
        {
            posts.erase(posts.begin() + i);
        }
    }
    
    if(ofGetFrameNum() % frequency == 0)
    {
        addPost();
    }
    
}

//--------------------------------------------------------------
void ofApp::draw(){
	
    
    ofBackground(0, 0, 0);
    
    ofSetColor(0, 255, 0);
    
    int w = ofGetWidth();
    int h = ofGetHeight();
    
    ofDrawRectangle(0, h/2, w/2, ofMap(left, -127, 127, -h/2, h/2));
    ofDrawRectangle(w/2, h/2, w/2, ofMap(right, -127, 127, -h/2, h/2));
    
    ofSetColor(255, 255, 255);
    
    for ( int i = 0; i < posts.size(); i++ )
    {
        posts.at(i).draw();
    }

    
}

//--------------------------------------------------------------
void ofApp::exit(){

}

//--------------------------------------------------------------
void ofApp::touchDown(ofTouchEventArgs & touch){
    if(touch.x < ofGetWidth()/2)
    {
        left = ofMap(touch.y, 0, ofGetHeight(), -127, 127);
    }
    else
    {
        right = ofMap(touch.y, 0, ofGetHeight(), -127, 127);
    }
    updateFlag = true;
}

//--------------------------------------------------------------
void ofApp::touchMoved(ofTouchEventArgs & touch){
    if(touch.x < ofGetWidth()/2)
    {
        left = ofMap(touch.y, 0, ofGetHeight(), -127, 127);
    }
    else
    {
        right = ofMap(touch.y, 0, ofGetHeight(), -127, 127);
    }
    
    if(ofGetFrameNum() % frequency == 0 )
    {
        addPost();
    }
    
    updateFlag = true;
}

//--------------------------------------------------------------
void ofApp::touchUp(ofTouchEventArgs & touch){

}

//--------------------------------------------------------------
void ofApp::touchDoubleTap(ofTouchEventArgs & touch){

}

//--------------------------------------------------------------
void ofApp::touchCancelled(ofTouchEventArgs & touch){
    
}

//--------------------------------------------------------------
void ofApp::lostFocus(){

}

//--------------------------------------------------------------
void ofApp::gotFocus(){

}

//--------------------------------------------------------------
void ofApp::gotMemoryWarning(){

}

//--------------------------------------------------------------
void ofApp::deviceOrientationChanged(int newOrientation){

}

void ofApp::addPost()
{
    float steer = ofMap(left - right, -254, 254, 0, ofGetWidth());
    if(speed < 0)
    {
        Posts p(steer, ofGetHeight() - 20, ofGetWidth(), 0, 0.0);
        posts.push_back(p);
    }
    else
    {
        Posts p(steer, 20, ofGetWidth(), ofGetHeight(), 0.0);
        posts.push_back(p);
    }
}

// websockets methods
//--------------------------------------------------------------
void ofApp::onConnect( ofxLibwebsockets::Event& args ){
    cout<<"on connected"<<endl;
}

//--------------------------------------------------------------
void ofApp::onOpen( ofxLibwebsockets::Event& args ){
    cout<<"on open"<<endl;
}

//--------------------------------------------------------------
void ofApp::onClose( ofxLibwebsockets::Event& args ){
    cout<<"on close"<<endl;
}

//--------------------------------------------------------------
void ofApp::onIdle( ofxLibwebsockets::Event& args ){
//    cout<<"on idle"<<endl;
}

//--------------------------------------------------------------
void ofApp::onMessage( ofxLibwebsockets::Event& args ){
//    cout<<"got message "<<args.message<<endl;
}

//--------------------------------------------------------------
void ofApp::onBroadcast( ofxLibwebsockets::Event& args ){
    cout<<"got broadcast "<<args.message<<endl;
}
