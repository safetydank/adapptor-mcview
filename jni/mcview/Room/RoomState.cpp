#include "Room/RoomState.h"

#include "motioncel/game.h"
#include "cinder/gl/Fbo.h"

#include "Room/Room.h"
#include "Room/SpringCam.h"

using namespace cinder;
using namespace motioncel;

#define APP_WIDTH		1280
#define APP_HEIGHT		720
#define ROOM_FBO_RES	1

class RoomState : public State
{
  public:
	// CAMERA
	SpringCam		  mSpringCam;
	
	// SHADERS
	gl::GlslProg	  mRoomShader;
	
	// TEXTURES
	gl::Texture		  mIconTex;
	
	// ROOM
	Room			  mRoom;
	gl::Fbo			  mRoomFbo;
	
	// CONTROLLER
	// Controller		  mController;
    propipe::Matrices mMatrices;

    void onEnter()
    {
        Log(kDebug, "RoomState onEnter");
        // CAMERA	
        mSpringCam = SpringCam( -450.0f, MCG->aspectRatio() );
        
        // LOAD SHADERS
        mRoomShader = MCG->res()->getShader("room");

        Log(kDebug, "RoomState shader loaded");
        
        gl::Fbo::Format roomFormat;
        roomFormat.setColorInternalFormat( GL_RGBA );
        mRoomFbo			= gl::Fbo( APP_WIDTH/ROOM_FBO_RES, APP_HEIGHT/ROOM_FBO_RES, roomFormat );
        Log(kDebug, "Init room");
        bool isPowerOn		= true;
        bool isGravityOn	= true;
        mRoom				= Room( Vec3f( 350.0f, 200.0f, 350.0f ), isPowerOn, isGravityOn );
        mRoom.init();
        mRoom.togglePower();
        
        Log(kDebug, "Finished onEnter");
    }

    void update()
    {
        // ROOM
        mRoom.update( false );

        // CAMERA
        // if( mMousePressed ) 
        //     mSpringCam.dragCam( ( mMouseOffset ) * 0.02f, ( mMouseOffset ).length() * 0.02f );
        mSpringCam.update( 0.5f );
    }

    void drawIntoRoomFbo()
    {
        if (!mRoomShader)
            return;
        
        gl::clear( ColorA( 0.0f, 0.0f, 0.0f, 1.0f ), true );
        
        gl::setViewport( mRoomFbo.getBounds() );
        Vec2i ws(MCG->width(), MCG->height());
        gl::setViewport( Area(0, 0, ws.x, ws.y) );
        gl::disableAlphaBlending();
        gl::disable( GL_TEXTURE_2D );
        glEnable( GL_CULL_FACE );
        glCullFace( GL_BACK );
        Matrix44f m;
        m.setToIdentity();
        m.scale( mRoom.getDims() );
        
        mRoomShader.bind();
        mRoomShader.uniform( "mvpMatrix", mSpringCam.mMvpMatrix );
        mRoomShader.uniform( "mMatrix", m );
        mRoomShader.uniform( "roomDims", mRoom.getDims() );
        mRoomShader.uniform( "power", mRoom.getPower() );
        mRoomShader.uniform( "lightPower", mRoom.getLightPower() );
        mRoomShader.uniform( "timePer", mRoom.getTimePer() * 1.5f + 0.5f );
        mRoom.mVbo->draw(mRoomShader);
        
        glDisable( GL_CULL_FACE );
    }

    void draw()
    {
        propipe::DrawShaderRef shader = MCG->drawShader();
        propipe::DrawRef draw         = MCG->draw();

        gl::clear( ColorA( 0.5f, 0.1f, 0.1f, 1.0f ), true );
        
        drawIntoRoomFbo();
        
        // SET MATRICES TO WINDOW
        // mMatrices.setMatricesWindow(MCG->width(), MCG->height(), false);
        // shader->bindProg();
        // shader->setModelView(mMatrices.getModelView());
        // shader->setProjection(mMatrices.getProjection());

        // gl::setViewport(MCG->bounds());

        // gl::disableDepthRead();
        // gl::disableDepthWrite();
        // gl::enableAlphaBlending();
        // gl::enable( GL_TEXTURE_2D );
        // shader->setColor(Color::white());
        // 
        // // DRAW ROOM FBO
        // gl::Texture tex = mRoomFbo.getTexture();
        // shader->bindTexture(tex);
        // draw->drawSolidRect(MCG->bounds());
        // shader->unbindProg();        
        
        // SET MATRICES TO SPRING CAM
        // gl::setMatrices( mSpringCam.getCam() );
        
        // DRAW INFO PANEL
        // drawInfoPanel();
        
        // SAVE FRAMES
        // if( mSaveFrames && mNumSavedFrames < 5000 ){
        //     writeImage( getHomeDirectory() / "Room" / (toString( mNumSavedFrames ) + ".png"), copyWindowSurface() );
        //     mNumSavedFrames ++;
        // }
    }

    void onExit()
    {
        Log(kDebug, "RoomState::onExit()");
    }
};

StateRef createRoomState()
{
	return StateRef(new RoomState);
}

