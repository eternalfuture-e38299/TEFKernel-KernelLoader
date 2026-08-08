/*******************************************************************************
 * KernelLoader - logger
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

#include "logger.hpp"
#include <spdlog/sinks/android_sink.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <cstdarg>

static spdlog::level::level_enum convert_log_level(const mod_log_level_t level) {
    switch (level) {
        case MOD_LOG_LEVEL_TRACE:    return spdlog::level::trace;
        case MOD_LOG_LEVEL_DEBUG:    return spdlog::level::debug;
        case MOD_LOG_LEVEL_INFO:     return spdlog::level::info;
        case MOD_LOG_LEVEL_WARNING:  return spdlog::level::warn;
        case MOD_LOG_LEVEL_ERROR:    return spdlog::level::err;
        case MOD_LOG_LEVEL_CRITICAL: return spdlog::level::critical;
        case MOD_LOG_LEVEL_FATAL:    return spdlog::level::critical;
        default:                     return spdlog::level::info;
    }
}

void kernelloader::logger::init() {
    try {
        if (auto existing_logger = spdlog::get("KernelLoader")) {
            return;
        }

        std::vector<spdlog::sink_ptr> sinks;


#ifdef __ANDROID__
        // Android平台使用android_sink
        const auto android_sink = std::make_shared<spdlog::sinks::android_sink_mt>("KernelLoader");
        android_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v%$");
        sinks.push_back(android_sink);
#else
        // 其他平台使用标准输出（支持颜色）
        const auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        stdout_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v%$");
        sinks.push_back(stdout_sink);
#endif

        /*
        if (!filename.empty()) {
            const auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] [%t] %v");
            sinks.push_back(file_sink);
        }
        */

        auto logger = std::make_shared<spdlog::logger>("KernelLoader", begin(sinks), end(sinks));
#if  !defined(NDEBUG)
        logger->set_level(spdlog::level::trace);
#else
        logger->set_level(spdlog::level::info);
#endif

        if (!spdlog::get("KernelLoader")) {
            spdlog::register_logger(logger);
            spdlog::set_default_logger(logger);
        } else {
            logger = spdlog::get("KernelLoader");
        }

        logger->flush_on(spdlog::level::info);
        g_logger = logger;

        LOG_INFO("New logger initialized successfully.");

    } catch (const spdlog::spdlog_ex& ex) {
        spdlog::error("Logger initialization failed: {}", ex.what());
        throw;
    }
}

void kernelloader::logger::shutdown() {
    LOG_INFO("Shutting down logger...");

    g_logger->flush();
    spdlog::drop("KernelLoader");
    spdlog::shutdown();
}

void kernelloader::logger::init_mod() {
    try {
        if (auto existing_logger = spdlog::get("KernelLoaderMod")) {
            return;
        }

        std::vector<spdlog::sink_ptr> sinks;

#ifdef __ANDROID__
        // Android平台使用android_sink
        const auto android_sink = std::make_shared<spdlog::sinks::android_sink_mt>("KernelLoaderMod");
        android_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v%$");
        sinks.push_back(android_sink);
#else
        // 其他平台使用标准输出（支持颜色）
        const auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        stdout_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v%$");
        sinks.push_back(stdout_sink);
#endif


        auto logger = std::make_shared<spdlog::logger>("KernelLoaderMod", begin(sinks), end(sinks));
        logger->set_level(spdlog::level::trace);

        if (!spdlog::get("KernelLoaderMod")) {
            spdlog::register_logger(logger);
            spdlog::set_default_logger(logger);
        } else {
            logger = spdlog::get("KernelLoaderMod");
        }

        logger->flush_on(spdlog::level::info);
        g_logger = logger;

        LOG_INFO("New logger initialized successfully.");

    } catch (const spdlog::spdlog_ex& ex) {
        spdlog::error("Logger initialization failed: {}", ex.what());
        throw;
    }
}

void kernelloader::logger::shutdown_mod() {
    g_mod_logger->flush();
    spdlog::drop("KernelLoaderMod");
}

void kernelloader::logger::mod_logger_write_imp(const mod_log_level_t level, const char *tag, const char *fmt, ...) {
    if (!fmt) return;

    // 获取模块日志器
    auto logger = g_mod_logger;
    if (!logger) {
        logger = g_logger;
        if (!logger) return;
    }

    // 格式化消息
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // 构建带tag的消息
    std::string message;
    if (tag && tag[0] != '\0') {
        message = std::string("[") + tag + "] " + buffer;
    } else {
        message = buffer;
    }

    // 输出日志
    switch (convert_log_level(level)) {
        case spdlog::level::trace:
            logger->trace(message);
            break;
        case spdlog::level::debug:
            logger->debug(message);
            break;
        case spdlog::level::info:
            logger->info(message);
            break;
        case spdlog::level::warn:
            logger->warn(message);
            break;
        case spdlog::level::err:
            logger->error(message);
            break;
        case spdlog::level::critical:
            logger->critical(message);
            break;
        default:
            logger->info(message);
            break;
    }

    // 致命错误立即刷新
    if (level == MOD_LOG_LEVEL_FATAL) {
        logger->flush();
    }
}