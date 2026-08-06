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
namespace slim::plugin::assert_plugin {
    using namespace slim;
    using namespace slim::common;

    // helper to get string representation of a value
    std::string to_string(v8::Isolate* isolate, v8::Local<v8::Value> value) {
        if(value.IsEmpty() || value->IsUndefined()) return "undefined";
        if(value->IsNull()) return "null";
        return utilities::v8ValueToString(isolate, value);
    }

    void assert_ok(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        if(args.Length() < 1 || !args[0]->BooleanValue(isolate)) {
            std::string msg = args.Length() >= 2 ? to_string(isolate, args[1]) : "Assertion failed";
            isolate->ThrowException(utilities::StringToV8String(isolate, msg));
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_fail(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        std::string msg = args.Length() >= 1 ? to_string(isolate, args[0]) : "Assertion failed";
        isolate->ThrowException(utilities::StringToV8String(isolate, msg));
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_equal(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        if(args.Length() < 2) return;
        bool equal = args[0]->Equals(context, args[1]).FromMaybe(false);
        if(!equal) {
            std::string msg = args.Length() >= 3 ? to_string(isolate, args[2])
                : std::format("{} == {}", to_string(isolate, args[0]), to_string(isolate, args[1]));
            isolate->ThrowException(utilities::StringToV8String(isolate, msg));
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_not_equal(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        if(args.Length() < 2) return;
        bool equal = args[0]->Equals(context, args[1]).FromMaybe(false);
        if(equal) {
            std::string msg = args.Length() >= 3 ? to_string(isolate, args[2])
                : std::format("{} != {}", to_string(isolate, args[0]), to_string(isolate, args[1]));
            isolate->ThrowException(utilities::StringToV8String(isolate, msg));
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_strict_equal(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        if(args.Length() < 2) return;
        bool equal = args[0]->StrictEquals(args[1]);
        if(!equal) {
            std::string msg = args.Length() >= 3 ? to_string(isolate, args[2])
                : std::format("{} === {}", to_string(isolate, args[0]), to_string(isolate, args[1]));
            isolate->ThrowException(utilities::StringToV8String(isolate, msg));
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_not_strict_equal(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        if(args.Length() < 2) return;
        bool equal = args[0]->StrictEquals(args[1]);
        if(equal) {
            std::string msg = args.Length() >= 3 ? to_string(isolate, args[2])
                : std::format("{} !== {}", to_string(isolate, args[0]), to_string(isolate, args[1]));
            isolate->ThrowException(utilities::StringToV8String(isolate, msg));
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_deep_equal(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        if(args.Length() < 2) return;
        // use JSON stringify for deep comparison
        auto json_actual = v8::JSON::Stringify(context, args[0]).ToLocalChecked();
        auto json_expected = v8::JSON::Stringify(context, args[1]).ToLocalChecked();
        bool equal = json_actual->StrictEquals(json_expected);
        if(!equal) {
            std::string msg = args.Length() >= 3 ? to_string(isolate, args[2])
                : std::format("deepEqual failed: {} != {}", utilities::v8StringToString(isolate, json_actual), utilities::v8StringToString(isolate, json_expected));
            isolate->ThrowException(utilities::StringToV8String(isolate, msg));
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_not_deep_equal(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        if(args.Length() < 2) return;
        auto json_actual = v8::JSON::Stringify(context, args[0]).ToLocalChecked();
        auto json_expected = v8::JSON::Stringify(context, args[1]).ToLocalChecked();
        bool equal = json_actual->StrictEquals(json_expected);
        if(equal) {
            std::string msg = args.Length() >= 3 ? to_string(isolate, args[2])
                : std::format("notDeepEqual failed: {} == {}", utilities::v8StringToString(isolate, json_actual), utilities::v8StringToString(isolate, json_expected));
            isolate->ThrowException(utilities::StringToV8String(isolate, msg));
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_deep_strict_equal(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        if(args.Length() < 2) return;
        auto json_actual = v8::JSON::Stringify(context, args[0]).ToLocalChecked();
        auto json_expected = v8::JSON::Stringify(context, args[1]).ToLocalChecked();
        bool equal = json_actual->StrictEquals(json_expected);
        if(!equal) {
            std::string msg = args.Length() >= 3 ? to_string(isolate, args[2])
                : std::format("deepStrictEqual failed: {} !== {}", utilities::v8StringToString(isolate, json_actual), utilities::v8StringToString(isolate, json_expected));
            isolate->ThrowException(utilities::StringToV8String(isolate, msg));
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_not_deep_strict_equal(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        if(args.Length() < 2) return;
        auto json_actual = v8::JSON::Stringify(context, args[0]).ToLocalChecked();
        auto json_expected = v8::JSON::Stringify(context, args[1]).ToLocalChecked();
        bool equal = json_actual->StrictEquals(json_expected);
        if(equal) {
            std::string msg = args.Length() >= 3 ? to_string(isolate, args[2])
                : std::format("notDeepStrictEqual failed: {} === {}", utilities::v8StringToString(isolate, json_actual), utilities::v8StringToString(isolate, json_expected));
            isolate->ThrowException(utilities::StringToV8String(isolate, msg));
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_throws(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        if(args.Length() < 1 || !args[0]->IsFunction()) {
            isolate->ThrowException(utilities::StringToV8String(isolate, "assert.throws: expected a function argument"));
            return;
        }
        v8::TryCatch try_catch(isolate);
        auto fn = args[0].As<v8::Function>();
        fn->Call(context, context->Global(), 0, nullptr);
        if(!try_catch.HasCaught()) {
            std::string msg = args.Length() >= 3 ? to_string(isolate, args[2]) : "assert.throws: function did not throw";
            isolate->ThrowException(utilities::StringToV8String(isolate, msg));
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_does_not_throw(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        if(args.Length() < 1 || !args[0]->IsFunction()) {
            isolate->ThrowException(utilities::StringToV8String(isolate, "assert.doesNotThrow: expected a function argument"));
            return;
        }
        v8::TryCatch try_catch(isolate);
        auto fn = args[0].As<v8::Function>();
        fn->Call(context, context->Global(), 0, nullptr);
        if(try_catch.HasCaught()) {
            std::string msg = args.Length() >= 3 ? to_string(isolate, args[2])
                : std::format("assert.doesNotThrow: function threw => {}", to_string(isolate, try_catch.Exception()));
            isolate->ThrowException(utilities::StringToV8String(isolate, msg));
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_if_error(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        if(args.Length() >= 1 && !args[0]->IsNull() && !args[0]->IsUndefined() && !args[0]->IsFalse()) {
            isolate->ThrowException(args[0]);
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_match(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        if(args.Length() < 2 || !args[0]->IsString() || !args[1]->IsRegExp()) {
            isolate->ThrowException(utilities::StringToV8String(isolate, "assert.match: expected string and RegExp arguments"));
            return;
        }
        auto regex = args[1].As<v8::RegExp>();
        auto result = regex->Exec(context, args[0].As<v8::String>()).ToLocalChecked();
        if(result->IsNull()) {
            std::string msg = args.Length() >= 3 ? to_string(isolate, args[2])
                : std::format("assert.match failed: {} does not match", to_string(isolate, args[0]));
            isolate->ThrowException(utilities::StringToV8String(isolate, msg));
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }

    void assert_does_not_match(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
        auto isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        if(args.Length() < 2 || !args[0]->IsString() || !args[1]->IsRegExp()) {
            isolate->ThrowException(utilities::StringToV8String(isolate, "assert.doesNotMatch: expected string and RegExp arguments"));
            return;
        }
        auto regex = args[1].As<v8::RegExp>();
        auto result = regex->Exec(context, args[0].As<v8::String>()).ToLocalChecked();
        if(!result->IsNull()) {
            std::string msg = args.Length() >= 3 ? to_string(isolate, args[2])
                : std::format("assert.doesNotMatch failed: {} matches", to_string(isolate, args[0]));
            isolate->ThrowException(utilities::StringToV8String(isolate, msg));
        }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    }
}

extern "C" void expose_plugin(v8::Isolate* isolate) {
    using namespace slim;
    auto context = isolate->GetCurrentContext();
    plugin::plugin assert_plugin(isolate, "assert");
    assert_plugin.add_function("ok", plugin::assert_plugin::assert_ok);
    assert_plugin.add_function("fail", plugin::assert_plugin::assert_fail);
    assert_plugin.add_function("equal", plugin::assert_plugin::assert_equal);
    assert_plugin.add_function("notEqual", plugin::assert_plugin::assert_not_equal);
    assert_plugin.add_function("strictEqual", plugin::assert_plugin::assert_strict_equal);
    assert_plugin.add_function("notStrictEqual", plugin::assert_plugin::assert_not_strict_equal);
    assert_plugin.add_function("deepEqual", plugin::assert_plugin::assert_deep_equal);
    assert_plugin.add_function("notDeepEqual", plugin::assert_plugin::assert_not_deep_equal);
    assert_plugin.add_function("deepStrictEqual", plugin::assert_plugin::assert_deep_strict_equal);
    assert_plugin.add_function("notDeepStrictEqual", plugin::assert_plugin::assert_not_deep_strict_equal);
    assert_plugin.add_function("throws", plugin::assert_plugin::assert_throws);
    assert_plugin.add_function("doesNotThrow", plugin::assert_plugin::assert_does_not_throw);
    assert_plugin.add_function("ifError", plugin::assert_plugin::assert_if_error);
    assert_plugin.add_function("match", plugin::assert_plugin::assert_match);
    assert_plugin.add_function("doesNotMatch", plugin::assert_plugin::assert_does_not_match);
    assert_plugin.expose_plugin();
    // assert() itself as a callable — expose as global function too
    auto assert_fn = v8::FunctionTemplate::New(isolate, plugin::assert_plugin::assert_ok)
        ->GetFunction(context).ToLocalChecked();
    auto assert_obj = assert_plugin.new_instance();
    // copy all properties from assert_obj onto assert_fn
    auto keys = assert_obj->GetOwnPropertyNames(context).ToLocalChecked();
    for(uint32_t i = 0; i < keys->Length(); i++) {
        auto key = keys->Get(context, i).ToLocalChecked();
        auto val = assert_obj->Get(context, key).ToLocalChecked();
        assert_fn->Set(context, key, val).Check();
    }
    context->Global()->Set(context, utilities::StringToV8String(isolate, "assert"), assert_fn).Check();
}
