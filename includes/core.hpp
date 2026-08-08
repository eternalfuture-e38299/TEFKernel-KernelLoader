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

#pragma once

#include <string>
#include <unordered_map>
#include "../mod-api/mod_core.h"
#include "tefkernel/modloader/modloader_core.h"

namespace kernelloader::core {
    inline std::unordered_map<std::string, kernel_mod_handle_t*> kernel_mod_handles{};
    inline std::unordered_map<std::string, multiplayer_mod_info_t*> kernel_mod_multiplayer_mod_infos{};

    ml_result_t load_mod(mod_manifest_t *mod_manifest);

    ml_result_t unload_mod(mod_manifest_t *mod_manifest);

    ml_result_t reload_mod(mod_manifest_t *mod_manifest);

    ml_result_t init_mod(mod_manifest_t *mod_manifest);

    const multiplayer_mod_info_t *get_multiplayer_info(mod_manifest_t *mod_manifest);

    ml_result_t init_ml(ml_entry_t *ml_entry);

    ml_result_t cleanup_ml(ml_entry_t *ml_entry);

    const ml_info_t* get_info();
}