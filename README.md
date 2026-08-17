# 🚀 KernelLoader - 基于 TEFKernel 的 ModLoader 实现
* [English](README-en.md)

KernelLoader 是一个基于 TEFKernel 框架开发的 ModLoader 实现，展示了如何利用 TEFKernel 的多 ModLoader 共存特性来构建现代化的模组加载系统。

---

## 📖 概述

### 什么是 KernelLoader？

KernelLoader 是一个**基于 TEFKernel API 构建的 ModLoader**，它展示了如何：

1. 实现 TEFKernel 的 `ml_ops_t` 接口
2. 从 TEF 包中加载和运行 Mod
3. 为 Mod 提供独立的日志系统
4. 管理 Mod 的生命周期（加载/卸载/重载/初始化）
5. 支持跨平台（Android/Windows/Linux/macOS）

### 技术亮点

| 特性                  | 说明                                               |
|:----------------------|:---------------------------------------------------|
| **🧩 TEFKernel 集成** | 完全符合 `ml_ops_t` 接口规范                       |
| **📦 内存动态库加载** | 使用 `memdl` 从内存加载 Mod 动态库                 |
| **🔌 符号共享**       | 通过 `tpf_register_shared_plugin_library` 共享符号 |
| **📝 独立日志系统**   | 基于 spdlog，支持 Mod 独立日志输出                 |
| **🎯 跨平台支持**     | 自动检测平台和架构，加载对应的动态库               |
| **📋 JSON 配置驱动**  | Mod 信息通过 JSON 配置文件声明                     |

---

## 🏗️ 架构设计

### 核心组件

```
KernelLoader/
├── core.hpp/cpp           # 核心逻辑 (实现 ml_ops_t)
├── logger.hpp/cpp         # 日志系统 (基于 spdlog)
├── mod-api/
│   ├── mod_core.h         # Mod 接口定义 (kernel_mod_ops_t)
│   └── mod_logger.h       # Mod 日志接口
└── dependencies/
├── tefkernel/         # TEFKernel 头文件
├── spdlog/            # 日志库
└── json.hpp           # JSON 解析库
```

---

## 📚 API 参考

### Mod 接口 (mod_core.h)

Mod 开发者需要实现以下接口：

```c
// Mod 信息结构
typedef struct kernel_mod_info_t {
    const char *pkg_id;      ///< 唯一包名
    int version_code;        ///< 版本代码
    int api_version;         ///< API 版本
    const char *version;     ///< 版本字符串
} kernel_mod_info_t;

// Mod 操作函数表
typedef struct kernel_mod_ops_t {
    void (*init_mod)(kernel_mod_handle_t* handle);      ///< 初始化 Mod
    void (*cleanup_mod)(kernel_mod_handle_t* handle);   ///< 清理 Mod
    kernel_mod_info_t* (*get_info)();                   ///< 获取信息
} kernel_mod_ops_t;

// Mod 句柄（由 KernelLoader 管理）
typedef struct kernel_mod_handle_t {
    char* private_dir;           ///< 私有数据目录
    kernel_mod_ops_t* ops;       ///< 操作表
    void* lib_handle;            ///< 动态库句柄
} kernel_mod_handle_t;

// 强制导出函数（Mod 必须实现）
MOD_API_EXPORT kernel_mod_ops_t* MOD_CALL_CONV create_kernel_mod();
```

### 日志接口 (mod_logger.h)

Mod 可以使用 KernelLoader 的日志系统：

```c
// 日志级别
typedef enum mod_log_level_t {
    MOD_LOG_LEVEL_TRACE,
    MOD_LOG_LEVEL_DEBUG,
    MOD_LOG_LEVEL_INFO,
    MOD_LOG_LEVEL_WARNING,
    MOD_LOG_LEVEL_ERROR,
    MOD_LOG_LEVEL_CRITICAL,
    MOD_LOG_LEVEL_FATAL
} mod_log_level_t;

// 全局日志函数指针（由 KernelLoader 注入）
extern void (MOD_CALL_CONV *mod_logger_write)(
    mod_log_level_t level,
    const char* tag,
    const char* fmt, ...
);
```

### TEFKernel ModLoader 接口实现

```cpp
namespace kernelloader::core {
    // 实现 ml_ops_t 的所有函数
    ml_result_t load_mod(mod_manifest_t* manifest);
    ml_result_t unload_mod(mod_manifest_t* manifest);
    ml_result_t reload_mod(mod_manifest_t* manifest);
    ml_result_t init_mod(mod_manifest_t* manifest);
    const multiplayer_mod_info_t* get_multiplayer_info(mod_manifest_t* manifest);
    ml_result_t init_ml(ml_entry_t* entry);
    ml_result_t cleanup_ml(ml_entry_t* entry);
    const ml_info_t* get_info();
}

// 导出的唯一函数
const ml_ops_t* API_CALL ml_create();
```

---

## 📝 Mod 开发指南

### 1. Mod 目录结构

