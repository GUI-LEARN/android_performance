package com.example.test


import com.android.build.api.transform.*
import groovyjarjarasm.asm.ClassReader
import groovyjarjarasm.asm.ClassVisitor
import groovyjarjarasm.asm.ClassWriter

class MyAsmTransform extends Transform {
    @Override
    void transform(TransformInvocation transformInvocation) throws TransformException, InterruptedException, IOException {
        // 在这里执行转换操作
        Collection<TransformInput> inputs = transformInvocation.getInputs();
        TransformOutputProvider outputProvider =
                transformInvocation.getOutputProvider();
        for (TransformInput input : inputs) {
            Collection<DirectoryInput> directoryInputs = input.getDirectoryInputs()
            //遍历 class
            for (DirectoryInput directoryInput : directoryInputs) {

                File dstFile = outputProvider.getContentLocation(
                        directoryInput.getName(),
                        directoryInput.getContentTypes(),
                        directoryInput.getScopes(),
                        Format.DIRECTORY);

                //根据字节码 file 创建 BufferedInputStream 和 BufferedOutputStream
                BufferedInputStream bis =
                        new BufferedInputStream(directoryInput.getFile())
                BufferedOutputStream bos = new BufferedOutputStream(dstFile)

                ClassReader reader = new ClassReader(bis)
                ClassWriter writer = new ClassWriter(reader)
                //在自定义的 TestClassVisitor 中通过 ASM 操作 class 文件中的方法
                ClassVisitor cv = new TestClassVisitor(writer)
                reader.accept(cv);
                bos.write(writer.toByteArray());
            }
        }
    }

    @Override
    String getName() {
        return "MyTransform"
    }

    @Override
    Set<QualifiedContent.ContentType> getInputTypes() {
        return Collections.<QualifiedContent.ContentType> singleton(QualifiedContent.DefaultContentType.CLASSES)
    }

    @Override
    Set<? super QualifiedContent.Scope> getScopes() {
        return Collections.singleton(QualifiedContent.Scope.PROJECT)
    }

    @Override
    boolean isIncremental() {
        return false
    }
}