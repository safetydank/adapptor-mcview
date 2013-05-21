#include "MCNative.h"

#include "cinder/Buffer.h"
#include "cinder/gl/gl.h"

#include "motioncel/Logger.h"

using namespace ci;
using namespace motioncel;

///////////////////////////////////////////////////////////////////////////////
//  JNI interface
//
JavaVM* gJVM = NULL;

extern "C" {

JNIEnv* getJNIEnv()
{
   JNIEnv* env = 0;
   int err = gJVM->GetEnv((void**) &env, JNI_VERSION_1_4);
   if (err == JNI_EDETACHED) {
      CI_LOGE("getJNIEnv error: current thread not attached to Java VM");
   }
   else if (err == JNI_EVERSION) {
      CI_LOGE("getJNIEnv error: VM doesn't support requested JNI version");
   }
   
   return env;
}

JavaVM* getJavaVM()
{
   return gJVM;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	//  Cache reference to Java VM
	gJVM = vm;
    motioncel::Logger::setup();
    return JNI_VERSION_1_4;
}

JNIEXPORT jlong JNICALL Java_com_adapptor_mcview_MCNative_jniCreate(JNIEnv* env, jobject obj,
        jobject mcview)
{
    jobject viewref = env->NewGlobalRef(mcview);
    env->DeleteLocalRef(mcview);
    return jlong(MCNative::create(viewref));
}

JNIEXPORT jlong JNICALL Java_com_adapptor_mcview_MCNative_jniUpdate(JNIEnv* env, jobject obj,
        jlong self)
{
    MCNative *mc = (MCNative *) self;
    mc->update();
}

JNIEXPORT jlong JNICALL Java_com_adapptor_mcview_MCNative_jniDraw(JNIEnv* env, jobject obj,
        jlong self)
{
    MCNative *mc = (MCNative *) self;
    mc->draw();
}

JNIEXPORT void JNICALL Java_com_adapptor_mcview_MCNative_jniSurfaceCreated(JNIEnv* env, jobject obj,
        jlong self) 
{
    MCNative *mc = (MCNative *) self;
    mc->surfaceCreated();
}

JNIEXPORT void JNICALL Java_com_adapptor_mcview_MCNative_jniSurfaceChanged(JNIEnv* env, jobject obj,
        jlong self, jint width, jint height, jint density) 
{
    MCNative *mc = (MCNative *) self;
    mc->surfaceChanged(width, height, density);
}

JNIEXPORT void JNICALL Java_com_adapptor_mcview_MCNative_jniPostData(JNIEnv *env, jobject obj,
        jlong self, jbyteArray array)
{
    MCNative *mc = (MCNative *) self;
    jint length = env->GetArrayLength(array);
    jbyte *bytes = (jbyte *) env->GetByteArrayElements(array, NULL);
    mc->postData(reinterpret_cast<uint8_t*>(bytes), length);
    env->ReleaseByteArrayElements(array, bytes, 0);
}

JNIEXPORT void JNICALL Java_com_adapptor_mcview_MCNative_jniTouchEvent(JNIEnv *env, jobject obj,
        jlong self, int ev, float x, float y, float px, float py, int id, jdouble eventTime)
{
    MCNative *mc = (MCNative *) self;
    mc->touchEvent(ev, x, y, px, py, id, eventTime);
}

JNIEXPORT jbyteArray JNICALL Java_com_adapptor_mcview_MCNative_jniPollData(JNIEnv *env, jobject obj,
        jlong self)
{
    MCNative *mc = (MCNative *) self;
    Buffer buffer = mc->pollData();
    if (buffer) {
        //  XXX convert buffer to java byte array
        jbyteArray arr = env->NewByteArray(buffer.getDataSize());
        if (arr) {
            env->SetByteArrayRegion(arr, 0, buffer.getDataSize(), (jbyte *) buffer.getData());
            return arr;
        }
    }

    return NULL;
}

void app_dummy() {}

} // extern "C"


///////////////////////////////////////////////////////////////////////////////
//  MCNative definition
//
MCNative::MCNative(motioncel::GameRef game, jobject view)
    : mGame(game), mView(view)
{
    Log(kDebug, "MCNative::MCNative()");
}

MCNative::~MCNative()
{
    Log(kDebug, "MCNative::~MCNative()");
    getJNIEnv()->DeleteGlobalRef(mView);
    mView = NULL;
}

void MCNative::update()
{
    // CI_LOGD("MCNative::update");
    mGame->privateUpdate__();
	mGame->eventQueue__.clear();
}

void MCNative::draw()
{
    // CI_LOGD("MCNative::draw");
    mGame->privateDraw__();
}

void MCNative::surfaceCreated()
{
    CI_LOGD("MCNative::surfaceCreated");
}

void MCNative::surfaceChanged(int width, int height, int density)
{
    CI_LOGD("MCNative::surfaceChanged %d x %d (%d dpi)", width, height, density);
    mGame->privateSetup__(std::bind(&MCNative::loadResource, this, std::placeholders::_1), width, height, density);
}

namespace {
	enum {
		kTouchDown  = 0,
		kTouchMoved = 1,
		kTouchUp    = 2
	};
}

void MCNative::touchEvent(int ev, float x, float y, float px, float py, int id, double eventTime)
{
	// XXX timestamp? reuse 1 event?
	app::TouchEvent event;
	event.getTouches().push_back(app::TouchEvent::Touch(Vec2f(x, y), Vec2f(px, py), id, eventTime, NULL));
	mGame->eventQueue__.push(ev, event);
}

void MCNative::postData(uint8_t *data, int length)
{
    mGame->onPostData(data, length);
}

//  Callback
DataSourceRef MCNative::loadResource(const char *path)
{
    JNIEnv *env = getJNIEnv();

    jclass mcviewClass = env->FindClass("com/adapptor/mcview/MCView");
    if (mcviewClass == NULL) {
        Log(kError, "Unable to resolve class com/adapptor/mcview/MCView");
        return DataSourceRef();
    }

    jmethodID loadAsset = env->GetMethodID(mcviewClass, "loadAsset", "(Ljava/lang/String;)[B");
    if (loadAsset == NULL) {
        Log(kError, "Unable to resolve method loadAsset");
        return DataSourceRef();
    }

    jstring pathString   = env->NewStringUTF(path);
    jbyteArray byteArray = (jbyteArray) env->CallObjectMethod(mView, loadAsset, pathString);

    if (byteArray == NULL) {
        Log(kDebug, "Received null array for asset path %s", path);
        return DataSourceRef();
    }

    int length     = env->GetArrayLength(byteArray);
    jbyte* dataPtr = env->GetByteArrayElements(byteArray, 0);

    Buffer buffer(length);
    buffer.copyFrom(dataPtr, length);

    env->ReleaseByteArrayElements(byteArray, dataPtr, 0);

    //  release local references explicitly (not required)
    // env->DeleteLocalRef(byteArray);
    // env->DeleteLocalRef(pathString);
    // env->DeleteLocalRef(mcviewClass);

    return DataSourceBuffer::create(buffer);
}

Buffer MCNative::pollData()
{
	if (!mGame->postedData().empty()) {
        auto &data = mGame->postedData();
		Buffer buf = data.front();
        data.pop_front();
		return buf;
	}

	return Buffer();
}

