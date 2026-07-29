#pragma once
#include <string>
#include <string_view>
#include <slim/common/log.h>
namespace slim::configuration_handler {
    std::string get_script_name();
    bool is_daemon();
    bool is_watching();
    void load();
}
