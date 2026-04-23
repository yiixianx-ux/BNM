## ByNameModding
#### English | [Русский](https://github.com/ByNameModding/BNM-Android/blob/master/README_RU.md)
ByNameModding is a library for modding of il2cpp games by classes, methods, field names. It includes everything you need for Android Unity games modding.<br>
Requires C++20 minimum.

## Recent Architectural Refactor (April 2026)
This version includes a major architectural overhaul focusing on stability, thread-safety, and modern C++ standards:
- **Robust ARM64 Scanning**: Replaced manual byte scanning with a formal ARM64 instruction decoder for reliable symbol resolution.
- **Thread-Safe Metadata**: Implemented atomic patching and memory barriers for vtables and method arrays.
- **Memory Safety**: Introduced RAII-based metadata management and thread-local trackers to prevent leaks.
- **Modern Access**: Added `std::span` support for high-performance, bounds-checked metadata access.

## ⚠️ Stability & Crash Notes
While this version significantly improves reliability, please note:
- **Runtime Modification**: Modifying classes already in use by the Unity engine remains the most common cause of crashes. Always prefer modifying classes during early initialization.
- **Binary Obfuscation**: Highly protected or obfuscated binaries may still break the symbol scanning logic if standard instruction patterns (ADRP+ADD/LDR) are disrupted.
- **GC Integrity**: Ensure your custom `Finalize` methods call the base finalizer to prevent memory leaks, despite improvements in BNM's finalizer inheritance.

## Getting Started
See documentation on [ByNameModding.github.io](https://bynamemodding.github.io/).

## Supported Unity versions: 5.6.4f1, 2017.x - 6000.0.x

## Dependencies
[UTF8-CPP](https://github.com/nemtrif/utfcpp) used by il2cpp and by BNM too.<br>
[Open-hierarchy custom RTTI](https://github.com/royvandam/rtti/tree/cf0dee6fb3999573f45b0726a8d5739022e3dacf) used to optimize memory usage
### Android hookinng software for example:
[Dobby](https://github.com/ferly-chy/dobby) recommended<br>
[ShadowHook](https://github.com/bytedance/android-inline-hook)<br>
[Substrate](https://github.com/jbro129/Unity-Substrate-Hook-Android/tree/master/C%2B%2B/Substrate) with [And64InlineHook](https://github.com/Rprop/And64InlineHook) - do not support unhook
