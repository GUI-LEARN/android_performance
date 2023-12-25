package com.example.performance_optimize.memory;

import androidx.fragment.app.FragmentActivity;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;

import com.bytedance.android.bytehook.ByteHook;
import com.example.performance_optimize.R;

public class MemoryExampleActivity extends FragmentActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_memory_example);
        findViewById(R.id.java_leak).setOnClickListener(v -> startActivity(new Intent(MemoryExampleActivity.this,JavaLeakActivity.class)));
        findViewById(R.id.native_leak).setOnClickListener(v -> startActivity(new Intent(MemoryExampleActivity.this,NativeLeakActivity.class)));
    }

}