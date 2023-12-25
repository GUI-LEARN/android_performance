package com.example.performance_optimize.stability;

import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.util.Log;
import android.view.View;

import com.bytedance.android.bytehook.ByteHook;
import com.example.performance_optimize.R;

import java.util.ArrayList;

public class StabilityExampleActivity extends AppCompatActivity {
    final static String TAG = "StabilityExample";
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_stability_example);
        findViewById(R.id.anr).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                ByteHook.init();
                hookAnrByBHook();
                Thread thread = new Thread(new Runnable() {
                    @Override
                    public void run() {
                        synchronized (StabilityExampleActivity.this){
                            while (true);
                        }
                    }
                });
                thread.start();
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    throw new RuntimeException(e);
                }
                Log.i(TAG,"beforeToast");
                synchronized (StabilityExampleActivity.this){
                    Log.i(TAG,"can enter this code");
                }
            }
        });

        findViewById(R.id.crash).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                 ArrayList testList = null;
                testList.size();
            }
        });

        findViewById(R.id.oom).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
            }
        });

        findViewById(R.id.native_crash).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mockCrash();
            }
        });
        findViewById(R.id.breakpad).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                captureNativeCrash();
            }
        });
        System.loadLibrary("example");
        System.loadLibrary("optimize");
    }

    private static native void mockCrash();
    private static native void captureNativeCrash();
    private static native void hookAnrByBHook();
}