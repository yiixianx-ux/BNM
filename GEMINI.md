# ByNameModding (BNM) Project Overview

ByNameModding (BNM) is a high-level, performance-oriented C++20 library designed for modding Android games built with the Unity engine (IL2CPP). It provides a symbolic modding framework, allowing developers to interact with game internals using class, method, and field names instead of brittle memory offsets.

## Core Architecture & Technologies

- **Language**: C++20 (minimum requirement).
- **Runtime Environment**: Android (target), IL2CPP (Unity's scripting backend).
- **Build Systems**: CMake and Android NDK (`Android.mk`).
- **Key Components**:
    - **Reflection API**: High-level wrappers (`BNM::Class`, `BNM::Method`, `BNM::Field`, `BNM::Property`, `BNM::Event`) for IL2CPP metadata.
    - **Data Structures**: C++ implementations of Mono/Unity types (`BNM::Structures::Mono::String`, `Array`, `List`, `Vector3`, etc.).
    - **Initialization**: Multiple loading methods via JNI, `dlfcn` handles, or custom finders.
    - **Runtime Class Management**: Ability to create new C# classes and methods from C++ at runtime.
    - **Coroutine Support**: Integration between C++20 coroutines and Unity's `IEnumerator` system.

## Building and Running

### Build Prerequisites
- Android NDK with C++20 support.
- A hooking library backend (e.g., [Dobby](https://github.com/jmpews/Dobby), [ShadowHook](https://github.com/bytedance/android-inline-hook)).

### Key Commands
- **Build (CMake)**:
  ```bash
  mkdir build && cd build
  cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a ..
  make
  ```
- **Build (ndk-build)**:
  Ensure `Android.mk` and `Application.mk` are correctly configured and run `ndk-build` from the project root.

### Integration
1. Configure `include/BNM/UserSettings/GlobalSettings.hpp` with your Unity version and hooking functions.
2. Initialize BNM in your `JNI_OnLoad`:
   ```cpp
   BNM::Loading::TryLoadByJNI(env);
   BNM::Loading::AddOnLoadedEvent([]() {
       // Mod logic here
   });
   ```

## Development Conventions

- **Symbolic Modding**: Always prefer using names (`Class("Namespace", "Name")`) over memory offsets to ensure stability across game updates.
- **Safety**:
    - Use `.Alive()` or `.IsValid()` to check the state of Unity objects (`BNM::UnityEngine::Object`).
    - Use `BNM::AttachIl2Cpp()` when calling BNM/IL2CPP functions from non-game threads.
- **Logging**: Use project-specific macros: `BNM_LOG_INFO`, `BNM_LOG_DEBUG`, `BNM_LOG_WARN`, `BNM_LOG_ERR`.
- **String Handling**: Use `BNM_OBFUSCATE("string")` for sensitive strings if a custom obfuscator is implemented in `GlobalSettings.hpp`.
- **Naming Style**: Generally follows PascalCase for classes and camelCase for methods/fields, matching C# conventions where appropriate for the reflection API.

## Directory Structure

- `include/BNM/`: Public headers for the library API.
- `src/`: Core implementation files.
- `external/`: Dependencies like `utf8-cpp` and custom RTTI.
- `include/BNM/Il2CppHeaders/`: Unity-version-specific IL2CPP headers.
- `include/BNM/UserSettings/`: Configuration files for users to customize BNM.

---
*For more detailed information, refer to `FULL_DOCUMENTATION.md` or the [official documentation](https://bynamemodding.github.io/).*
