/*******************************************************************************
 * KernelLoader - core
 * Copyright (C) 2026 eternalfuture-e38299
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Author: eternalfuture-e38299
 * GitHub: https://github.com/eternalfuture-e38299
 * Created: 2026/5/1
 *******************************************************************************/

#include "core.hpp"
#include "json.hpp"

#include <fstream>

#include "logger.hpp"
#include "tefkernel/memdl/memdl.h"
#include "tefkernel/tefplugin/tpf_core.h"

#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_NAME "windows"
#define DYLIB_EXT ".dll"
#elif defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
#define PLATFORM_NAME "ios"
#define DYLIB_EXT ".dylib"
#else
#define PLATFORM_NAME "macos"
#define DYLIB_EXT ".dylib"
#endif
#elif defined(__ANDROID__)
#define PLATFORM_NAME "android"
#define DYLIB_EXT ".so"
#elif defined(__linux__)
#define PLATFORM_NAME "linux"
#define DYLIB_EXT ".so"
#else
#define PLATFORM_NAME "Unknown"
#define DYLIB_EXT ""
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define ARCH_NAME "x64"
#elif defined(__i386__) || defined(_M_IX86)
#define ARCH_NAME "x86"
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARCH_NAME "arm64"
#elif defined(__arm__) || defined(_M_ARM)
#define ARCH_NAME "arm"
#else
#define ARCH_NAME "unknown"
#endif


const ml_ops_t * API_CALL ml_create() {
    static ml_ops_t ml_ops{};

    ml_ops.cleanup_ml = kernelloader::core::cleanup_ml;
    ml_ops.get_info = kernelloader::core::get_info;
    ml_ops.get_multiplayer_info = kernelloader::core::get_multiplayer_info;
    ml_ops.init_ml = kernelloader::core::init_ml;
    ml_ops.init_mod = kernelloader::core::init_mod;
    ml_ops.load_mod = kernelloader::core::load_mod;
    ml_ops.reload_mod = kernelloader::core::reload_mod;
    ml_ops.unload_mod = kernelloader::core::unload_mod;

    return &ml_ops;
}

