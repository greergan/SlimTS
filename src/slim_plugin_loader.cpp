#include <dlfcn.h>
#include <filesystem>
#include <unordered_map>
#include <v8.h>
#include <slim/common/log.h>
#include <slim/path.h>
#include <slim/utilities.h>
#include <slim/plugin/loader.h>
namespace slim::plugin::loader {
    using namespace slim::common;
    std::string plugin_library_path = slim::path::getExecutableDir() + "/../lib/SlimTS/";
    std::unordered_map<std::string, void*> loaded_plugins;
}
void slim::plugin::loader::destroy() {
    log::trace(log::Message("slim::plugin::loader::destroy()", "begins", __FILE__, __LINE__));
    for(auto [plugin_name_string, plugin] : loaded_plugins) {
        log::debug(log::Message("slim::plugin::loader::destroy()", plugin_name_string.c_str(), __FILE__, __LINE__));
        dlclose(loaded_plugins[plugin_name_string]);
    }
    log::trace(log::Message("slim::plugin::loader::destroy()", "begins", __FILE__, __LINE__));
}
void slim::plugin::loader::load(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto package_name = slim::utilities::v8ValueToString(isolate, args[0]);
    log::trace(log::Message("slim::plugin::loader::load()", "begins, package name => " + package_name, __FILE__, __LINE__));
    v8::HandleScope scope(isolate);
    if(args.Length() == 0) {
        isolate->ThrowException(slim::utilities::StringToValue(isolate, "slim.load(package_name, [bool]"));
    }
    if(!args[0]->IsString()) {
        isolate->ThrowException(slim::utilities::StringToValue(isolate, "argument 1 must be a string"));
    }
    if(args.Length() > 1 && !args[1]->IsBoolean()) {
        isolate->ThrowException(slim::utilities::StringToValue(isolate, "argument 2 must be true or false"));
    }
    
    auto global_scope = args.Length() > 1 && args[1]->IsBoolean() ? args[1]->BooleanValue(isolate) : false;
    load_plugin(isolate, package_name, global_scope);
    log::trace(log::Message("slim::plugin::loader::load()", "ends, package name => " + package_name, __FILE__, __LINE__));
};
void slim::plugin::loader::load_plugin(v8::Isolate* isolate, const std::string plugin_name_string, const bool global_scope) {
    log::trace(log::Message("slim::plugin::loader::load_plugin()", "begins, plugin name => " + plugin_name_string, __FILE__, __LINE__));
    auto open_bits = global_scope ? RTLD_NOW | RTLD_GLOBAL : RTLD_NOW;
    std::string plugin_so_path = plugin_library_path + plugin_name_string + ".so";
    if(!std::filesystem::exists(plugin_so_path)) {
        std::string file_not_found_string = "error loading plugin name => " + plugin_name_string + " file not found => " + plugin_so_path;
        log::error(log::Message("slim::plugin::loader::load_plugin()", file_not_found_string, __FILE__, __LINE__));
        isolate->ThrowException(slim::utilities::StringToValue(isolate, file_not_found_string));
        return;
    }
    log::debug(log::Message("slim::plugin::loader::load_plugin()", "loading plugin => " + plugin_name_string + " from => " + plugin_so_path, __FILE__, __LINE__));
    loaded_plugins[plugin_name_string] = dlopen(plugin_so_path.c_str(), open_bits);
    if(!loaded_plugins[plugin_name_string]) {
        std::string error_string = dlerror();
        log::error(log::Message("slim::plugin::loader::load_plugin()", "error loading plugin => " + plugin_name_string + " error => " + error_string, __FILE__, __LINE__));
        isolate->ThrowException(slim::utilities::StringToValue(isolate, "error loading plugin name => " + plugin_name_string + " error => " + error_string));
        return;
    }
    else {
        typedef void (*expose_plugin_t)(v8::Isolate* isolate);
        expose_plugin_t expose_plugin = (expose_plugin_t) dlsym(loaded_plugins[plugin_name_string], "expose_plugin");
        if(!expose_plugin) {
            std::string error_string = dlerror();
            log::error(log::Message("slim::plugin::loader::load_plugin()", "error loading symbols for plugin => " + plugin_name_string + " error => " + error_string, __FILE__, __LINE__));
            isolate->ThrowException(slim::utilities::StringToValue(isolate, "error loading symbols for plugin name => " + plugin_name_string + " error => " + error_string));
            dlclose(loaded_plugins[plugin_name_string]);
            return;
        }
        else {
            log::debug(log::Message("slim::plugin::loader::load_plugin()", "exposing plugin => " + plugin_name_string, __FILE__, __LINE__));
            expose_plugin(isolate);
            expose_plugin = nullptr;
        }
    }
    log::trace(log::Message("slim::plugin::loader::load_plugin()", "ends, plugin name => " + plugin_name_string, __FILE__, __LINE__));
}