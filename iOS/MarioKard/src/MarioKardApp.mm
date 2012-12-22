#include "cinder/app/AppCocoaTouch.h"
#include "cinder/app/Renderer.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"

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

////////////////////////////////////////////////
// simple handler to handle landing
////////////////////////////////////////////////

class echo_client_handler : public client::handler {
public:
    void on_message(connection_ptr con, message_ptr msg) {
        // we're not expecting anything
    }
    
    void on_fail(connection_ptr con) {
        std::cout << "connection failed" << std::endl;
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
    virtual void    shutdown();
		
	ci::Matrix44f	mCubeRotation;
	ci::gl::Texture mTex;
	ci::CameraPersp	mCam;
    
    // websocket
    std::string uri;
    client::handler::ptr handler;
    client::connection_ptr con;
    
    // Objective C
    CMMotionManager *motionManager;
    CMAttitude      *referenceAttitude;
    
    int cachedLeftTread, cachedRightTread;
    
    const float rollRatio;
    float treadSpeed;
    
};

MarioKardApp::MarioKardApp():rollRatio(0.8105694691387) {
    uri = "10.118.73.25:3000";
}

void MarioKardApp::setup()
{
	//////////////////////////////////////////////////////
    try {
        handler.reset(new echo_client_handler());
        client endpoint(handler);
        
        endpoint.alog().unset_level(websocketpp::log::alevel::ALL);
        endpoint.elog().unset_level(websocketpp::log::elevel::ALL);
        
        con = endpoint.connect(uri);
        
        endpoint.run();
        
        std::cout << "case count: " << boost::dynamic_pointer_cast<echo_client_handler>(handler)->m_case_count << std::endl;
        
        for (int i = 1; i <= boost::dynamic_pointer_cast<echo_client_handler>(handler)->m_case_count; i++) {
            endpoint.reset();
            
            std::stringstream url;
            
            url << uri << "runCase?case=" << i << "&agent=WebSocket++/0.2.0-dev";
            
            con = endpoint.connect(url.str());
            
            endpoint.run();
        }
        
        std::cout << "done" << std::endl;
        
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
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
    Quatf cubeQuat = Quatf( quat.w, quat.x, -quat.y, quat.z );
    
    // values we'll be sending over to 
    float leftTread = 122, rightTread = 122;
    
    //console() << " quat " << cubeQuat.getYaw() << " " << cubeQuat.getRoll()<< " " << cubeQuat.getPitch() << std::endl;
    console() << cubeQuat.getRoll()<< " " << cubeQuat.getPitch() << std::endl;

    float roll = cubeQuat.getRoll();
    
    // wretched hack linear mapping for steering - effing suck
    // roll() is 2.8 all the way to the left, -0.6 all the way to the right
    if( roll < 1.0) { // turning left
        if( treadSpeed < 0) { // going backwards
            
            leftTread = 127 + (127 * treadSpeed);
            rightTread = lmap<float>( roll, 1, 2.8, 0, 142 );
        } else { // going fowards
            
            leftTread  = (127 * treadSpeed) + 127;
            rightTread = lmap<float>( roll, -0.8, 1.0, 102, 255 );
        }
    } else if(roll > 1.0) {  // turning right
        if( treadSpeed < 0) { // going backwards
            
            leftTread = lmap<float>( roll, 1, 2.8, 0, 142 );
            rightTread = 127 + (127 * treadSpeed);
            
        } else { // going forwards
            leftTread = lmap<float>( roll, 1.0, 2.8, 255, 102 );
            rightTread = (127 * treadSpeed) + 127;
        }
    }
    
    std::stringstream payload;
    payload << (int) math<float>::floor(leftTread) << ":" << (int) math<float>::floor(rightTread) << std::endl;
    
    console() << " payload " << payload.str();
    
    if(con) {
        con->send(payload.str());
    }
    
}

void MarioKardApp::draw()
{
    gl::clear();
    Rectf rect(getWindowWidth() * (treadSpeed / 2.0) - 20, 0, getWindowWidth() * (treadSpeed / 2.0) + 20, getWindowHeight());
    gl::drawSolidRect(rect);
}

void MarioKardApp::shutdown()
{
    [motionManager stopDeviceMotionUpdates];
    [motionManager release];
    [referenceAttitude release];
}

void MarioKardApp::touchesMoved(cinder::app::TouchEvent event)
{
    
    int largestDiffY = 0, ind = 0;
    
    for(int i = 0; i < event.getTouches().size(); i++) {
        
        int d = abs(event.getTouches()[i].getPrevX() - event.getTouches()[i].getX());
        if(d > largestDiffY) {
            largestDiffY = d;
            ind = i;
        }
    }
    
    // calculate the position
    treadSpeed = (event.getTouches()[ind].getX() * 2.f / getWindowWidth());
    
}


CINDER_APP_COCOA_TOUCH( MarioKardApp, RendererGl )
