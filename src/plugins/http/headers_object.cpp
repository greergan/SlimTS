#include "http_plugin.h"

#include <algorithm>
#include <slim/common/log.h>
#include <slim/utilities.h>

namespace slim::plugin::http {
    using namespace slim::common;

    v8::Local<v8::Object> make_headers_object(v8::Isolate* isolate, const slim::common::http::Headers& headers) {
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
        auto context = isolate->GetCurrentContext();
        auto headers_obj = v8::Object::New(isolate);

        auto entries_arr = v8::Array::New(isolate);
        uint32_t idx = 0;
        for (const auto& header : headers.entries()) {
            std::string value;
            bool first = true;
            for (const auto& v : const_cast<slim::common::http::Header&>(*header).get_value()) {
                if (!first) value += ", ";
                value += v;
                first = false;
            }
            auto pair = v8::Array::New(isolate, 2);
            pair->Set(context, 0, utilities::StringToV8String(isolate, std::string(header->get_name()))).Check();
            pair->Set(context, 1, utilities::StringToV8String(isolate, value)).Check();
            entries_arr->Set(context, idx++, pair).Check();
        }

        auto cookies_arr = v8::Array::New(isolate);
        uint32_t cidx = 0;
        if (headers.get_cookies()) {
            for (const auto& cookie : headers.get_cookies()->entries()) {
                cookies_arr->Set(context, cidx++, utilities::StringToV8String(isolate, cookie->serialize())).Check();
            }
        }

        headers_obj->DefineOwnProperty(context, utilities::StringToV8String(isolate, "__entries__"), entries_arr, v8::PropertyAttribute::DontEnum).Check();
        headers_obj->DefineOwnProperty(context, utilities::StringToV8String(isolate, "__cookies__"), cookies_arr, v8::PropertyAttribute::DontEnum).Check();

        // entries() — iterates [key, value] pairs
        auto entries_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();
            auto arr = self->Get(context, utilities::StringToV8String(isolate, "__entries__")).ToLocalChecked().As<v8::Array>();
            auto iter_obj = v8::Object::New(isolate);
            auto state = v8::Object::New(isolate);
            state->Set(context, utilities::StringToV8String(isolate, "arr"), arr).Check();
            state->Set(context, utilities::StringToV8String(isolate, "idx"), v8::Integer::New(isolate, 0)).Check();
            iter_obj->DefineOwnProperty(context, utilities::StringToV8String(isolate, "__state__"), state, v8::PropertyAttribute::DontEnum).Check();
            auto next_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                auto* isolate = args.GetIsolate();
                auto context = isolate->GetCurrentContext();
                auto self = args.This();
                auto state = self->Get(context, utilities::StringToV8String(isolate, "__state__")).ToLocalChecked().As<v8::Object>();
                auto arr = state->Get(context, utilities::StringToV8String(isolate, "arr")).ToLocalChecked().As<v8::Array>();
                int idx = state->Get(context, utilities::StringToV8String(isolate, "idx")).ToLocalChecked().As<v8::Int32>()->Value();
                auto result = v8::Object::New(isolate);
                if (idx >= static_cast<int>(arr->Length())) {
                    result->Set(context, utilities::StringToV8String(isolate, "value"), v8::Undefined(isolate)).Check();
                    result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, true)).Check();
                } else {
                    auto pair = arr->Get(context, idx).ToLocalChecked().As<v8::Array>();
                    result->Set(context, utilities::StringToV8String(isolate, "value"), pair).Check();
                    result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, false)).Check();
                    state->Set(context, utilities::StringToV8String(isolate, "idx"), v8::Integer::New(isolate, idx + 1)).Check();
                }
                args.GetReturnValue().Set(result);
            }).ToLocalChecked();
            iter_obj->Set(context, utilities::StringToV8String(isolate, "next"), next_fn).Check();
            auto self_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                args.GetReturnValue().Set(args.This());
            }).ToLocalChecked();
            iter_obj->Set(context, v8::Symbol::GetIterator(isolate), self_fn).Check();
            args.GetReturnValue().Set(iter_obj);
        }).ToLocalChecked();

        // keys() — iterates key strings
        auto keys_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();
            auto arr = self->Get(context, utilities::StringToV8String(isolate, "__entries__")).ToLocalChecked().As<v8::Array>();
            auto iter_obj = v8::Object::New(isolate);
            auto state = v8::Object::New(isolate);
            state->Set(context, utilities::StringToV8String(isolate, "arr"), arr).Check();
            state->Set(context, utilities::StringToV8String(isolate, "idx"), v8::Integer::New(isolate, 0)).Check();
            iter_obj->DefineOwnProperty(context, utilities::StringToV8String(isolate, "__state__"), state, v8::PropertyAttribute::DontEnum).Check();
            auto next_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                auto* isolate = args.GetIsolate();
                auto context = isolate->GetCurrentContext();
                auto self = args.This();
                auto state = self->Get(context, utilities::StringToV8String(isolate, "__state__")).ToLocalChecked().As<v8::Object>();
                auto arr = state->Get(context, utilities::StringToV8String(isolate, "arr")).ToLocalChecked().As<v8::Array>();
                int idx = state->Get(context, utilities::StringToV8String(isolate, "idx")).ToLocalChecked().As<v8::Int32>()->Value();
                auto result = v8::Object::New(isolate);
                if (idx >= static_cast<int>(arr->Length())) {
                    result->Set(context, utilities::StringToV8String(isolate, "value"), v8::Undefined(isolate)).Check();
                    result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, true)).Check();
                } else {
                    auto pair = arr->Get(context, idx).ToLocalChecked().As<v8::Array>();
                    result->Set(context, utilities::StringToV8String(isolate, "value"), pair->Get(context, 0).ToLocalChecked()).Check();
                    result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, false)).Check();
                    state->Set(context, utilities::StringToV8String(isolate, "idx"), v8::Integer::New(isolate, idx + 1)).Check();
                }
                args.GetReturnValue().Set(result);
            }).ToLocalChecked();
            iter_obj->Set(context, utilities::StringToV8String(isolate, "next"), next_fn).Check();
            auto self_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                args.GetReturnValue().Set(args.This());
            }).ToLocalChecked();
            iter_obj->Set(context, v8::Symbol::GetIterator(isolate), self_fn).Check();
            args.GetReturnValue().Set(iter_obj);
        }).ToLocalChecked();

        // values() — iterates value strings
        auto values_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();
            auto arr = self->Get(context, utilities::StringToV8String(isolate, "__entries__")).ToLocalChecked().As<v8::Array>();
            auto iter_obj = v8::Object::New(isolate);
            auto state = v8::Object::New(isolate);
            state->Set(context, utilities::StringToV8String(isolate, "arr"), arr).Check();
            state->Set(context, utilities::StringToV8String(isolate, "idx"), v8::Integer::New(isolate, 0)).Check();
            iter_obj->DefineOwnProperty(context, utilities::StringToV8String(isolate, "__state__"), state, v8::PropertyAttribute::DontEnum).Check();
            auto next_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                auto* isolate = args.GetIsolate();
                auto context = isolate->GetCurrentContext();
                auto self = args.This();
                auto state = self->Get(context, utilities::StringToV8String(isolate, "__state__")).ToLocalChecked().As<v8::Object>();
                auto arr = state->Get(context, utilities::StringToV8String(isolate, "arr")).ToLocalChecked().As<v8::Array>();
                int idx = state->Get(context, utilities::StringToV8String(isolate, "idx")).ToLocalChecked().As<v8::Int32>()->Value();
                auto result = v8::Object::New(isolate);
                if (idx >= static_cast<int>(arr->Length())) {
                    result->Set(context, utilities::StringToV8String(isolate, "value"), v8::Undefined(isolate)).Check();
                    result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, true)).Check();
                } else {
                    auto pair = arr->Get(context, idx).ToLocalChecked().As<v8::Array>();
                    result->Set(context, utilities::StringToV8String(isolate, "value"), pair->Get(context, 1).ToLocalChecked()).Check();
                    result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, false)).Check();
                    state->Set(context, utilities::StringToV8String(isolate, "idx"), v8::Integer::New(isolate, idx + 1)).Check();
                }
                args.GetReturnValue().Set(result);
            }).ToLocalChecked();
            iter_obj->Set(context, utilities::StringToV8String(isolate, "next"), next_fn).Check();
            auto self_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                args.GetReturnValue().Set(args.This());
            }).ToLocalChecked();
            iter_obj->Set(context, v8::Symbol::GetIterator(isolate), self_fn).Check();
            args.GetReturnValue().Set(iter_obj);
        }).ToLocalChecked();

        // get(name)
        auto get_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();
            v8::String::Utf8Value name(isolate, args[0]);
            auto arr = self->Get(context, utilities::StringToV8String(isolate, "__entries__")).ToLocalChecked().As<v8::Array>();
            for (uint32_t i = 0; i < arr->Length(); ++i) {
                auto pair = arr->Get(context, i).ToLocalChecked().As<v8::Array>();
                v8::String::Utf8Value key(isolate, pair->Get(context, 0).ToLocalChecked());
                if (std::string(*key) == std::string(*name)) {
                    args.GetReturnValue().Set(pair->Get(context, 1).ToLocalChecked());
                    return;
                }
            }
            args.GetReturnValue().SetNull();
        }).ToLocalChecked();

        // has(name)
        auto has_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();
            v8::String::Utf8Value name(isolate, args[0]);
            auto arr = self->Get(context, utilities::StringToV8String(isolate, "__entries__")).ToLocalChecked().As<v8::Array>();
            for (uint32_t i = 0; i < arr->Length(); ++i) {
                auto pair = arr->Get(context, i).ToLocalChecked().As<v8::Array>();
                v8::String::Utf8Value key(isolate, pair->Get(context, 0).ToLocalChecked());
                if (std::string(*key) == std::string(*name)) {
                    args.GetReturnValue().Set(v8::Boolean::New(isolate, true));
                    return;
                }
            }
            args.GetReturnValue().Set(v8::Boolean::New(isolate, false));
        }).ToLocalChecked();

        // forEach(cb)
        auto foreach_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();
            auto cb = args[0].As<v8::Function>();
            auto arr = self->Get(context, utilities::StringToV8String(isolate, "__entries__")).ToLocalChecked().As<v8::Array>();
            for (uint32_t i = 0; i < arr->Length(); ++i) {
                auto pair = arr->Get(context, i).ToLocalChecked().As<v8::Array>();
                v8::Local<v8::Value> cb_args[2] = { pair->Get(context, 1).ToLocalChecked(), pair->Get(context, 0).ToLocalChecked() };
                cb->Call(context, v8::Undefined(isolate), 2, cb_args).ToLocalChecked();
            }
        }).ToLocalChecked();

        // getSetCookie()
        auto get_set_cookie_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();
            args.GetReturnValue().Set(self->Get(context, utilities::StringToV8String(isolate, "__cookies__")).ToLocalChecked());
        }).ToLocalChecked();

        // Symbol.iterator on headers_obj itself
        auto symbol_iterator_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();
            auto arr = self->Get(context, utilities::StringToV8String(isolate, "__entries__")).ToLocalChecked().As<v8::Array>();
            auto iter_obj = v8::Object::New(isolate);
            auto state = v8::Object::New(isolate);
            state->Set(context, utilities::StringToV8String(isolate, "arr"), arr).Check();
            state->Set(context, utilities::StringToV8String(isolate, "idx"), v8::Integer::New(isolate, 0)).Check();
            iter_obj->DefineOwnProperty(context, utilities::StringToV8String(isolate, "__state__"), state, v8::PropertyAttribute::DontEnum).Check();
            auto next_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                auto* isolate = args.GetIsolate();
                auto context = isolate->GetCurrentContext();
                auto self = args.This();
                auto state = self->Get(context, utilities::StringToV8String(isolate, "__state__")).ToLocalChecked().As<v8::Object>();
                auto arr = state->Get(context, utilities::StringToV8String(isolate, "arr")).ToLocalChecked().As<v8::Array>();
                int idx = state->Get(context, utilities::StringToV8String(isolate, "idx")).ToLocalChecked().As<v8::Int32>()->Value();
                auto result = v8::Object::New(isolate);
                if (idx >= static_cast<int>(arr->Length())) {
                    result->Set(context, utilities::StringToV8String(isolate, "value"), v8::Undefined(isolate)).Check();
                    result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, true)).Check();
                } else {
                    auto pair = arr->Get(context, idx).ToLocalChecked().As<v8::Array>();
                    result->Set(context, utilities::StringToV8String(isolate, "value"), pair).Check();
                    result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, false)).Check();
                    state->Set(context, utilities::StringToV8String(isolate, "idx"), v8::Integer::New(isolate, idx + 1)).Check();
                }
                args.GetReturnValue().Set(result);
            }).ToLocalChecked();
            iter_obj->Set(context, utilities::StringToV8String(isolate, "next"), next_fn).Check();
            auto self_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
                args.GetReturnValue().Set(args.This());
            }).ToLocalChecked();
            iter_obj->Set(context, v8::Symbol::GetIterator(isolate), self_fn).Check();
            args.GetReturnValue().Set(iter_obj);
        }).ToLocalChecked();

        headers_obj->Set(context, utilities::StringToV8String(isolate, "get"), get_fn).Check();
        headers_obj->Set(context, utilities::StringToV8String(isolate, "has"), has_fn).Check();
        headers_obj->Set(context, utilities::StringToV8String(isolate, "entries"), entries_fn).Check();
        headers_obj->Set(context, utilities::StringToV8String(isolate, "keys"), keys_fn).Check();
        headers_obj->Set(context, utilities::StringToV8String(isolate, "values"), values_fn).Check();
        headers_obj->Set(context, utilities::StringToV8String(isolate, "forEach"), foreach_fn).Check();
        headers_obj->Set(context, utilities::StringToV8String(isolate, "getSetCookie"), get_set_cookie_fn).Check();
        headers_obj->Set(context, v8::Symbol::GetIterator(isolate), symbol_iterator_fn).Check();

        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
        return headers_obj;
    }

} // namespace slim::plugin::http
