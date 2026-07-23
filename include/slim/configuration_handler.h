#pragma once
#include <slim/common/log.h>
namespace slim::configuration_handler {
    std::string get_script_name();
    bool is_daemon();
    void load();
}
