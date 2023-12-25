package com.example.test

import com.android.build.gradle.AppExtension
import org.gradle.api.Plugin
import org.gradle.api.Project

class MyAsmPlugin implements Plugin<Project> {
    @Override
    void apply(Project project) {
        // 获取AppExtension对象
        AppExtension appExtension = project.getExtensions().getByType(AppExtension.class);
        // 注册自定义的Transform脚本
        appExtension.registerTransform(new MyAsmTransform());
    }
}