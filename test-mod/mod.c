/*******************************************************************************
 * File: mod
 * Project: KernelLoader
 * Created: 2026/5/4
 * Author: eternalfuture-e38299
 * Github: https://github.com/eternalfuture-e38299
 *
 * MIT License
 *
 * Copyright (c) 2026 eternalfuture-e38299
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *******************************************************************************/

#include "mod_core.h"
#include "mod_logger.h"
#include "stddef.h"

#include "patchlib/method.h"
#include "patchlib/field.h"

void (*mod_logger_write)(mod_log_level_t level, const char* tag, const char* fmt, ...) = NULL;


patch_handle_t useTime;
patch_handle_t useAnimation;
patch_handle_t damage;

void SetDefaults_postfix(patch_handle_t instance, void **args, void *result,
                                   const patch_method_signature_t *sig_info) {
    static int time = 5;
    mod_logger_write(MOD_LOG_LEVEL_INFO, "TestMod", "加速物品: %d", *(int*)args[0]);
    patchlib_field_set_value(useTime, instance, &time);
    patchlib_field_set_value(useAnimation, instance, &time);
}

static kernel_mod_info_t g_mod_info = {
    .pkg_id = "eternal.future.test_mod",
    .version_code = 1,
    .api_version = 1,
    .version = "1.0.0"
};

static void init_mod(kernel_mod_handle_t* handle) {
    if (mod_logger_write) {
        mod_logger_write(MOD_LOG_LEVEL_INFO, "TestMod", "init_mod called");
        mod_logger_write(MOD_LOG_LEVEL_INFO, "TestMod", "Private dir: %s",
                        handle->private_dir ? handle->private_dir : "NULL");
    }

    patch_handle_t item = patchlib_type_get_type("Terraria", "Item");
    useTime = patchlib_type_get_field(item, "useTime");
    useAnimation = patchlib_type_get_field(item, "useAnimation");
    damage = patchlib_type_get_field(item, "damage");
    patch_handle_t setDefaults = patchlib_type_get_method_by_param_count(item, "SetDefaults", 2);

    patchlib_install_prepost_hook(setDefaults, NULL, SetDefaults_postfix);
}

static void cleanup_mod(kernel_mod_handle_t* handle) {
    if (mod_logger_write) {
        mod_logger_write(MOD_LOG_LEVEL_INFO, "TestMod", "cleanup_mod called");
    }
}

static kernel_mod_info_t* get_info(void) {
    if (mod_logger_write) {
        mod_logger_write(MOD_LOG_LEVEL_INFO, "TestMod", "get_info called");
    }
    return &g_mod_info;
}

static kernel_mod_ops_t g_ops = {
    .init_mod = init_mod,
    .cleanup_mod = cleanup_mod,
    .get_info = get_info
};

kernel_mod_ops_t* create_kernel_mod(void) {
    if (mod_logger_write) {
        mod_logger_write(MOD_LOG_LEVEL_INFO, "TestMod", "create_kernel_mod called");
    }
    return &g_ops;
}
