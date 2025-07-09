# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an Android performance optimization example project that demonstrates various optimization techniques including memory management, stability improvements, and APK size reduction. The project includes both Java/Kotlin and native C++ components.

## Build Commands

### Building the Project
```bash
./gradlew build
```

### Building for Debug
```bash
./gradlew assembleDebug
```

### Building for Release
```bash
./gradlew assembleRelease
```

### Running Tests
```bash
./gradlew test
```

### Cleaning Build
```bash
./gradlew clean
```

## Architecture

### Module Structure
- **app/**: Main Android application module
- **buildSrc/**: Custom Gradle plugin for ASM bytecode manipulation
  - Contains `MyAsmPlugin` for custom transformations
  - Uses ASM 7.1 for bytecode manipulation

### Key Components

#### Java/Android Layer
- **MainActivity**: Entry point with navigation to different optimization examples
- **memory/**: Memory optimization demonstrations
  - `MemoryExampleActivity`: Main memory examples menu
  - `JavaLeakActivity`: Java memory leak examples
  - `NativeLeakActivity`: Native memory leak examples using ByteHook
- **stability/**: App stability examples
  - `StabilityExampleActivity`: Demonstrates ANR, crash, and native crash handling
- **apksize/**: APK size optimization examples
  - `ApkSizeOptimizeActivity`: APK size reduction techniques

#### Native C++ Layer
- **optimize.cpp**: CPU performance optimization utilities
- **hook_functions.cpp**: Function hooking implementations
- **native_capture.cpp**: Native crash capture using Breakpad
- **mock_native_*.cpp**: Mock implementations for testing crashes and leaks

#### Key Libraries
- **ByteHook**: Function hooking library for native code monitoring
- **Breakpad**: Google's crash reporting library for native crashes
- **LeakCanary**: Memory leak detection (debug builds only)

### Build Configuration
- **Target SDK**: 33
- **Min SDK**: 21
- **NDK ABI**: armeabi-v7a only
- **Build Tools**: Android Gradle Plugin 4.1.1
- **CMake**: Native code compilation with custom CMakeLists.txt

### Native Build System
The project uses CMake to build two main native libraries:
- `liboptimize.so`: Performance optimization functions
- `libexample.so`: Example native implementations with Breakpad integration

### Development Notes
- The project uses custom ASM transformations via `MyAsmPlugin`
- Breakpad is statically linked for native crash reporting
- ByteHook is used for runtime function hooking
- All native libraries are configured for ARM 32-bit architecture
- Custom APK naming: outputs as "PerformanceOptimizeDemo.apk"

### Performance Monitoring
The project includes examples for:
- Memory leak detection (Java and native)
- CPU performance optimization
- Native crash handling and reporting
- ANR (Application Not Responding) simulation and detection