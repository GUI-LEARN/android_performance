package com.example.performance_optimize;

import android.content.Intent;
import android.os.Bundle;
import com.example.performance_optimize.memory.MemoryExampleActivity;
import com.example.performance_optimize.stability.StabilityExampleActivity;

import android.view.View;
import androidx.fragment.app.FragmentActivity;

public class MainActivity extends FragmentActivity {


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findViewById(R.id.memory).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivity(new Intent(MainActivity.this, MemoryExampleActivity.class));
            }
        });
        findViewById(R.id.stability).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivity(new Intent(MainActivity.this, StabilityExampleActivity.class));
            }
        });
    }

}