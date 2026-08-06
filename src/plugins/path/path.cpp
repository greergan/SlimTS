#include <filesystem>
#include <format>
#include <string>
#include <v8.h>
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/plugin.hpp>
#include <slim/utilities.h>

namespace slim::common {}
namespace slim::plugin::path {
    using namespace slim;
    using namespace slim::common;

    void basename(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        if(args.Length() < 1 || !args[0]->IsString()) {
            isolate->ThrowException(utilities::StringToV8String(isolate, "path.basename: expected a string argument"));
            return;
        }
        std::string path_string = utilities::v8StringToString(isolate, args[0].As<v8::String>());
        std::string ext;
        if(args.Length() >= 2 && args[1]->IsString()) {
            ext = utilities::v8StringToString(isolate, args[1].As<v8::String>());
        }
        std::filesystem::path p(path_string);
        std::string result = p.filename().string();
        if(!ext.empty() && result.ends_with(ext)) {
            result = result.substr(0, result.size() - ext.size());
        }
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("basename => {}", result), __FILE__, __LINE__});
#endif
        args.GetReturnValue().Set(utilities::StringToV8String(isolate, result));
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void dirname(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        if(args.Length() < 1 || !args[0]->IsString()) {
            isolate->ThrowException(utilities::StringToV8String(isolate, "path.dirname: expected a string argument"));
            return;
        }
        std::string path_string = utilities::v8StringToString(isolate, args[0].As<v8::String>());
        std::string result = std::filesystem::path(path_string).parent_path().string();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("dirname => {}", result), __FILE__, __LINE__});
#endif
        args.GetReturnValue().Set(utilities::StringToV8String(isolate, result));
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void extname(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        if(args.Length() < 1 || !args[0]->IsString()) {
            isolate->ThrowException(utilities::StringToV8String(isolate, "path.extname: expected a string argument"));
            return;
        }
        std::string path_string = utilities::v8StringToString(isolate, args[0].As<v8::String>());
        std::string result = std::filesystem::path(path_string).extension().string();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("extname => {}", result), __FILE__, __LINE__});
#endif
        args.GetReturnValue().Set(utilities::StringToV8String(isolate, result));
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void join(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        std::filesystem::path result;
        for(int i = 0; i < args.Length(); i++) {
            if(!args[i]->IsString()) {
                isolate->ThrowException(utilities::StringToV8String(isolate, "path.join: all arguments must be strings"));
                return;
            }
            result /= utilities::v8StringToString(isolate, args[i].As<v8::String>());
        }
        std::string result_string = result.lexically_normal().string();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("join => {}", result_string), __FILE__, __LINE__});
#endif
        args.GetReturnValue().Set(utilities::StringToV8String(isolate, result_string));
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void resolve(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        std::filesystem::path result = std::filesystem::current_path();
        for(int i = 0; i < args.Length(); i++) {
            if(!args[i]->IsString()) {
                isolate->ThrowException(utilities::StringToV8String(isolate, "path.resolve: all arguments must be strings"));
                return;
            }
            std::filesystem::path segment(utilities::v8StringToString(isolate, args[i].As<v8::String>()));
            if(segment.is_absolute()) {
                result = segment;
            }
            else {
                result /= segment;
            }
        }
        std::string result_string = result.lexically_normal().string();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("resolve => {}", result_string), __FILE__, __LINE__});
#endif
        args.GetReturnValue().Set(utilities::StringToV8String(isolate, result_string));
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void normalize(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        if(args.Length() < 1 || !args[0]->IsString()) {
            isolate->ThrowException(utilities::StringToV8String(isolate, "path.normalize: expected a string argument"));
            return;
        }
        std::string path_string = utilities::v8StringToString(isolate, args[0].As<v8::String>());
        std::string result = std::filesystem::path(path_string).lexically_normal().string();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("normalize => {}", result), __FILE__, __LINE__});
