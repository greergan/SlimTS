#pragma once
#include <v8.h>
#include <string_view>

namespace slim::plugin::loader {
    void load(const v8::FunctionCallbackInfo<v8::Value>& args);
    void load_plugin(v8::Isolate* isolate, std::string plugin_name, bool global_scope);
}
