# 🚀 KernelLoader - TEFKernel-based ModLoader Implementation

KernelLoader is a ModLoader implementation built on the TEFKernel framework, demonstrating how to leverage TEFKernel's multi-ModLoader coexistence feature to build a modern mod loading system.

---

## 📖 Overview

### What is KernelLoader?

KernelLoader is a **ModLoader built on the TEFKernel API** that demonstrates how to:

1. Implement the TEFKernel `ml_ops_t` interface
2. Load and run Mods from TEF packages
3. Provide independent logging systems for Mods
4. Manage Mod lifecycle (load/unload/reload/init)
5. Support cross-platform (Android/Windows/Linux/macOS)

### Technical Highlights

| Feature                                  | Description                                                                              |
|:-----------------------------------------|:-----------------------------------------------------------------------------------------|
| **🧩 TEFKernel Integration**             | Fully compliant with the `ml_ops_t` interface specification                              |
| **📦 In-Memory Dynamic Library Loading** | Uses `memdl` to load Mod dynamic libraries from memory                                   |
| **🔌 Symbol Sharing**                    | Shares symbols via `tpf_register_shared_plugin_library`                                  |
| **📝 Independent Logging System**        | Based on spdlog, supports independent log output for Mods                                |
| **🎯 Cross-Platform Support**            | Automatically detects platform and architecture, loads the corresponding dynamic library |
| **📋 JSON Configuration Driven**         | Mod information is declared via JSON configuration files                                 |

---

## 🏗️ Architecture Design

### Core Components

```
KernelLoader/
├── core.hpp/cpp           # Core logic (implements ml_ops_t)
├── logger.hpp/cpp         # Logging system (based on spdlog)
├── mod-api/
│   ├── mod_core.h         # Mod interface definitions (kernel_mod_ops_t)
│   └── mod_logger.h       # Mod logging interface
└── dependencies/
├── tefkernel/         # TEFKernel headers
├── spdlog/            # Logging library
└── json.hpp           # JSON parsing library
```

---

## 📚 API Reference

### Mod Interface (mod_core.h)

Mod developers need to implement the following interfaces:

```c
// Mod information structure
typedef struct kernel_mod_info_t {
    const char *pkg_id;      ///< Unique package name
    int version_code;        ///< Version code
    int api_version;         ///< API version
    const char *version;     ///< Version string
} kernel_mod_info_t;

// Mod operation function table
typedef struct kernel_mod_ops_t {
    void (*init_mod)(kernel_mod_handle_t* handle);      ///< Initialize Mod
    void (*cleanup_mod)(kernel_mod_handle_t* handle);   ///< Clean up Mod
    kernel_mod_info_t* (*get_info)();                   ///< Get information
} kernel_mod_ops_t;

// Mod handle (managed by KernelLoader)
typedef struct kernel_mod_handle_t {
    char* private_dir;           ///< Private data directory
    kernel_mod_ops_t* ops;       ///< Operation table
    void* lib_handle;            ///< Dynamic library handle
} kernel_mod_handle_t;

// Mandatory export function (must be implemented by Mod)
MOD_API_EXPORT kernel_mod_ops_t* MOD_CALL_CONV create_kernel_mod();
```

### Logging Interface (mod_logger.h)

Mods can use KernelLoader's logging system:

```c
// Log levels
typedef enum mod_log_level_t {
    MOD_LOG_LEVEL_TRACE,
    MOD_LOG_LEVEL_DEBUG,
    MOD_LOG_LEVEL_INFO,
    MOD_LOG_LEVEL_WARNING,
    MOD_LOG_LEVEL_ERROR,
    MOD_LOG_LEVEL_CRITICAL,
    MOD_LOG_LEVEL_FATAL
} mod_log_level_t;

// Global log function pointer (injected by KernelLoader)
extern void (MOD_CALL_CONV *mod_logger_write)(
    mod_log_level_t level,
    const char* tag,
    const char* fmt, ...
);
```

### TEFKernel ModLoader Interface Implementation

```cpp
namespace kernelloader::core {
    // Implements all ml_ops_t functions
    ml_result_t load_mod(mod_manifest_t* manifest);
    ml_result_t unload_mod(mod_manifest_t* manifest);
    ml_result_t reload_mod(mod_manifest_t* manifest);
    ml_result_t init_mod(mod_manifest_t* manifest);
    const multiplayer_mod_info_t* get_multiplayer_info(mod_manifest_t* manifest);
    ml_result_t init_ml(ml_entry_t* entry);
    ml_result_t cleanup_ml(ml_entry_t* entry);
    const ml_info_t* get_info();
}

// The only exported function
const ml_ops_t* API_CALL ml_create();
```

---

## 📝 Mod Development Guide

### 1. Mod Directory Structure

```
my_kernel_mod/
├── mod.json                # Mod configuration file
├── lib/                    # Dynamic library directory
│   ├── libmymod.android.arm64.so
│   ├── libmymod.android.arm.so
│   ├── libmymod.linux.x64.so
│   ├── libmymod.windows.x64.dll
│   └── libmymod.macos.x64.dylib
└── src/                    # Source code
    └── my_mod.c
```

### 2. Mod Configuration File (mod.json)

```json
{
    "lib_name": "mymod"
}
```

**Configuration Fields:**

| Field      | Type   | Description                                  |
|:-----------|:-------|:---------------------------------------------|
| `lib_name` | string | Dynamic library name (without prefix/suffix) |

### 3. Mod Implementation Example

