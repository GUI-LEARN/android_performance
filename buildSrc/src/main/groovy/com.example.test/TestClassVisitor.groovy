package com.example.test

import jdk.internal.org.objectweb.asm.commons.AdviceAdapter
import org.objectweb.asm.ClassVisitor
import org.objectweb.asm.MethodVisitor

class TestClassVisitor extends ClassVisitor {

    TestClassVisitor(int api) {
        super(api)
    }


    @Override
    MethodVisitor visitMethod(int access, String name, String desc, String signature, String[] exceptions) {
        MethodVisitor mv = super.visitMethod(access, name, desc, signature, exceptions);
        //取出所有需要修改的方法一个一个创建visitor去进行访问
        mv = new TestMethodVisitor(changer, mClassName, mv, access, name, desc);
        return mv;
    }

    private static class TestMethodVisitor extends AdviceAdapter {
        protected TestMethodVisitor(int api, MethodVisitor mv, int access, String name, String desc) {
            super(api, mv, access, name, desc)
        }

        @Override
        protected void onMethodEnter() {
            // 执行指令；获取静态属性
            mv.visitFieldInsn(Opcodes.GETSTATIC,
                    "java/lang/System", "out", "Ljava/io/PrintStream;")
            // 加载常量 load constant
            mv.visitLdcInsn("hello world")
            // 调用方法
            mv.visitMethodInsn(Opcodes.INVOKEVIRTUAL,
                    "java/io/PrintStream", "print", "(Ljava/lang/String;)V", false)
            super.onMethodEnter()
        }
    }

    @Override
    void visitEnd() {
        //访问结束
    }

    @Override
    void visit(int version, int access, String name, String signature, String superName, String[] interfaces) {
        super.visit(version, access, name, signature, superName, interfaces)
    }


}