package com.adapptor.mcview;

import android.util.DisplayMetrics;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

public class MCNative
{
    static {
        System.loadLibrary("mcview");
    }

    static final String TAG = "MCNative";

    private long mSelf;
    private MCView mView;

    public MCNative(MCView view)
    {
        mView = view;
        mSelf = jniCreate(view);
    }

    public void update()
    {
        jniUpdate(mSelf);
    }

    public void draw()
    {
        jniDraw(mSelf);
    }

    public void surfaceCreated()
    {
        jniSurfaceCreated(mSelf);
    }

    public void surfaceChanged(int width, int height)
    {
        DisplayMetrics metrics = mView.getContext().getResources().getDisplayMetrics();
        jniSurfaceChanged(mSelf, width, height, metrics.densityDpi);
    }

    public void postData(byte[] data)
    {
        jniPostData(mSelf, data);
    }

	public List<byte[]> pollData()
	{
		ArrayList<byte[]> dataList = new ArrayList<byte[]>();
		while (true) {
			byte[] data = jniPollData(mSelf);
			if (data != null) {
				dataList.add(data);
			}
			else {
				break;
			}
		}

		return dataList;
	}

    public void touchEvent(int ev, float x, float y, float prevX, float prevY, int id, double eventTime)
    {
        jniTouchEvent(mSelf, ev, x, y, prevX, prevY, id, eventTime);
    }

    //  Native methods
    public static native long   jniCreate(MCView view);
    public static native void   jniUpdate(long self);
    public static native void   jniDraw(long self);
    public static native void   jniSurfaceCreated(long self);
    public static native void   jniSurfaceChanged(long self, int width, int height, int density);
    public static native void   jniPostData(long self, byte[] string);
    public static native void   jniTouchEvent(long self, int ev, float x, float y, float prevX, float prevY, int id, double eventTime);
	public static native byte[] jniPollData(long self);
}