ml_result_t kernelloader::core::load_mod(mod_manifest_t *mod_manifest) {
    LOG_INFO("Starting to load mod: mod_id={}, path={}, private_dir={}",
             mod_manifest->mod_id, mod_manifest->path, mod_manifest->private_dir);

    auto mod_handle = new kernel_mod_handle_t;
    auto mod_multiplayer_mod_infos = new multiplayer_mod_info_t;

    try {
        // 检查文件是否存在
        std::filesystem::path mod_path(mod_manifest->path);
        LOG_DEBUG("Checking if mod file exists: {}", mod_path.string());

        if (!std::filesystem::exists(mod_path)) {
            LOG_ERROR("Mod file not found: {}", mod_path.string());
            delete mod_handle;
            delete mod_multiplayer_mod_infos;
            return ML_ERROR_NOT_FOUND;
        }
        LOG_DEBUG("Mod file exists: {}", mod_path.string());

        // 解析 JSON 配置文件
        LOG_DEBUG("Parsing JSON configuration from: {}", mod_path.string());
        std::ifstream mod_file(mod_path);
        auto json_data = nlohmann::json::parse(mod_file);
        auto lib_name = json_data["lib_name"].get<std::string>();
        LOG_INFO("Found lib_name in config: {}", lib_name);

        // 构建动态库路径
        std::filesystem::path lib_path = mod_manifest->private_dir;
        lib_path /= "lib";
        LOG_DEBUG("Base library path: {}", lib_path.string());

        // 构建文件名
        std::string filename;

        // 非 Windows 平台添加 lib 前缀
#if !defined(_WIN32) && !defined(_WIN64)
        filename = "lib";
#endif

        filename += lib_name;
        filename += "." + std::string(PLATFORM_NAME);
        filename += "." + std::string(ARCH_NAME);
        filename += DYLIB_EXT;

        lib_path /= filename;
        LOG_DEBUG("Target library path with platform/arch: {}", lib_path.string());

        // 检查动态库文件是否存在
        if (!std::filesystem::exists(lib_path)) {
            LOG_WARN("Platform-specific library not found: {}", lib_path.string());

            // 也可以尝试不包含架构的平台通用版本
            std::filesystem::path fallback_path = mod_manifest->private_dir;
            fallback_path /= "lib";

            std::string fallback_filename;
#if !defined(_WIN32) && !defined(_WIN64)
            fallback_filename = "lib";
#endif
            fallback_filename += lib_name;
            fallback_filename += "." + std::string(PLATFORM_NAME);
            fallback_filename += DYLIB_EXT;

            fallback_path /= fallback_filename;
            LOG_DEBUG("Trying fallback library path: {}", fallback_path.string());

            if (std::filesystem::exists(fallback_path)) {
                LOG_INFO("Using fallback library: {}", fallback_path.string());
                lib_path = fallback_path;
            } else {
                LOG_ERROR("No valid library found for mod: {}", mod_manifest->mod_id);
                delete mod_handle;
                delete mod_multiplayer_mod_infos;
                return ML_ERROR_NOT_FOUND;
            }
        }

        LOG_INFO("Loading library from: {}", lib_path.string());

        // 打开动态库文件
        LOG_DEBUG("Opening library file: {}", lib_path.string());
        std::ifstream lib_file(lib_path, std::ios::binary | std::ios::ate);
        if (!lib_file.is_open()) {
            LOG_ERROR("Failed to open library file: {}", lib_path.string());
            delete mod_handle;
            delete mod_multiplayer_mod_infos;
            return ML_ERROR_NOT_FOUND;
        }

        // 获取文件大小
        std::streamsize lib_size = lib_file.tellg();
        lib_file.seekg(0, std::ios::beg);
        LOG_DEBUG("Library file size: {} bytes", lib_size);

        // 读取整个文件到内存
        std::vector<char> buffer(lib_size);
        if (!lib_file.read(buffer.data(), lib_size)) {
            LOG_ERROR("Failed to read library file: {}", lib_path.string());
            delete mod_handle;
            delete mod_multiplayer_mod_infos;
            return ML_ERROR;
        }
        lib_file.close();
        LOG_DEBUG("Library file read successfully into memory");

        // 验证动态库文件
        LOG_DEBUG("Validating dynamic library...");
        int valid = memdl_validate(buffer.data(), buffer.size());
        if (valid != 0) {
            LOG_ERROR("Library validation failed with code: {}", valid);
            delete mod_handle;
            delete mod_multiplayer_mod_infos;
            return ML_ERROR_INVALID_PARAM;
        }
        LOG_DEBUG("Library validation passed");

        // 从内存加载动态库
        LOG_DEBUG("Loading library from memory...");
        int flags = MEMDL_NOW;
        memdl_handle_t handle = memdl_open(buffer.data(), buffer.size(), flags);
        if (!handle) {
            LOG_ERROR("Failed to open library from memory");
            delete mod_handle;
            delete mod_multiplayer_mod_infos;
            return ML_ERROR;
        }
        LOG_DEBUG("Library loaded successfully, handle: {}", fmt::ptr(handle));

        auto create_mod = reinterpret_cast<kernel_mod_ops_t*(*)()>(memdl_sym(handle, "create_kernel_mod"));
        if (!create_mod) {
            LOG_ERROR("Failed to find symbol 'create_kernel_mod' in library");
            delete mod_handle;
            delete mod_multiplayer_mod_infos;
            memdl_close(handle);
            return ML_ERROR;
        }
        LOG_DEBUG("Found create_kernel_mod symbol");

        auto ops = create_mod();
        if (!ops) {
            LOG_ERROR("create_kernel_mod returned null");
            delete mod_handle;
            delete mod_multiplayer_mod_infos;
            memdl_close(handle);
            return ML_ERROR;
        }
        LOG_DEBUG("Mod operations created successfully");

        tpf_register_shared_plugin_library(handle);
        LOG_DEBUG("Library registered with TPF");

        mod_handle->private_dir = strdup(mod_manifest->private_dir);
        mod_handle->lib_handle = handle;
        mod_handle->ops = ops;

        mod_multiplayer_mod_infos->mod_id = strdup(mod_manifest->mod_id);
        mod_multiplayer_mod_infos->is_multiplayer_safe = true;
        mod_multiplayer_mod_infos->version = ops->get_info()->version;
        mod_multiplayer_mod_infos->version_code = ops->get_info()->version_code;

        LOG_INFO("Mod info - Version: {}, Version Code: {}",
                 mod_multiplayer_mod_infos->version, mod_multiplayer_mod_infos->version_code);

        kernel_mod_handles.emplace(mod_manifest->mod_id, mod_handle);
        kernel_mod_multiplayer_mod_infos.emplace(mod_manifest->mod_id, mod_multiplayer_mod_infos);

        *static_cast<void **>(memdl_sym(handle, "mod_logger_write")) = reinterpret_cast<void*>(logger::mod_logger_write_imp);
        LOG_DEBUG("Logger function pointer set in mod");

        LOG_INFO("Mod loaded successfully: {}", mod_manifest->mod_id);
        return ML_SUCCESS;

    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR("Filesystem error while loading mod {}: {}", mod_manifest->mod_id, e.what());
        delete mod_handle;
        delete mod_multiplayer_mod_infos;
        return ML_ERROR;
    } catch (const nlohmann::json::parse_error& e) {
        LOG_ERROR("JSON parse error while loading mod {}: {}", mod_manifest->mod_id, e.what());
        delete mod_handle;
        delete mod_multiplayer_mod_infos;
        return ML_ERROR;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception while loading mod {}: {}", mod_manifest->mod_id, e.what());
        delete mod_handle;
        delete mod_multiplayer_mod_infos;
        return ML_ERROR;
    }
}

