#pragma once

#include <jni.h>
#include "cinder/Buffer.h"
#include "cinder/DataSource.h"
#include "motioncel/Game.h"

typedef unsigned char uint8_t;

class MCNative
{
  public:
    MCNative(motioncel::GameRef game, jobject view);
    ~MCNative();

    void update();
    void draw();
    void surfaceCreated();
    void surfaceChanged(int width, int height, int density);

	void touchEvent(int ev, float x, float y, float px, float py, int id, double eventTime);
    void postData(uint8_t *data, int length);
	ci::Buffer pollData();

    ci::DataSourceRef loadResource(const char *path);

    static class MCNative *create(jobject mcview);

  protected:
    motioncel::GameRef mGame;
    jobject            mView;
};

#define MOTIONCEL_VIEW(GAME) \
MCNative *MCNative::create(jobject mcview)                        \
{                                                                 \
	MCG = new GAME;                                               \
    MCNative* mc = new MCNative(motioncel::GameRef(MCG), mcview); \
    return mc;                                                    \
}

