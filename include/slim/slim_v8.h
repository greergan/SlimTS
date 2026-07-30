#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <v8.h>
#include <libplatform/libplatform.h>

namespace slim::v_8 {
    void dispose_isolate(std::string_view label);
    v8::Platform* get_platform();
    void initialize(std::vector<std::string>& v8_command_line_arguments);
    v8::Isolate* new_isolate(std::string label);
    void register_cleanup(v8::Isolate* isolate, std::function<void()> fn);
    void run_cleanup(v8::Isolate* isolate);
    void tear_down();
}
