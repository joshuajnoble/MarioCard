#include "cinder/app/AppCocoaTouch.h"
#include "cinder/app/Renderer.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/ImageIo.h"
#include "cinder/Camera.h"

#include "Resources.h"

// for sensor fusion
#include "cinder/Cinder.h"
#include "cinder/Vector.h"
#include "cinder/Matrix.h"
#include "cinder/Quaternion.h"

#import <CoreMotion/CoreMotion.h>

// for libwebsocketcpp

#pragma push_macro("nil") // gets around a weird "nil" in boost error
#pragma push_macro("Nil")
#undef nil
#undef Nil

#include "roles/client.hpp"
#include "websocketpp.hpp"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#pragma pop_macro("nil")
#pragma pop_macro("Nil")

using boost::asio::ip::tcp;
using namespace websocketpp;

using namespace ci;
using namespace ci::app;

///////////////////////////////////////////////
// static var for errors
///////////////////////////////////////////////

int errorState;



////////////////////////////////////////////////
// simple handler to handle landing
////////////////////////////////////////////////

class echo_client_handler : public client::handler {
public:
    
    void on_open(connection_ptr con) {
        console() << "connection open " << std::endl;
        errorState = 0;
    }
    
   void on_close(connection_ptr con) {
        console() << "connection closed " << std::endl;
        errorState = 2;
    }
    
    void on_message(connection_ptr con, message_ptr msg) {
        // we're not expecting anything
    }
    
    void on_fail(connection_ptr con) {
        console() << "connection failed" << std::endl;
        errorState = 1;
    }
    
    int m_case_count;
};

////////////////////////////////////////////////
// main application
////////////////////////////////////////////////

class MarioKardApp : public AppCocoaTouch {
  public:
    
    MarioKardApp();
    
	virtual void	setup();
	virtual void	resize( int width, int height );
	virtual void	update();
	virtual void	draw();
	
    void touchesMoved(cinder::app::TouchEvent event);
    void touchesBegan(cinder::app::TouchEvent event);
    void touchesEnded(cinder::app::TouchEvent event);
    
    virtual void    shutdown();
		
	ci::Matrix44f	mCubeRotation;
	ci::gl::Texture mTex;
	ci::CameraPersp	mCam;
    
    // websocket
    std::shared_ptr<boost::thread> thread;
    std::string uri;
    client *endpoint;
    client::handler::ptr handler;
    client::connection_ptr con;
    
    // Objective C
    CMMotionManager *motionManager;
    CMAttitude      *referenceAttitude;
    
    int cachedLeftTread, cachedRightTread;
    
    const float rollRatio;
    float treadSpeed;
    gl::Texture bg;
    
    float maxRoll, minRoll;
    
    float ROLL_MAX, ROLL_MIN, ROLL_MEAN;
    int FULL_SPEED_BWD, STOPPED, FULL_SPEED_FWD;
    
    bool touchMoved;
    float timeTouchHeld;
    Font mFont;
    gl::TextureFontRef mTextureFont;
    std::string speeds;
    
};

MarioKardApp::MarioKardApp():rollRatio(0.8105694691387) {
    uri = "ws://10.118.73.84:3000";
}

void MarioKardApp::setup()
{
    
    errorState = 0;
    
    mFont = Font( "Helvetica", 36 );
    mTextureFont = gl::TextureFont::create( mFont );
    
    timeTouchHeld = 0;
    
    ROLL_MAX = 0.5;
    ROLL_MIN = -0.5;
    ROLL_MEAN = 0;
    
    FULL_SPEED_BWD = 0;
    STOPPED = 127;
    FULL_SPEED_FWD = 255;
    
    treadSpeed = 0;
    maxRoll = -55;
    minRoll = 55;
    
    bg = loadImage(loadResource(BACKGROUND));
    
    console() << " start up " << std::endl;
    
	//////////////////////////////////////////////////////
    try {
        handler.reset(new echo_client_handler());
        endpoint = new client(handler);
        
        endpoint->alog().unset_level(websocketpp::log::alevel::ALL);
        endpoint->elog().unset_level(websocketpp::log::elevel::ALL);
        
        con = endpoint->connect(uri);
        
        thread = std::shared_ptr<boost::thread>( new boost::thread(boost::bind(&client::run, endpoint, false)));
        
        console() << "done" << std::endl;
        
    } catch (std::exception& e) {
        console() << "Exception: " << e.what() << std::endl;
        errorState = 1;
    }
    
    //////////////////////////////////////////////////////
    // now set up the motion manager
    
    motionManager = [[CMMotionManager alloc] init];
    [motionManager startDeviceMotionUpdates];
    
    CMDeviceMotion *dm = motionManager.deviceMotion;
    referenceAttitude = [dm.attitude retain];
}

void MarioKardApp::resize( int width, int height )
{
	mCam.lookAt( Vec3f( 3, 2, -3 ), Vec3f::zero() );
	mCam.setPerspective( 60, width / (float)height, 1, 1000 );
}

