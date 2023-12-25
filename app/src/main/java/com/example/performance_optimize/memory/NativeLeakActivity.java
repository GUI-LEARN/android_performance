package com.example.performance_optimize.memory;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;

import com.bytedance.android.bytehook.ByteHook;
import com.example.performance_optimize.R;

public class NativeLeakActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_native_leak);
        ByteHook.init();
        System.loadLibrary("example");
        System.loadLibrary("optimize");
        hookMallocByBHook();
        mallocLeak();
    }

    private native void mallocLeak();

    private native void hookMallocByPLTHook();

    private native void hookMallocByBHook();
}


