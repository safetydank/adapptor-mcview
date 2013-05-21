package com.adapptor.mcview;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.List;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;

public class MCView extends GLSurfaceView implements GLSurfaceView.Renderer
{
    static final String TAG = "MCView";

    Context  mContext;
    MCNative mMC;

    //  Maximum number of points tracked
    static final int kMaxPoints  = 10;

    static final int kTouchDown  = 0;
    static final int kTouchMoved = 1;
    static final int kTouchUp    = 2;

    float[] mPrevX;
    float[] mPrevY;

    public MCView(Context context)
    {
        super(context);
        init(context);
    }

    public MCView(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        init(context);
    }

    private void init(Context context)
    {
        mPrevX = new float[kMaxPoints];
        mPrevY = new float[kMaxPoints];

        mContext = context;
        mMC = new MCNative(this);
        setEGLContextClientVersion(2);
        setRenderer(this);
    }

    @Override
    public void onDrawFrame(GL10 gl) 
    {
        //  Render thread
        mMC.update();
        mMC.draw();
		List<byte[]> dataList = mMC.pollData();
		if (!dataList.isEmpty()) {
			for (byte[] data : dataList) {
				onPostData(data);
			}
		}
    }

	//  Override to receive polled data from the native component
	public void onPostData(byte[] data)
	{
	}

    public boolean onTouchEvent(MotionEvent ev)
    {
        final int action       = ev.getActionMasked();
        final double eventTime = ev.getEventTime() / 1000.0;

        switch (action) {
            //  Touches began
            case MotionEvent.ACTION_DOWN: {
                float x = ev.getX();
                float y = ev.getY();
                int id  = ev.getPointerId(0);
                addTouchEvent(kTouchDown, x, y, x, y, id, eventTime);
                mPrevX[id] = x;
                mPrevY[id] = y;
                break;
            }
            case MotionEvent.ACTION_POINTER_DOWN: {
                int pointerIndex = ev.getActionIndex();
                float x = ev.getX(pointerIndex);
                float y = ev.getY(pointerIndex);
                int id  = ev.getPointerId(pointerIndex);
                addTouchEvent(kTouchDown, x, y, x, y, id, eventTime);
                mPrevX[id] = x;
                mPrevY[id] = y;
                break;
            }

            //  Touches moved
            case MotionEvent.ACTION_MOVE: {
                //  XXX getActionIndex() does not indicate when a secondary pointer moves
                for (int index=0; index < ev.getPointerCount(); ++index) {
                    float x = ev.getX(index);
                    float y = ev.getY(index);
                    int id = ev.getPointerId(index);
                    addTouchEvent(kTouchMoved, x, y, mPrevX[id], mPrevY[id], id, eventTime);
                    mPrevX[id] = x;
                    mPrevY[id] = y;
                }
                break;
            }

            //  Touches ended
            case MotionEvent.ACTION_UP: {
                float x = ev.getX();
                float y = ev.getY();
                int id = ev.getPointerId(0);
                addTouchEvent(kTouchUp, x, y, mPrevX[id], mPrevY[id], id, eventTime);
                break;
            }
            case MotionEvent.ACTION_POINTER_UP: {
                // Extract the index of the pointer that left the touch sensor
                int pointerIndex = ev.getActionIndex();
                float x = ev.getX(pointerIndex);
                float y = ev.getY(pointerIndex);
                int id = ev.getPointerId(pointerIndex);
                addTouchEvent(kTouchUp, x, y, mPrevX[id], mPrevY[id], id, eventTime);
                break;
            }
        }

        return true;
    }

    void addTouchEvent(final int ev, final float x, final float y,
            final float mPrevX, final float mPrevY, final int id, final double eventTime)
    {
        queueEvent(new Runnable() {
            @Override
            public void run() {
                mMC.touchEvent(ev, x, y, mPrevX, mPrevY, id, eventTime);
            }
        });
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) 
    {
        Log.d(TAG, "onSurfaceChanged");
        mMC.surfaceChanged(width, height);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) 
    {
        Log.d(TAG, "onSurfaceCreated");
        mMC.surfaceCreated();
    }

    //  Resource helpers
    public byte[] loadAsset(String path)
    {
        AssetManager am = mContext.getAssets();
        byte[] asset = null;

        try {
            InputStream is = am.open(path);

            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            byte[] buffer = new byte[4096];
            int len;
            while ((len = is.read(buffer)) > 0) {
                baos.write(buffer, 0, len);
            }
            asset = baos.toByteArray();
        }
        catch (IOException ex) {
            Log.v(TAG, "postAsset error: "+ex);
            asset = null;
        }

        return asset;
    }

    public void postAsset(String path)
    {
        byte[] asset = loadAsset(path);
        postResourceData(path, asset);
    }

    public void postResourceData(String path, byte[] data)
    {
        byte[] cookie = new byte[] { 'R', 'E', 'S', 'M' };
        byte[] pathBytes = path.getBytes();

        Log.d(TAG, String.format("Writing resource %s, data size %d", path, data.length));
        try {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            baos.write(cookie);
            baos.write(bytesFromInt(pathBytes.length));
            baos.write(pathBytes);
            baos.write(bytesFromInt(data.length));
            baos.write(data);

            mMC.postData(baos.toByteArray());
        }
        catch (IOException ex) {
            Log.e(TAG, "postResourceData error: "+ex);
        }
    }

    byte[] bytesFromInt(int val)
    {
        //  Write lengths as little-endian values
        byte[] bytes = ByteBuffer.allocate(4).putInt(val).array();
        for (int i=0, j=3; i < j; ++i, --j) {
            byte tmp = bytes[i];
            bytes[i] = bytes[j];
            bytes[j] = tmp;
        }

        return bytes;
    }

    public void postData(final byte[] data)
    {
        //  Post data on render thread
        queueEvent(new Runnable() {
            public void run() {
                mMC.postData(data);
            }
        });
    }
}

