package com.ckzn.jrtptest;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import java.io.IOException;
import java.nio.ByteBuffer;

public class MainActivity extends AppCompatActivity
{
    byte[] data = new byte[1024];
    private boolean isRun = true;
    private static SurfaceView mSurfaceView;
    private static MediaCodec mCodec;
    private boolean isInit = false;
    private int mCount = 0;

    //i like you to the cast off sunday app compact activity

    private final static String MIME_TYPE = "video/avc";
    private final static int VIDEO_WIDTH = 720;
    private final static int VIDEO_HEIGHT = 1280;
    private final static int TIME_INTERNAL = 3000;

    static
    {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mSurfaceView = findViewById(R.id.surface_view);
        TextView tv = findViewById(R.id.rtp_log_tv);
        Button button = findViewById(R.id.start_receive);
        int result = initRtpLib("192.168.0.103" , 9002 , 9002);
        tv.setText("Return code = " + result);
        isRun = true;

        button.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                receiveData();
            }
        });
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        mSurfaceView.post(new Runnable()
        {
            @Override
            public void run()
            {
                if (!isInit)
                {
                    initDecoder();
                    isInit = true;
                }
            }
        });
    }

    public void initDecoder()
    {
        try
        {
            mCodec = MediaCodec.createDecoderByType(MIME_TYPE);
        }
        catch (IOException e)
        {
            e.printStackTrace();
        }
        MediaFormat mediaFormat = MediaFormat.createVideoFormat(MIME_TYPE,VIDEO_WIDTH, VIDEO_HEIGHT);
        mCodec.configure(mediaFormat, mSurfaceView.getHolder().getSurface(),null, 0);
        mCodec.start();
    }

    public boolean onFrame(byte[] buf, int length)
    {
        if (mCodec == null)
        {
            return false;
        }
        try
        {
            ByteBuffer[] inputBuffers = mCodec.getInputBuffers();
            int inputBufferIndex = mCodec.dequeueInputBuffer(-1);
            if (inputBufferIndex >= 0)
            {
                ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
                inputBuffer.clear();
                inputBuffer.put(buf, 0, length);
                mCodec.queueInputBuffer(inputBufferIndex, 0, length, mCount	* TIME_INTERNAL, 0);
                mCount++;
            }
            MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
            int outputBufferIndex = mCodec.dequeueOutputBuffer(bufferInfo, 100);
            while (outputBufferIndex >= 0)
            {
                mCodec.releaseOutputBuffer(outputBufferIndex, true);
                outputBufferIndex = mCodec.dequeueOutputBuffer(bufferInfo, 0);
            }
        }
        catch (IllegalStateException e)
        {
            e.printStackTrace();
        }
        return true;
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        isRun = false;
        deInitRtpLib();
    }

    public native int initRtpLib(String ip , int basePort , int destPort);

    public native int receiveData();

    public native int deInitRtpLib();
}