void MarioKardApp::update()
{
	//mCubeRotation.rotate( Vec3f( 1, 1, 1 ), 0.03f );
    
    // The translation and rotation components of the modelview matrix
    CMQuaternion quat;
    CMDeviceMotion *deviceMotion = motionManager.deviceMotion;
    CMAttitude *attitude = deviceMotion.attitude;    
    quat = attitude.quaternion;

    // values we'll be sending over to 
    float leftTread = 122, rightTread = 122;
    float roll = quat.z;
    
    if(maxRoll < roll) {
        maxRoll = roll;
    }
    
    if(minRoll > roll) {
        minRoll = roll;
    }

    float normalizedTreadSpeed = treadSpeed * 2;

    
    /* STEERING
     
     0 - 0.5 = left
     0 - -0.5 = right
     
     We have switches to handle going forwards or backwards
     
     */
    if( roll > 0) { // turning left
        if( treadSpeed < 0) { // going backwards WORKS
            
            leftTread = math<float>::clamp(STOPPED + (STOPPED * normalizedTreadSpeed), FULL_SPEED_BWD, FULL_SPEED_FWD); // to 0
            rightTread = STOPPED - ( fabs(STOPPED * normalizedTreadSpeed) - fabs(roll * STOPPED));
            
        } else { // going fowards WORKS
            
            rightTread  = math<float>::clamp(STOPPED + (STOPPED * normalizedTreadSpeed), STOPPED, FULL_SPEED_FWD); // to 255
            leftTread = (STOPPED * normalizedTreadSpeed + STOPPED) - ( roll  * STOPPED);
        }
    } else {  // turning right
        if( treadSpeed < 0) { // going backwards
            
            rightTread = math<float>::clamp(STOPPED + (STOPPED * normalizedTreadSpeed), FULL_SPEED_BWD, FULL_SPEED_FWD); // to 0
            leftTread = STOPPED - ( fabs(STOPPED * normalizedTreadSpeed) - fabs(roll * STOPPED));
            
        } else { // going forwards
            
            leftTread  = math<float>::clamp(STOPPED + (STOPPED * normalizedTreadSpeed), STOPPED, FULL_SPEED_FWD); // to 255
            rightTread = (STOPPED * normalizedTreadSpeed + STOPPED) + (roll * STOPPED);
        }
    }
    
    std::stringstream payload;
    
    std::stringstream lstring, rstring;
    int l = (int) math<float>::floor(leftTread);
    
    if( l < 10 ) {
        lstring << "00" << l;
    }
    else if( l < 100) {
        lstring << "0" << l;
    }
    else {
        lstring << l;
    }
    
    int r = (int) math<float>::floor(rightTread);
    
    if( r < 10 ) {
        rstring << "00" << r;
    }
    else if( l < 100) {
        rstring << "0" << r;
    }
    else {
        rstring << r;
    }
    
    payload << "speed:" << lstring.str()  << ":" << rstring.str() << std::endl;
    
    //payload << "speed:" << (int) math<float>::floor(leftTread) << ":" << (int) math<float>::floor(rightTread) << std::endl;
    //console() << " payload " << payload.str();
    
    speeds = payload.str();
    
    if(con->get_state() == session::state::OPEN)
    {
        //console() << " sending " << payload.str() << std::endl;
        con->send(payload.str());
    } else if(con->get_state() != session::state::OPEN && getElapsedSeconds() > 3.f) {
        errorState = 1;
        return;
    }

    
    
}

void MarioKardApp::draw()
{
    gl::clear();
    
    gl::pushMatrices();
    
    if(getElapsedSeconds() < 20.f)
    {
        gl::rotate(90);
        gl::translate(0, -640);
        gl::draw( bg );
        gl::translate(0, 640);
        gl::rotate(-90);
    }
    
    Rectf rect(getWindowWidth() * (treadSpeed + 0.5) - 20, -100, getWindowWidth() * (treadSpeed + 0.5) + 20, getWindowHeight() + 100);
    gl::drawSolidRect(rect);
    
    gl::color(Color(1, 1, 1));
    
    if( errorState == 0 ) {
        gl::rotate(90);
        gl::translate(0, -640);
        mTextureFont->drawString( speeds, Vec2f( 10, 50 ) );
        gl::translate(0, 640);
        gl::rotate(-90);
    }
    
    if( errorState == 1 ) {
        gl::rotate(90);
        gl::translate(0, -640);
        mTextureFont->drawString( " Can't connect to server ", Vec2f( 10, 50 ) );
        gl::translate(0, 640);
        gl::rotate(-90);
    }
    
    if( errorState == 2 ) {
        gl::rotate(90);
        gl::translate(0, -640);
        mTextureFont->drawString( " Server closed connection ", Vec2f( 10, 50 ) );
        gl::translate(0, 640);
        gl::rotate(-90);
    }
    
    gl::popMatrices();
    
}

void MarioKardApp::shutdown()
{
    
    delete endpoint;
    
    thread->join();
    
    [motionManager stopDeviceMotionUpdates];
    [motionManager release];
    [referenceAttitude release];
}

void MarioKardApp::touchesBegan(cinder::app::TouchEvent event) {
    timeTouchHeld = getElapsedSeconds();
    touchMoved = false;
}

void MarioKardApp::touchesEnded(cinder::app::TouchEvent event) {
    if(!touchMoved && getElapsedSeconds() - timeTouchHeld > 3.0) {
        
        // we just restart to get a new reference frame
        [motionManager stopDeviceMotionUpdates];
        [motionManager startDeviceMotionUpdates];
    }
}

void MarioKardApp::touchesMoved(cinder::app::TouchEvent event)
{
    
    touchMoved = true;
    
    int largestDiffY = 0, ind = 0;
    
    for(int i = 0; i < event.getTouches().size(); i++) {
        
        int d = abs(event.getTouches()[i].getPrevX() - event.getTouches()[i].getX());
        if(d > largestDiffY) {
            largestDiffY = d;
            ind = i;
        }
    }
    
    // calculate the speed: -0.5 = reverse, 0.5 = forward
    treadSpeed = (event.getTouches()[ind].getX() / getWindowWidth()) - 0.5;
    
}


CINDER_APP_COCOA_TOUCH( MarioKardApp, RendererGl )