```
my_kernel_mod/
├── mod.json                # Mod 配置文件
├── lib/                    # 动态库目录
│   ├── libmymod.android.arm64.so
│   ├── libmymod.android.arm.so
│   ├── libmymod.linux.x64.so
│   ├── libmymod.windows.x64.dll
│   └── libmymod.macos.x64.dylib
└── src/                    # 源代码
    └── my_mod.c
```

### 2. Mod 配置文件 (mod.json)

```json
{
    "lib_name": "mymod"
}
```

**配置项说明：**

| 字段           | 类型   | 说明                         |
|:---------------|:-------|:-----------------------------|
| `lib_name`     | string | 动态库名称（不含前缀和后缀） |

### 3. Mod 实现示例

```c
// my_mod.c
#include "mod-api/mod_core.h"
#include "mod-api/mod_logger.h"
#include <stdio.h>
#include <string.h>

// ============================================================
// 1. Mod 信息
// ============================================================
static kernel_mod_info_t g_info = {
    .pkg_id = "com.example.mymod",
    .version_code = 1,
    .api_version = 1,
    .version = "1.0.0"
};

// ============================================================
// 2. 生命周期函数
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
// 3. 导出操作表
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

### 4. 编译 Mod

```bash
# Linux / Android
gcc -shared -fPIC -I/path/to/KernelLoader my_mod.c -o libmymod.linux.x64.so

# Windows (MinGW)
gcc -shared -I/path/to/KernelLoader my_mod.c -o mymod.windows.x64.dll

# macOS
gcc -shared -fPIC -I/path/to/KernelLoader my_mod.c -o libmymod.macos.x64.dylib
```

---

## 📦 打包与部署

### 使用 TEFPkg-Tool 打包 KernelLoader

```bash
# 1. 编译 KernelLoader
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# 2. 准备打包目录
mkdir -p package
cp libkernelloader.*.so package/  # 复制所有平台的动态库

# 3. 创建配置文件
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

# 4. 打包
tefpkg_tool -c package/config.json -o eternal.future.kernelloader.tefpkg
```

### 部署到 TEFKernel

```
工作目录/
├── modloader/
│   ├── enables.txt              # 添加: eternal.future.kernelloader
│   └── pkg/
│       └── eternal.future.kernelloader.tefpkg
└── mods/
    └── eternal.future.kernelloader/
        ├── private/              # Mod 私有数据目录
        │   └── {mod_id}/
        ├── logs/                 # Mod 日志目录
        │   └── {mod_id}/
        ├── enables.txt           # 启用的 Mod 列表
        └── lib/                  # 存放 Mod 文件
            ├── com.example.mymod/
            │   ├── libmymod.linux.x64.so
            │   └── mod.json
            └── com.example.another/
                └── ...
```

---

## 🔄 工作流程

### Mod 加载流程

```
1. TEFKernel 调用 KernelLoader::load_mod()
   ↓
2. 读取 mod.json 获取 lib_name
   ↓
3. 构建平台相关动态库路径:
   lib/{lib_name}.{platform}.{arch}.{ext}
   ↓
4. 使用 memdl 从内存加载动态库
   ↓
5. 查找 create_kernel_mod() 符号
   ↓
6. 调用 create_kernel_mod() 获取 ops
   ↓
7. 通过 tpf_register_shared_plugin_library 注册符号
   ↓
8. 注入 mod_logger_write 函数指针
   ↓
9. 保存 mod_handle 到映射表
   ↓
10. 返回 ML_SUCCESS
```

### Mod 初始化流程

```
1. TEFKernel 调用 KernelLoader::init_mod()
   ↓
2. 从映射表查找 mod_handle
   ↓
3. 调用 mod->ops->init_mod(handle)
   ↓
4. Mod 执行初始化逻辑
   ↓
5. 返回 ML_SUCCESS
```

---

## 🛠️ 平台支持

| 平台    | 架构       | 动态库后缀 | 目录命名 |
|:--------|:-----------|:-----------|:---------|
| Android | arm64, arm | .so        | android  |
| Linux   | x64, x86   | .so        | linux    |
| Windows | x64, x86   | .dll       | windows  |
| macOS   | arm64, x64 | .dylib     | macos    |
| iOS     | arm64, x64 | .dylib     | ios      |

**自动检测逻辑：**

```cpp
#if defined(__ANDROID__)
    #define PLATFORM_NAME "android"
#elif defined(__linux__)
    #define PLATFORM_NAME "linux"
#elif defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_NAME "windows"
#elif defined(__APPLE__) && defined(__MACH__)
    // macOS / iOS 检测...
#endif
```

---

## 🔗 相关链接

- [KernelLoader GitHub](https://github.com/eternalfuture-e38299/KernelLoader)
- [TEFKernel GitHub](https://github.com/eternalfuture-e38299/tefkernel)
- [TEFPkg-Tool 官方仓库](https://github.com/eternalfuture-e38299/TEFPkg-Tool)

---

*基于 TEFKernel 的 ModLoader 实现* 🚀