#define ENABLE_LOGGING
#include <format>
#include <string>
#include <v8.h>
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/module/import_specifier.h>
#include <slim/plugin.hpp>
#include <slim/utilities.h>
namespace slim::common {}
namespace slim::plugin::require {
    using namespace slim;
    using namespace slim::common;
    void require(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        if(args.Length() < 1 || !args[0]->IsString()) {
            isolate->ThrowException(utilities::StringToV8String(isolate, "require: expected a string argument"));
            return;
        }
        std::string specifier_string = utilities::v8StringToString(isolate, args[0].As<v8::String>());
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("require => {}", specifier_string), __FILE__, __LINE__});
#endif
        // strip node: prefix for built-in aliases
        if(specifier_string.starts_with("node:")) {
            specifier_string = specifier_string.substr(5);
        }
        // check if already a global (e.g. a loaded plugin)
        auto context = isolate->GetCurrentContext();
        v8::Local<v8::Value> global_val = context->Global()->Get(context,
            utilities::StringToV8String(isolate, specifier_string)).ToLocalChecked();
        if(!global_val->IsUndefined() && !global_val->IsNull()) {
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("returning global for => {}", specifier_string), __FILE__, __LINE__});
#endif
            args.GetReturnValue().Set(global_val);
            return;
        }
        slim::module::import_specifier spec(isolate, specifier_string, false, v8::Local<v8::Module>());
        args.GetReturnValue().Set(spec.cjs_exports());
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }
}
extern "C" void expose_plugin(v8::Isolate* isolate) {
    using namespace slim;
    auto context = isolate->GetCurrentContext();
    auto require_fn = v8::FunctionTemplate::New(isolate, plugin::require::require)->GetFunction(context).ToLocalChecked();
    context->Global()->Set(context, utilities::StringToV8String(isolate, "require"), require_fn).Check();
}
