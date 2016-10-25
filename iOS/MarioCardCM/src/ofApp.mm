#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    isConnected = false;
    
    disconnectSprite.setText("disconnect");
    disconnectSprite.setPosition(ofVec2f(20, 30));
    disconnectSprite.setScale(ofVec2f(100, 30));
    
    reconnectSprite.setText("reconnect");
    reconnectSprite.setPosition(ofVec2f(20, 65));
    reconnectSprite.setScale(ofVec2f(100, 30));
    
    spinSprite.setText("spin");
    spinSprite.setPosition(ofVec2f(20, 100));
    spinSprite.setScale(ofVec2f(100, 30));
    
    reconnectSprite.setText("reconnect");
    reconnectSprite.setPosition(ofVec2f(20, 65));
    reconnectSprite.setScale(ofVec2f(100, 30));
    
    coreMotion.setupMagnetometer();
    coreMotion.setupGyroscope();
    coreMotion.setupAccelerometer();
    coreMotion.setupAttitude(CMAttitudeReferenceFrameXMagneticNorthZVertical);
    
    connected = false;
    
    string regStr = "register_control";
    
#ifdef UDP
    
    // make a udp connection that we can stream data to
    client.Create();
    client.Connect("192.168.42.1",3000);
    client.SetNonBlocking(true);
    
    // now register
    client.Send(regStr.c_str(), regStr.size());
    
#else
    
    client.setup("192.168.42.1",3000);
    client.send(regStr);
    client.receiveRawBytes(&serverId, 1 );
    
#endif
    
    isConnected = true;
    
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
    
    int count = 0;
    for( float i = 0; i < 1.0; i+=0.05 )
    {
        arcPoint p;
        p.position = i;
        
        if(count % 2)
        {
            p.fill.set(122, 122, 122);
        }
        else
        {
            p.fill.set(255, 255, 255);
        }
        
        arcPoints.push_back(p);
        count++;
    }
    
    left = 0;
    right = 0;
    
    carIcon.load("car2.png");
    coreMotion.resetAttitude();
    
    mouseDown = false;
    keepAliveTimer = ofGetElapsedTimef();
}

//--------------------------------------------------------------
void ofApp::update()
{
    
    if(!mouseDown)
    {
        speed *= 0.99;
    }
    
    coreMotion.update();
    
    float pitch = coreMotion.getPitch();
    pitch = ofClamp(pitch, -1.4, 1.4);
    
    left = ofMap(pitch, -1.4, 1.4, -127, 127);
    right = ofMap(pitch, -1.4, 1.4, 127, -127);
    
    // figure out speed and direction from L/R tread
    //speed = ofMap( left + right, -254, 254, 0.03, -0.03);
    float steer = ofMap(left - right, -254, 254, 0, ofGetWidth());
    
    if(isConnected)
    {
    
        // send a message over our socket about our speed & position
        if(ofGetFrameNum() % 5 == 0) // 10hz refresh?
        {
            
            float trueSteer = left - right;
            
            int leftTread = 90 * speed;
            int rightTread = 90 * speed;
            
            const int steerValue = 40;
            
            if(speed > 0.0)
            {
            
                // all the way to the left will be -254, so slow left tread to we steer to left
                leftTread -= ofMap(trueSteer, -254, 254, -steerValue, steerValue);
                // all the way to the right will be 254, so slow right tread to we steer to right
                rightTread -= ofMap(trueSteer, -254, 254, steerValue, -steerValue);
            }
            else
            {
                // all the way to the left will be -254, so slow left tread to we steer to left
                leftTread += ofMap(trueSteer, -254, 254, -steerValue, steerValue);
                // all the way to the right will be 254, so slow right tread to we steer to right
                rightTread += ofMap(trueSteer, -254, 254, steerValue, -steerValue);
            }
            
            // Kart is just listening for 0-255 where 127 = stopped, 0 = full backwards, 255 = full forwards
            stringstream message;
            message << "speed:" << serverId << ":" << min(255, max(0, (leftTread + 127))) << ":" << min(255, max(0, (rightTread + 127)));
            //cout << message.str() << endl;
            udpMessage = message.str();
            
#ifdef UDP
            client.Send(message.str().c_str(), message.str().size());
#else
            client.send(message.str());
#endif
        }
        
//        if(ofGetElapsedTimef() - keepAliveTimer > 4.0)
//        {
//            string keepAlive = "keep_alive_control";
//            client.Send(keepAlive.c_str(), keepAlive.size());
//            keepAliveTimer = ofGetElapsedTimef();
//        }
        
    }
    
    // make a brand new arc using our steer
    arc.clear();
    arc.addVertex(steer, 50);
    arc.bezierTo( ofGetWidth()/2 + ((ofGetWidth()/2 - steer) / 4), ofGetHeight()*0.5, ofGetWidth()/2, ofGetHeight()*0.75, ofGetWidth()/2, ofGetHeight(), 100);
    arc.addVertex(ofGetWidth()/2, ofGetHeight());
    
    // accelerate the arc points nicely for point-along-arc calculations
    for( int i = 0; i < arcPoints.size(); i++ )
    {
        arcPoints.at(i).position += speed * 0.03;
        arcPoints.at(i).position = roundf(arcPoints.at(i).position * 60) / 60.0;
    }
    
    // if the point is off either end of the arc, delete it and add a new one at the beginning or end
    // of the arc so we're continuous
    for( int i = 0; i < arcPoints.size(); i++ )
    {
        
        if(arcPoints.at(i).position < 0.0)
        {
            
            //cout << (arcPoints.begin() + i)->fill << endl;
            arcPoints.erase(arcPoints.begin() + i);
            arcPoint p;
            p.position = 1.0;
            if(arcPoints.at(arcPoints.size()-1).fill.r == 122)
            {
                p.fill.set(255, 255, 255);
            }
            else
            {
                p.fill.set(122, 122, 122);
            }
            arcPoints.push_back(p);
        }
        
        if(arcPoints.at(i).position > 1.0)
        {
            //cout << (arcPoints.begin() + i)->fill << endl;
            arcPoints.erase(arcPoints.begin() + i);
            arcPoint p;
            p.position = 0.0;
            if(arcPoints.at(0).fill.r == 122)
            {
                p.fill.set(255, 255, 255, 255);
            }
            else
            {
                p.fill.set(122, 122, 122, 255);
            }
            arcPoints.push_front(p);
        }
    }

}

