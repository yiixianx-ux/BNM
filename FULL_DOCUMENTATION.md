# ByNameModding (BNM) - Full Documentation

ByNameModding (BNM) is a powerful, high-level C++20 library designed for modding Android games built with the Unity engine (IL2CPP). 

---

## 1. Philosophy: Why BNM?

### Symbolic vs. Offset Modding
Traditional modding often relies on **memory offsets** (e.g., `libil2cpp.so + 0x123456`). This approach is brittle; every game update changes these offsets, requiring tedious re-dumping and updating of the mod.

**BNM's Philosophy** is to use **Symbolic Modding**. Instead of offsets, you use the names of classes, methods, and fields.
*   **Stability**: If a game updates but the class/method names remain the same, your mod continues to work without changes.
    *   **Readability**: Code is much easier to understand (e.g., `Class("Player").GetMethod("SetHealth")` vs. `call_offset(0xABC123)`).
    *   **Productivity**: No need to wait for a dump or manual offset searching for every minor update.

---

## 2. Integration and Lifecycle

### Prerequisites
*   **Android NDK** (C++20 support required).
*   **Hooking Library**: BNM is a *framework*, not a *hooker*. You must provide a hooking backend (like [Dobby](https://github.com/jmpews/Dobby) or [ShadowHook](https://github.com/bytedance/android-inline-hook)).

### Configuration
Edit `include/BNM/UserSettings/GlobalSettings.hpp`:
1.  Set `UNITY_VER` (e.g., `222` for 2022.2.x).
2.  Implement `BasicHook` and `Unhook` templates using your chosen hooking library.

### Initialization
BNM must be initialized when the game loads. This is typically done in `JNI_OnLoad`.

```cpp
#include <BNM/Loading.hpp>

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);

    // Try to load BNM. It will find libil2cpp.so and setup its internal hooks.
    BNM::Loading::TryLoadByJNI(env);

    // Register a callback for when the engine is fully ready
    BNM::Loading::AddOnLoadedEvent([]() {
        BNM_LOG_INFO("BNM and IL2CPP are ready!");
        // Your mod logic here
    });

    return JNI_VERSION_1_6;
}
```

---

## 3. Core API

### Finding Classes (`BNM::Class`)
Classes are the entry point for most operations.

```cpp
using namespace BNM;

// Find by namespace and name
Class playerClass("UnityEngine", "Player");

// Find in a specific assembly (Image)
Image gameAssembly("Assembly-CSharp.dll");
Class enemyClass("", "EnemyController", gameAssembly);

// Get class from an existing IL2CPP object
Class targetClass(someObject);
```

### Methods (`BNM::Method`)
You can call any method, static or instance.

```cpp
// Instance method
auto setHealth = playerClass.GetMethod("SetHealth", 1); // 1 is parameter count
setHealth.cast<void>()[playerInstance](100.0f); // cast<ReturnType>

// Static method
auto getGravity = Class("UnityEngine", "Physics").GetMethod("get_gravity");
Vector3 gravity = getGravity.cast<Vector3>()();

// Finding by parameter names (slower but safer)
auto complexMethod = myClass.GetMethod("DoSomething", {"id", "name"});
```

### Hooking Methods
BNM provides a seamless way to hook IL2CPP methods.

```cpp
// 1. Define the original pointer
void (*old_Update)(BNM::UnityEngine::Object*);

// 2. Define your hook
void my_Update(BNM::UnityEngine::Object* instance) {
    // Logic before
    old_Update(instance);
    // Logic after
}

// 3. Apply the hook
BNM::Loading::AddOnLoadedEvent([]() {
    auto updateMethod = Class("Player").GetMethod("Update");
    BNM::BasicHook(updateMethod.GetOffset(), (void*)my_Update, (void**)&old_Update);
});
```

### Fields (`BNM::Field`)
Read and write fields easily.

```cpp
// Instance field
auto healthField = playerClass.GetField("_health").cast<float>();
float currentHealth = healthField[playerInstance].Get();
healthField[playerInstance].Set(200.0f);

// Static field
auto instanceField = Class("GameManager").GetField("Instance").cast<IL2CPP::Il2CppObject*>();
auto manager = instanceField.Get();
```

---

## 4. Data Types

### Mono Structures
BNM maps IL2CPP structures to easy-to-use C++ types.

*   **`BNM::Structures::Mono::String*`**: Use `.str()` to get `std::string`. Use `BNM::CreateMonoString("text")` to create one.
*   **`BNM::Structures::Mono::Array<T>*`**: Managed Unity arrays. Use `At(index)` or `[]`.
*   **`BNM::Structures::Mono::List<T>*`**: Managed Unity lists. Support `Add()`, `Remove()`, `Clear()`.

### Unity Structures
BNM includes implementations for:
*   `Vector2`, `Vector3`, `Vector4`, `Quaternion`, `Color`, `Ray`, etc.
*   **`BNM::UnityEngine::Object`**: The base for all Unity objects (GameObject, Component, etc.). 
    *   **CRITICAL**: Always use `.Alive()` or `IsValid()` to check if a Unity object is still valid in the engine.

---

## 5. ClassesManagement (Runtime Class Creation)

BNM allows you to create **NEW** C# classes at runtime or modify existing ones. This is unique and extremely powerful for adding new behaviors.

```cpp
class MyNewComponent : public BNM::UnityEngine::MonoBehaviour {
    // 1. Define the custom class
    BNM_CustomClass(MyNewComponent, BNM::CompileTimeClassBuilder("MyMod", "MyComponent"), Defaults::Get<UnityEngine::MonoBehaviour>(), nullptr);

    // 2. Define custom fields (accessible from C#!)
    int myCounter;
    BNM_CustomField(myCounter, Defaults::Get<int>(), "myCounter");

    // 3. Define custom methods
    void Start() {
        BNM_LOG_INFO("MyComponent started!");
    }
    BNM_CustomMethod(Start, false, Defaults::Get<void>(), "Start");

    void Update() {
        myCounter++;
    }
    BNM_CustomMethod(Update, false, Defaults::Get<void>(), "Update");
};
```

---

## 6. Advanced Features

### Coroutines
BNM bridges C++20 coroutines with Unity's Coroutine system.

```cpp
BNM::Coroutine::IEnumerator MyCoroutine() {
    BNM_LOG_INFO("Step 1");
    co_yield BNM::Coroutine::WaitForSeconds(2.0f);
    BNM_LOG_INFO("Step 2 after 2 seconds");
}

// Start it in Unity
someMonoBehaviour.GetMethod("StartCoroutine", 1)(MyCoroutine());
```

### CompileTimeClass
Used to reference classes that might not be loaded yet or for generic definitions.

```cpp
// Reference to List<int>
auto intListClass = Class("System.Collections.Generic", "List`1").GetGeneric({Defaults::Get<int>()});
```

---

## 7. Troubleshooting & Best Practices

1.  **Thread Safety**: Most BNM operations should happen on the IL2CPP thread. Use `BNM::AttachIl2Cpp()` if calling from your own thread.
2.  **Null Checks**: Use `BNM::CheckForNull(ptr)` for general pointers and `obj->Alive()` for Unity objects.
3.  **Generics**: Finding generic methods/classes requires `GetGeneric`. For example, `List<T>` is `List`1` in metadata.
4.  **Logging**: Use `BNM_LOG_INFO`, `BNM_LOG_ERR`, etc. These route to `__android_log_print`.
5.  **Obfuscation**: Use `BNM_OBFUSCATE("string")` if you have a string encryptor defined in `GlobalSettings.hpp`.

---

*Documentation generated for BNM Version 2.5.2*