ml_result_t kernelloader::core::unload_mod(mod_manifest_t *mod_manifest) {
    LOG_INFO("Unloading mod: {}", mod_manifest->mod_id);

    auto it = kernel_mod_handles.find(mod_manifest->mod_id);
    if (it == kernel_mod_handles.end()) {
        LOG_WARN("Mod not found in handles map: {}", mod_manifest->mod_id);
        return ML_ERROR_NOT_FOUND;
    }

    const auto mod = it->second;
    const auto mod_multiplayer_mod_info = kernel_mod_multiplayer_mod_infos[mod_manifest->mod_id];

    LOG_DEBUG("Cleaning up mod operations...");
    mod->ops->cleanup_mod(mod);

    LOG_DEBUG("Closing library handle...");
    memdl_close(mod->lib_handle);

    LOG_DEBUG("Freeing private_dir: {}", mod->private_dir);
    free(mod->private_dir);

    LOG_DEBUG("Freeing multiplayer mod info...");
    free((void*)mod_multiplayer_mod_info->mod_id);
    delete mod_multiplayer_mod_info;
    delete mod;

    kernel_mod_handles.erase(it);
    kernel_mod_multiplayer_mod_infos.erase(mod_manifest->mod_id);

    LOG_INFO("Mod unloaded successfully: {}", mod_manifest->mod_id);
    return ML_SUCCESS;
}

ml_result_t kernelloader::core::reload_mod(mod_manifest_t *mod_manifest) {
    LOG_INFO("Reloading mod: {}", mod_manifest->mod_id);

    auto it = kernel_mod_handles.find(mod_manifest->mod_id);
    if (it != kernel_mod_handles.end()) {
        LOG_DEBUG("Mod exists, unloading first...");
        unload_mod(mod_manifest);
        LOG_DEBUG("Loading mod again...");
        load_mod(mod_manifest);
    } else {
        LOG_DEBUG("Mod not found, loading new instance...");
        load_mod(mod_manifest);
    }

    LOG_INFO("Mod reloaded successfully: {}", mod_manifest->mod_id);
    return ML_SUCCESS;
}

ml_result_t kernelloader::core::init_mod(mod_manifest_t *mod_manifest) {
    LOG_INFO("Initializing mod: {}", mod_manifest->mod_id);

    auto it = kernel_mod_handles.find(mod_manifest->mod_id);
    if (it == kernel_mod_handles.end()) {
        LOG_ERROR("Cannot initialize mod - not found in handles: {}", mod_manifest->mod_id);
        return ML_ERROR_NOT_FOUND;
    }

    const auto mod = it->second;
    LOG_DEBUG("Calling mod's init_mod function...");
    mod->ops->init_mod(mod);

    LOG_INFO("Mod initialized successfully: {}", mod_manifest->mod_id);
    return ML_SUCCESS;
}

const multiplayer_mod_info_t * kernelloader::core::get_multiplayer_info(mod_manifest_t *mod_manifest) {
    LOG_DEBUG("Getting multiplayer info for mod: {}", mod_manifest->mod_id);

    auto it = kernel_mod_multiplayer_mod_infos.find(mod_manifest->mod_id);
    if (it == kernel_mod_multiplayer_mod_infos.end()) {
        LOG_WARN("Multiplayer info not found for mod: {}", mod_manifest->mod_id);
        return nullptr;
    }

    return it->second;
}

ml_result_t kernelloader::core::init_ml(ml_entry_t *ml_entry) {
    LOG_INFO("Initializing KernelLoader...");

    LOG_DEBUG("Platform: {}, Architecture: {}", PLATFORM_NAME, ARCH_NAME);

    LOG_DEBUG("Initializing logger...");
    logger::init();

    LOG_DEBUG("Initializing mod logger...");
    logger::init_mod();

    LOG_INFO("KernelLoader initialized successfully");
    return ML_SUCCESS;
}

ml_result_t kernelloader::core::cleanup_ml(ml_entry_t *ml_entry) {
    LOG_INFO("Cleaning up KernelLoader...");

    LOG_DEBUG("Shutting down logger...");
    logger::shutdown();

    LOG_DEBUG("Shutting down mod logger...");
    logger::shutdown_mod();

    LOG_INFO("KernelLoader cleaned up successfully");
    return ML_SUCCESS;
}

const ml_info_t * kernelloader::core::get_info() {
    static ml_info_t info = {
        "eternal.future.kernelloader",
        1,
        "1.0.0",
        1,
        0,
        nullptr
    };

    return &info;
}