//--------------------------------------------------------------
void ofApp::draw(){
    // clear
    ofBackground(0, 0, 0);
    
    ofEnableAlphaBlending();
    ofSetColor(0, 255, 0);
    
    ofPushMatrix();
    ofTranslate(0, 20);
    ofRotateX(50);
    
    ofSetLineWidth(10);
    
    int scale = 60;
    
    // use our arc segments to draw boxes that are our 'road'
    for( int i = 1; i < arcPoints.size(); i++ )
    {
        
        float position = arcPoints.at(i).position;
        float prevPosition = roundf(arcPoints.at(i-1).position * 60) / 60.0;
        
        ofSetColor(arcPoints.at(i).fill);
        ofBeginShape();
        
        ofVertex(arc.getPointAtPercent(prevPosition).x - scale, arc.getPointAtPercent(prevPosition).y);
        ofVertex(arc.getPointAtPercent(prevPosition).x + scale, arc.getPointAtPercent(prevPosition).y);
        ofVertex(arc.getPointAtPercent(position).x + scale, arc.getPointAtPercent(position).y);
        ofVertex(arc.getPointAtPercent(position).x - scale, arc.getPointAtPercent(position).y);
        
        ofEndShape();
        
    }
    
    ofPopMatrix();
    
    ofSetColor(255, 255, 255);
    carIcon.draw(ofGetWidth()/2 - (carIcon.getWidth()/8), ofGetHeight() - (carIcon.getHeight()/4) - 20, carIcon.getWidth()/4, carIcon.getHeight()/4);
    
//    stringstream ss;
//    ss <<  left << " " << right;
//    
//    ofDrawBitmapString(ss.str(), 30, 30);
//    ofDrawBitmapString(udpMessage, 30, 50);

    ofPushMatrix();
    ofTranslate(reconnectSprite.getBounds().x, reconnectSprite.getBounds().y);
    reconnectSprite.draw();
    ofPopMatrix();
    
    ofPushMatrix();
    ofTranslate(disconnectSprite.getBounds().x, disconnectSprite.getBounds().y);
    disconnectSprite.draw();
    ofPopMatrix();
    
    ofPushMatrix();
    ofTranslate(spinSprite.getBounds().x, spinSprite.getBounds().y);
    spinSprite.draw();
    ofPopMatrix();
    
    ofSetColor(255, 0, 0);
    ofDrawRectangle(ofGetWidth() - 40, (ofGetHeight()/2), 42, speed * -160);
    ofSetColor(255, 255, 255);
    
    
}

//--------------------------------------------------------------
void ofApp::exit(){

}

//--------------------------------------------------------------
void ofApp::touchDown(ofTouchEventArgs & touch){
    
    if( disconnectSprite.hitTest(touch))
    {
        disconnect();
    }
    else if( reconnectSprite.hitTest(touch))
    {
        reconnect();
    }
    else
    {
        speed = ofMap(touch.y, 0, ofGetHeight(), 1, -1);
        mouseDown = true;
    }
    
}

//--------------------------------------------------------------
void ofApp::touchMoved(ofTouchEventArgs & touch){
    // used to be 0.03 to -0.03
    if( disconnectSprite.hitTest(touch))
    {
        //disconnect();
    }
    else if( reconnectSprite.hitTest(touch))
    {
        //reconnect();
    }
    else
    {

        speed = ofMap(touch.y, 0, ofGetHeight(), 1, -1);
    }
}

//--------------------------------------------------------------
void ofApp::touchUp(ofTouchEventArgs & touch){
    mouseDown = false;
}

//--------------------------------------------------------------
void ofApp::touchDoubleTap(ofTouchEventArgs & touch){

}

//--------------------------------------------------------------
void ofApp::touchCancelled(ofTouchEventArgs & touch){
    
}

void ofApp::spin()
{
    cout << " spin " << endl;
    if(isConnected)
    {
        string spinStr = "spin_control";
        
#ifdef UDP
        client.Send(spinStr.c_str(), spinStr.size());
#else
        client.send(spinStr);
#endif
    }
}

void ofApp::disconnect()
{
    cout << " disconnect " << endl;
    if(isConnected)
    {
        isConnected = false;
        string disconnectStr = "disconnect_control";
        
#ifdef UDP
        client.Send(disconnectStr.c_str(), disconnectStr.size());
        client.Close();
        
#else
        client.send(disconnectStr);
        client.close();
#endif
    }
}

void ofApp::reconnect()
{
    cout << " reconnect " << endl;
#ifdef UDP
    client.Create();
    client.Connect("192.168.42.1",3000);
    client.SetNonBlocking(true);
    
    // now register
    string regStr = "register_control";
    client.Send("register_control", regStr.size());
    
#else
    
    client.setup("192.168.42.1",3000);
    
    // now register
    string regStr = "register_control";
    client.send("register_control");
    client.receiveRawBytes(&serverId, 1 );
    
#endif
    
    isConnected = true;
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
