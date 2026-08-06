#include <format>
#include <vector>
#include <v8.h>
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/common/utilities.h>
#include <slim/exception_handler.h>
#include <slim/module/import_specifier.h>
#include <slim/module/resolver.h>
#include <slim/utilities.h>

namespace slim::module {
    using namespace slim;
    using namespace slim::common;
}

void slim::module::import_specifier::evaluate_as_cjs() {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
    log::debug({__func__, std::format("specifier_uri_ => {}", specifier_uri_), __FILE__, __LINE__});
#endif
    v8::TryCatch try_catch(isolate_);

    // build module object with exports property
    v8::Local<v8::Object> cjs_module_obj = v8::Object::New(isolate_);
    v8::Local<v8::Object> cjs_exports_obj = v8::Object::New(isolate_);
    cjs_module_obj->Set(context_, slim::utilities::StringToV8String(isolate_, "exports"), cjs_exports_obj).Check();

    // wrap source in CJS IIFE
    std::string_view body(reinterpret_cast<const char*>(fetched_mjs_source_.body.data()), fetched_mjs_source_.body.size());
    std::string wrapped = std::format("(function(module, exports) {{\n{}\n}})(cjs_module_, cjs_module_.exports);", body);
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("wrapped source size => {}", wrapped.size()), __FILE__, __LINE__});
#endif

    // inject module object into context as cjs_module_
    context_->Global()->Set(context_, slim::utilities::StringToV8String(isolate_, "cjs_module_"), cjs_module_obj).Check();

    v8::Local<v8::String> source_string = v8::String::NewFromUtf8(
        isolate_,
        wrapped.data(),
        v8::NewStringType::kNormal,
        static_cast<int>(wrapped.size())
    ).ToLocalChecked();

    v8::ScriptOrigin origin(slim::utilities::StringToV8Value(isolate_, specifier_uri_.data()), 0, 0, false, -1,
        slim::utilities::StringToV8Value(isolate_, ""), false, false, false);
    v8::ScriptCompiler::Source script_source(source_string, origin);
    v8::MaybeLocal<v8::Script> maybe_script = v8::ScriptCompiler::Compile(context_, &script_source);
    if(maybe_script.IsEmpty()) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "script compile failed", __FILE__, __LINE__});
#endif
        if(try_catch.HasCaught()) {
            slim::exception_handler::v8_try_catch_handler(&try_catch);
        }
        return;
    }

    v8::MaybeLocal<v8::Value> run_result = maybe_script.ToLocalChecked()->Run(context_);
    if(run_result.IsEmpty()) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "script run failed", __FILE__, __LINE__});
#endif
        if(try_catch.HasCaught()) {
            slim::exception_handler::v8_try_catch_handler(&try_catch);
        }
        return;
    }

    // pull module.exports back out — CJS bundle may have reassigned it via module.exports = ...
    v8::Local<v8::Value> exports_val = cjs_module_obj->Get(context_,
        slim::utilities::StringToV8String(isolate_, "exports")).ToLocalChecked();
    if(!exports_val->IsObject()) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "module.exports is not an object", __FILE__, __LINE__});
#endif
        isolate_->ThrowException(slim::utilities::StringToV8String(isolate_,
            std::format("cjs module.exports is not an object => {}", specifier_uri_).c_str()));
        return;
    }
    cjs_exports_ = exports_val.As<v8::Object>();

    // synthetic module exposes only default = module.exports
    // named export detection from CJS static analysis is deferred until a concrete use case requires it
    v8::Local<v8::String> default_name = slim::utilities::StringToV8String(isolate_, "default");
    std::vector<v8::Local<v8::String>> export_names = { default_name };
    v8::MemorySpan<const v8::Local<v8::String>> memory_span(export_names.data(), export_names.size());
    v8_module_ = v8::Module::CreateSyntheticModule(isolate_,
        slim::utilities::StringToV8String(isolate_, specifier_uri_),
        memory_span,
        slim::module::import_specifier::cjs_evaluation_steps);
    is_synthetic_module_ = true;
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("synthetic module created, hash_id => {}", v8_module_->GetIdentityHash()), __FILE__, __LINE__});
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    if(try_catch.HasCaught()) {
        slim::exception_handler::v8_try_catch_handler(&try_catch);
    }
}

v8::MaybeLocal<v8::Value> slim::module::import_specifier::cjs_evaluation_steps(
        v8::Local<v8::Context> context, v8::Local<v8::Module> module) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    auto isolate = context->GetIsolate();
    v8::TryCatch try_catch(isolate);
    int hash_id = module->GetIdentityHash();
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("hash_id => {}", hash_id), __FILE__, __LINE__});
#endif
    auto maybe_specifier = slim::module::resolver::get_import_specifier_by_hash_id(hash_id);
    if(!maybe_specifier.has_value()) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("no import_specifier found for hash_id => {}", hash_id), __FILE__, __LINE__});
#endif
        isolate->ThrowException(slim::utilities::StringToV8String(isolate,
            std::format("cjs_evaluation_steps: no specifier for hash_id => {}", hash_id).c_str()));
        return v8::MaybeLocal<v8::Value>();
    }
    import_specifier& spec = maybe_specifier.value().get();
    // set default export to the full module.exports object
    // property access on it at runtime will invoke lazy getters naturally
    auto default_result = module->SetSyntheticModuleExport(isolate,
        slim::utilities::StringToV8String(isolate, "default"), spec.cjs_exports_);
    if(default_result.IsNothing()) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "SetSyntheticModuleExport returned Nothing for => default", __FILE__, __LINE__});
#endif
    }
#ifdef ENABLE_LOGGING
    else {
        log::debug({__func__, "SetSyntheticModuleExport ok => default", __FILE__, __LINE__});
    }
#endif
    if(try_catch.HasCaught()) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "exception caught", __FILE__, __LINE__});
#endif
        slim::exception_handler::v8_try_catch_handler(&try_catch);
        return v8::MaybeLocal<v8::Value>();
    }
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    return v8::MaybeLocal<v8::Value>(v8::True(isolate));
}
