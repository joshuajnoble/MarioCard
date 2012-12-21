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
	virtual void	mouseDown( ci::app::MouseEvent event );
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
    
};

MarioKardApp::MarioKardApp():rollRatio(0.8105694691387) {
}

void MarioKardApp::setup()
{
	//////////////////////////////////////////////////////
    try {
        handler.reset(new echo_client_handler());
        client endpoint(handler);
        
        endpoint.alog().unset_level(websocketpp::log::alevel::ALL);
        endpoint.elog().unset_level(websocketpp::log::elevel::ALL);
        
        con = endpoint.connect(uri+"getCaseCount");
        
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

void MarioKardApp::mouseDown( MouseEvent event )
{
	std::cout << "Mouse down @ " << event.getPos() << std::endl;
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
    
    // first speed, direction matters
    leftTread *= cubeQuat.getRoll() * rollRatio;
    rightTread *= cubeQuat.getRoll() * rollRatio;
    
    // quat.z is -1.67 all the way to the left, 1.67 all the way to the right
    leftTread += (122.f * abs(math<float>::min(1.67, cubeQuat.getPitch())/1.67));
    rightTread += (122.f * (math<float>::min(1.67, cubeQuat.getPitch()) / 1.67));
    
    std::stringstream payload;
    payload << (int) math<float>::floor(leftTread) << ":" << (int) math<float>::floor(rightTread) << std::endl;
    
    con->send(payload.str());
    
}

void MarioKardApp::draw()
{
	gl::clear( Color( 0.2f, 0.2f, 0.3f ) );
	gl::enableDepthRead();
	
	mTex.bind();
	gl::setMatrices( mCam );
	glPushMatrix();
		gl::multModelView( mCubeRotation );
		gl::drawCube( Vec3f::zero(), Vec3f( 2.0f, 2.0f, 2.0f ) );
	glPopMatrix();
}

void MarioKardApp::shutdown()
{
    [motionManager stopDeviceMotionUpdates];
    [motionManager release];
    [referenceAttitude release];
}


CINDER_APP_COCOA_TOUCH( MarioKardApp, RendererGl )
