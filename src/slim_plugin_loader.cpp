#include <dlfcn.h>
#include <filesystem>
#include <format>
#include <string_view>
#include <unordered_map>
#include <v8.h>
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/path.h>
#include <slim/utilities.h>
#include <slim/plugin/loader.h>

namespace slim::common {}
namespace slim::plugin::loader {
    using namespace slim;
    using namespace slim::common;
    std::string plugin_library_path = slim::path::getExecutableDir() + "/../lib/SlimTS/";
    std::unordered_map<std::string, void*> loaded_plugins;
}

void slim::plugin::loader::load(const v8::FunctionCallbackInfo<v8::Value>& args) {
    #ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
    #endif
    auto isolate = args.GetIsolate();
    auto plugin_name = utilities::v8ValueToString(isolate, args[0]);
    #ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("loading => {}", plugin_name), __FILE__, __LINE__});
    #endif
    v8::HandleScope scope(isolate);
    if(args.Length() == 0) {
        isolate->ThrowException(utilities::StringToValue(isolate, "slim.load(plugin_name, [bool]"));
    }
    if(!args[0]->IsString()) {
        isolate->ThrowException(utilities::StringToValue(isolate, "argument 1 must be a string"));
    }
    if(args.Length() > 1 && !args[1]->IsBoolean()) {
        isolate->ThrowException(utilities::StringToValue(isolate, "argument 2 must be true or false"));
    }

    auto global_scope = args.Length() > 1 && args[1]->IsBoolean() ? args[1]->BooleanValue(isolate) : false;
    load_plugin(isolate, plugin_name, global_scope);
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
};
void slim::plugin::loader::load_plugin(v8::Isolate* isolate, std::string plugin_name, bool global_scope) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
    log::debug({__func__, std::format("loading => {}", plugin_name), __FILE__, __LINE__});
#endif
    auto open_bits = global_scope ? RTLD_NOW | RTLD_GLOBAL : RTLD_NOW;
    std::string plugin_so_path = std::format("{}{}.so", plugin_library_path, plugin_name);
    if(!std::filesystem::exists(plugin_so_path)) {
#ifdef ENABLE_LOGGING
        log::error({__func__, std::format("plugin not found => {}", plugin_so_path), __FILE__, __LINE__});
#endif
        isolate->ThrowException(utilities::StringToValue(isolate, std::format("error loading plugin => {}", plugin_so_path)));
        return;
    }
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("calling dlopen => {}", plugin_so_path), __FILE__, __LINE__});
#endif
    loaded_plugins[plugin_name] = dlopen(plugin_so_path.c_str(), open_bits);
    if(!loaded_plugins[plugin_name]) {
        std::string error_string = dlerror();
#ifdef ENABLE_LOGGING
        log::error({__func__, std::format("dlopen failed => {}", error_string), __FILE__, __LINE__});
#endif
        isolate->ThrowException(utilities::StringToValue(isolate, std::format("error loading plugin => {}", error_string)));
        return;
    }
    else {
        typedef void (*expose_plugin_t)(v8::Isolate* isolate);
        expose_plugin_t expose_plugin = (expose_plugin_t) dlsym(loaded_plugins[plugin_name], "expose_plugin");
        if(!expose_plugin) {
            std::string error_string = dlerror();
#ifdef ENABLE_LOGGING
            log::error({__func__, std::format("dlsym failed => {} => error => {}", plugin_name, error_string), __FILE__, __LINE__});
#endif
            isolate->ThrowException(utilities::StringToValue(isolate, std::format("error loading symbols => {}", error_string)));
            dlclose(loaded_plugins[plugin_name]);
            return;
        }
        else {
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("exposing plugin => {}", plugin_name), __FILE__, __LINE__});
#endif
            expose_plugin(isolate);
            expose_plugin = nullptr;
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("exposed plugin => {}", plugin_name), __FILE__, __LINE__});
#endif
        }
    }
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
