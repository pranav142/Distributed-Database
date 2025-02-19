//
// Created by pknadimp on 2/12/25.
//

#include "logging.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

void utils::initialize_global_logging() {
  spdlog::set_level(spdlog::level::debug);
  spdlog::set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");
}