#endif
        args.GetReturnValue().Set(utilities::StringToV8String(isolate, result));
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void is_absolute(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        if(args.Length() < 1 || !args[0]->IsString()) {
            isolate->ThrowException(utilities::StringToV8String(isolate, "path.isAbsolute: expected a string argument"));
            return;
        }
        std::string path_string = utilities::v8StringToString(isolate, args[0].As<v8::String>());
        bool result = std::filesystem::path(path_string).is_absolute();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("isAbsolute => {}", result), __FILE__, __LINE__});
#endif
        args.GetReturnValue().Set(v8::Boolean::New(isolate, result));
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void relative(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        if(args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
            isolate->ThrowException(utilities::StringToV8String(isolate, "path.relative: expected two string arguments"));
            return;
        }
        std::string from_string = utilities::v8StringToString(isolate, args[0].As<v8::String>());
        std::string to_string = utilities::v8StringToString(isolate, args[1].As<v8::String>());
        std::string result = std::filesystem::path(to_string).lexically_relative(from_string).string();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("relative => {}", result), __FILE__, __LINE__});
#endif
        args.GetReturnValue().Set(utilities::StringToV8String(isolate, result));
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void parse(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        if(args.Length() < 1 || !args[0]->IsString()) {
            isolate->ThrowException(utilities::StringToV8String(isolate, "path.parse: expected a string argument"));
            return;
        }
        std::string path_string = utilities::v8StringToString(isolate, args[0].As<v8::String>());
        std::filesystem::path p(path_string);
        auto context = isolate->GetCurrentContext();
        auto result = v8::Object::New(isolate);
        result->Set(context, utilities::StringToV8String(isolate, "root"), utilities::StringToV8String(isolate, p.root_path().string())).Check();
        result->Set(context, utilities::StringToV8String(isolate, "dir"), utilities::StringToV8String(isolate, p.parent_path().string())).Check();
        result->Set(context, utilities::StringToV8String(isolate, "base"), utilities::StringToV8String(isolate, p.filename().string())).Check();
        result->Set(context, utilities::StringToV8String(isolate, "ext"), utilities::StringToV8String(isolate, p.extension().string())).Check();
        result->Set(context, utilities::StringToV8String(isolate, "name"), utilities::StringToV8String(isolate, p.stem().string())).Check();
        args.GetReturnValue().Set(result);
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void format(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        if(args.Length() < 1 || !args[0]->IsObject()) {
            isolate->ThrowException(utilities::StringToV8String(isolate, "path.format: expected an object argument"));
            return;
        }
        auto obj = args[0].As<v8::Object>();
        std::string dir = utilities::StringValue(isolate, "dir", obj);
        std::string root = utilities::StringValue(isolate, "root", obj);
        std::string base = utilities::StringValue(isolate, "base", obj);
        std::string name = utilities::StringValue(isolate, "name", obj);
        std::string ext = utilities::StringValue(isolate, "ext", obj);
        std::filesystem::path result;
        if(!dir.empty()) result = dir;
        else result = root;
        if(!base.empty()) result /= base;
        else result /= (name + ext);
        std::string result_string = result.string();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("format => {}", result_string), __FILE__, __LINE__});
#endif
        args.GetReturnValue().Set(utilities::StringToV8String(isolate, result_string));
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }
}

extern "C" void expose_plugin(v8::Isolate* isolate) {
    slim::plugin::plugin path_plugin(isolate, "path");
    path_plugin.add_function("basename", slim::plugin::path::basename);
    path_plugin.add_function("dirname", slim::plugin::path::dirname);
    path_plugin.add_function("extname", slim::plugin::path::extname);
    path_plugin.add_function("join", slim::plugin::path::join);
    path_plugin.add_function("resolve", slim::plugin::path::resolve);
    path_plugin.add_function("normalize", slim::plugin::path::normalize);
    path_plugin.add_function("isAbsolute", slim::plugin::path::is_absolute);
    path_plugin.add_function("relative", slim::plugin::path::relative);
    path_plugin.add_function("parse", slim::plugin::path::parse);
    path_plugin.add_function("format", slim::plugin::path::format);
    path_plugin.add_property_immutable("sep", "/");
    path_plugin.add_property_immutable("delimiter", ":");
    path_plugin.expose_plugin();
}
