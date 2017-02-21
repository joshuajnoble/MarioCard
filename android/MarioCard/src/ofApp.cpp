
#include "ofApp.h"

#ifdef ANDROID
static const int minPitch = -1.1;
static const int maxPitch = 1.1;

static const int carIconWidth = 1;
static const int carIconHeight = 1;

#else
static const int minPitch = -1.0;
static const int maxPitch = 1.0;
#endif

//--------------------------------------------------------------
void ofApp::setup(){

#ifdef ANDROID
    ofxAccelerometer.setup();

#else
    coreMotion.setupMagnetometer();
    coreMotion.setupGyroscope();
    coreMotion.setupAccelerometer();
    coreMotion.setupAttitude(CMAttitudeReferenceFrameXMagneticNorthZVertical);
    coreMotion.resetAttitude();
#endif

    fontface.load("verdana.ttf", 32);

    // make a udp connection that we can stream data to
    client.Create();
    client.Connect("192.168.42.1",3000);
    client.SetNonBlocking(true);

    string regStr = "register_control";
    // now register
    client.Send(regStr.c_str(), regStr.size());

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

    mouseDown = false;
    keepAliveTimer = ofGetElapsedTimef();

    disconnectSprite.setText("disconnect");
    disconnectSprite.setPosition(ofVec2f(20, 30));
    disconnectSprite.setScale(ofVec2f(300, 130));

    reconnectSprite.setText("reconnect");
    reconnectSprite.setPosition(ofVec2f(20, 165));
    reconnectSprite.setScale(ofVec2f(300, 130));

    spinSprite.setText("spin");
    spinSprite.setPosition(ofVec2f(20, 300));
    spinSprite.setScale(ofVec2f(300, 130));


}

//--------------------------------------------------------------
void ofApp::update(){

	if(!mouseDown)
	{
		speed *= 0.99;
	}

#ifdef ANDROID

    accel = ofxAccelerometer.getRawAcceleration();
    // figure out speed and direction from L/R tread
    float steer = ofMap(accel.x, -1.1, 1.1, 0, ofGetWidth());

    left = ofMap(accel.x, -1.1, 1.1, -127, 127);
    right = ofMap(accel.x, -1.1, 1.1, 127, -127);

#else
    coreMotion.update();

    float pitch = coreMotion.getPitch();
    pitch = ofClamp(pitch, minPitch, maxPitch);

    left = ofMap(pitch, minPitch, maxPitch, -127, 127);
    right = ofMap(pitch, minPitch, maxPitch, 127, -127);

    // figure out speed and direction from L/R tread
    //speed = ofMap( left + right, -254, 254, 0.03, -0.03);
    float steer = ofMap(left - right, -254, 254, 0, ofGetWidth());

#endif

    if(isConnected)
    {

        // send a message over our socket about our speed & position
        if(ofGetElapsedTimeMillis() - lastSend > 200) // 10hz refresh?
        {
            lastSend = ofGetElapsedTimeMillis();
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
            message << "speed:" << min(255, max(0, (leftTread + 127))) << ":" << min(255, max(0, (rightTread + 127)));
            ofLogVerbose() << " UDP message " << message.str();
            udpMessage = message.str();

            client.Send(message.str().c_str(), message.str().size());
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
    for( unsigned int i = 0; i < arcPoints.size(); i++ )
    {
        arcPoints.at(i).position += speed * 0.03;
        arcPoints.at(i).position = roundf(arcPoints.at(i).position * 60) / 60.0;
    }

    // if the point is off either end of the arc, delete it and add a new one at the beginning or end
    // of the arc so we're continuous
    for( unsigned int i = 0; i < arcPoints.size(); i++ )
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

    int scale = ofGetWidth()/10;

    // use our arc segments to draw boxes that are our 'road'
    for( unsigned int i = 1; i < arcPoints.size(); i++ )
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
    carIcon.draw(ofGetWidth()/2 - (carIcon.getWidth()/8), ofGetHeight() - carIcon.getHeight(), carIcon.getWidth()/carIconWidth, carIcon.getHeight()/carIconHeight);

    //    stringstream ss;
    //    ss <<  left << " " << right;
    //
    //    ofDrawBitmapString(ss.str(), 30, 30);
    //    ofDrawBitmapString(udpMessage, 30, 50);

    ofPushMatrix();
    ofTranslate(reconnectSprite.getBounds().x, reconnectSprite.getBounds().y);
    reconnectSprite.draw(fontface);
    ofPopMatrix();

    ofPushMatrix();
    ofTranslate(disconnectSprite.getBounds().x, disconnectSprite.getBounds().y);
    disconnectSprite.draw(fontface);
    ofPopMatrix();

    ofPushMatrix();
    ofTranslate(spinSprite.getBounds().x, spinSprite.getBounds().y);
    spinSprite.draw(fontface);
    ofPopMatrix();

    ofSetColor(255, 0, 0);
    ofDrawRectangle(ofGetWidth() - 40, (ofGetHeight()/2), 42, speed * -160);
    ofSetColor(255, 255, 255);
}

void ofApp::spin()
{
    cout << " spin " << endl;
    if(isConnected)
    {
        string spinStr = "spin_control";
        client.Send(spinStr.c_str(), spinStr.size());
    }
}

void ofApp::disconnect()
{
    cout << " disconnect " << endl;
    if(isConnected)
    {
        isConnected = false;
        string disconnectStr = "disconnect_control";

        client.Send(disconnectStr.c_str(), disconnectStr.size());
        client.Close();
    }
}

void ofApp::reconnect()
{
    cout << " reconnect " << endl;
    client.Create();
    client.Connect("192.168.42.1",3000);
    client.SetNonBlocking(true);

    // now register
    string regStr = "register_control";
    client.Send("register_control", regStr.size());

    isConnected = true;
}


#ifdef ANDROID

//--------------------------------------------------------------
void ofApp::touchDown(int x, int y, int id){
	ofVec2f touch(x,y);
    if( disconnectSprite.hitTest(touch))
    {
        disconnect();
    }
    else if( reconnectSprite.hitTest(touch))
    {
        reconnect();
    }
    else if( spinSprite.hitTest(touch) )
    {
        client.Send("do_spin", 7);
    }
    else
    {
        speed = ofMap(touch.y, 0, ofGetHeight(), 1, -1);
        mouseDown = true;
    }

}

//--------------------------------------------------------------
void ofApp::touchMoved(int x, int y, int id){
    // used to be 0.03 to -0.03
	ofVec2f touch(x,y);
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
void ofApp::touchUp(int x, int y, int id){
	mouseDown = false;
}

//--------------------------------------------------------------
void ofApp::touchDoubleTap(int x, int y, int id){

}

//--------------------------------------------------------------
void ofApp::touchCancelled(int x, int y, int id){

}

//--------------------------------------------------------------
void ofApp::swipe(ofxAndroidSwipeDir swipeDir, int id){

}

//--------------------------------------------------------------
void ofApp::pause(){

}

//--------------------------------------------------------------
void ofApp::stop(){

}

//--------------------------------------------------------------
void ofApp::resume(){

}

//--------------------------------------------------------------
void ofApp::reloadTextures(){

}

//--------------------------------------------------------------
bool ofApp::backPressed(){
    return false;
}

//--------------------------------------------------------------
void ofApp::okPressed(){

}

//--------------------------------------------------------------
void ofApp::cancelPressed(){

}


//--------------------------------------------------------------
void ofApp::keyPressed  (int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

#else


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
    else if( spinSprite.hitTest(touch) )
    {
        client.Send("do_spin", 7);
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

void ofApp::exit(){

}

#endif