```c
// my_mod.c
#include "mod-api/mod_core.h"
#include "mod-api/mod_logger.h"
#include <stdio.h>
#include <string.h>

// ============================================================
// 1. Mod Information
// ============================================================
static kernel_mod_info_t g_info = {
    .pkg_id = "com.example.mymod",
    .version_code = 1,
    .api_version = 1,
    .version = "1.0.0"
};

// ============================================================
// 2. Lifecycle Functions
// ============================================================
static void init_mod(kernel_mod_handle_t* handle) {
    mod_logger_write(MOD_LOG_LEVEL_INFO, "MyMod", 
                     "Initializing MyMod...");
    mod_logger_write(MOD_LOG_LEVEL_INFO, "MyMod", 
                     "Private directory: %s", handle->private_dir);
}

static void cleanup_mod(kernel_mod_handle_t* handle) {
    mod_logger_write(MOD_LOG_LEVEL_INFO, "MyMod", 
                     "Cleaning up MyMod...");
}

static kernel_mod_info_t* get_info() {
    return &g_info;
}

// ============================================================
// 3. Export Operation Table
// ============================================================
static kernel_mod_ops_t g_ops = {
    .init_mod = init_mod,
    .cleanup_mod = cleanup_mod,
    .get_info = get_info
};

MOD_API_EXPORT kernel_mod_ops_t* MOD_CALL_CONV create_kernel_mod() {
    return &g_ops;
}
```

### 4. Building a Mod

```bash
# Linux / Android
gcc -shared -fPIC -I/path/to/KernelLoader my_mod.c -o libmymod.linux.x64.so

# Windows (MinGW)
gcc -shared -I/path/to/KernelLoader my_mod.c -o mymod.windows.x64.dll

# macOS
gcc -shared -fPIC -I/path/to/KernelLoader my_mod.c -o libmymod.macos.x64.dylib
```

---

## 📦 Packaging and Deployment

### Packaging KernelLoader with TEFPkg-Tool

```bash
# 1. Build KernelLoader
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# 2. Prepare packaging directory
mkdir -p package
cp libkernelloader.*.so package/  # Copy dynamic libraries for all platforms

# 3. Create configuration file
cat > package/config.json << EOF
{
    "id": "eternal.future.kernelloader",
    "name": "KernelLoader",
    "author": "TEFKernel Team",
    "version": "1.0.0",
    "version_code": 1,
    "type": "modloader",
    "files": [
        {"path": "libkernelloader.android.arm64.so", "type": "dylib", "compress": true},
        {"path": "libkernelloader.linux.x64.so", "type": "dylib", "compress": true},
        {"path": "libkernelloader.windows.x64.dll", "type": "dylib", "compress": true}
    ]
}
EOF

# 4. Package
tefpkg_tool -c package/config.json -o eternal.future.kernelloader.tefpkg
```

### Deploying to TEFKernel

```
Working Directory/
├── modloader/
│   ├── enables.txt              # Add: eternal.future.kernelloader
│   └── pkg/
│       └── eternal.future.kernelloader.tefpkg
└── mods/
    └── eternal.future.kernelloader/
        ├── private/              # Mod private data directory
        │   └── {mod_id}/
        ├── logs/                 # Mod log directory
        │   └── {mod_id}/
        ├── enables.txt           # List of enabled Mods
        └── lib/                  # Mod file storage
            ├── com.example.mymod/
            │   ├── libmymod.linux.x64.so
            │   └── mod.json
            └── com.example.another/
                └── ...
```

---

## 🔄 Workflow

### Mod Loading Process

```
1. TEFKernel calls KernelLoader::load_mod()
   ↓
2. Read mod.json to get lib_name
   ↓
3. Build platform-specific dynamic library path:
   lib/{lib_name}.{platform}.{arch}.{ext}
   ↓
4. Use memdl to load dynamic library from memory
   ↓
5. Look up the create_kernel_mod() symbol
   ↓
6. Call create_kernel_mod() to get ops
   ↓
7. Register symbols via tpf_register_shared_plugin_library
   ↓
8. Inject mod_logger_write function pointer
   ↓
9. Save mod_handle to the mapping table
   ↓
10. Return ML_SUCCESS
```

### Mod Initialization Process

```
1. TEFKernel calls KernelLoader::init_mod()
   ↓
2. Look up mod_handle from the mapping table
   ↓
3. Call mod->ops->init_mod(handle)
   ↓
4. Mod executes initialization logic
   ↓
5. Return ML_SUCCESS
```

---

## 🛠️ Platform Support

| Platform | Architecture | Library Suffix | Directory Name |
|:---------|:-------------|:---------------|:---------------|
| Android  | arm64, arm   | .so            | android        |
| Linux    | x64, x86     | .so            | linux          |
| Windows  | x64, x86     | .dll           | windows        |
| macOS    | arm64, x64   | .dylib         | macos          |
| iOS      | arm64, x64   | .dylib         | ios            |

**Auto-Detection Logic:**

```cpp
#if defined(__ANDROID__)
    #define PLATFORM_NAME "android"
#elif defined(__linux__)
    #define PLATFORM_NAME "linux"
#elif defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_NAME "windows"
#elif defined(__APPLE__) && defined(__MACH__)
    // macOS / iOS detection...
#endif
```

---

## 🔗 Related Links

- [KernelLoader GitHub](https://github.com/eternalfuture-e38299/KernelLoader)
- [TEFKernel GitHub](https://github.com/eternalfuture-e38299/tefkernel)
- [TEFPkg-Tool Official Repository](https://github.com/eternalfuture-e38299/TEFPkg-Tool)

---

*TEFKernel-based ModLoader Implementation* 🚀