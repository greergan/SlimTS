#pragma once
#include <span>
#include <string>
#include <vector>
namespace slim::configuration_handler {
    std::span<std::string> library_path();
    std::string get_script_name();
    bool is_daemon();
    bool is_watching();
    void load();
}
