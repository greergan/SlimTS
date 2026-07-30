#include "http_plugin.h"

#include <slim/common/log.h>
#include <slim/utilities.h>

namespace slim::plugin::http {
    using namespace slim::common;

    v8::Local<v8::Object> make_request_object(v8::Isolate* isolate, slim::common::http::Request& request) {
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
        auto context = isolate->GetCurrentContext();
        auto request_obj = v8::Object::New(isolate);

        request_obj->Set(context, utilities::StringToV8String(isolate, "url"), utilities::StringToV8String(isolate, std::string(request.url().href()))).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "method"), utilities::StringToV8String(isolate, std::string(request.method()))).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "headers"), make_headers_object(isolate, request.headers())).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "bodyUsed"), v8::Boolean::New(isolate, false)).Check();

        auto body_span = request.get_body();
        auto body_str = std::string(reinterpret_cast<const char*>(body_span.data()), body_span.size());

        request_obj->DefineOwnProperty(context, utilities::StringToV8String(isolate, "__body__"),
            utilities::StringToV8String(isolate, body_str), v8::PropertyAttribute::DontEnum).Check();

        // text()
        auto text_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();
            auto resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
            auto body = self->Get(context, utilities::StringToV8String(isolate, "__body__")).ToLocalChecked();
            resolver->Resolve(context, body).Check();
            self->Set(context, utilities::StringToV8String(isolate, "bodyUsed"), v8::Boolean::New(isolate, true)).Check();
            args.GetReturnValue().Set(resolver->GetPromise());
        }).ToLocalChecked();

        // json()
        auto json_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();
            auto resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
            auto body = self->Get(context, utilities::StringToV8String(isolate, "__body__")).ToLocalChecked();
            auto parsed = v8::JSON::Parse(context, body.As<v8::String>());
            v8::Local<v8::Value> result;
            if (parsed.ToLocal(&result)) {
                resolver->Resolve(context, result).Check();
            } else {
                resolver->Reject(context, utilities::StringToV8String(isolate, "JSON parse error")).Check();
            }
            self->Set(context, utilities::StringToV8String(isolate, "bodyUsed"), v8::Boolean::New(isolate, true)).Check();
            args.GetReturnValue().Set(resolver->GetPromise());
        }).ToLocalChecked();

        // arrayBuffer()
        auto arraybuffer_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();
            auto resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
            auto body = self->Get(context, utilities::StringToV8String(isolate, "__body__")).ToLocalChecked();
            v8::String::Utf8Value utf8(isolate, body);
            auto ab = v8::ArrayBuffer::New(isolate, utf8.length());
            memcpy(ab->GetBackingStore()->Data(), *utf8, utf8.length());
            resolver->Resolve(context, ab).Check();
            self->Set(context, utilities::StringToV8String(isolate, "bodyUsed"), v8::Boolean::New(isolate, true)).Check();
            args.GetReturnValue().Set(resolver->GetPromise());
        }).ToLocalChecked();

        // clone()
        auto clone_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();
            auto clone = v8::Object::New(isolate);
            auto prop_names = self->GetOwnPropertyNames(context).ToLocalChecked();
            for (uint32_t i = 0; i < prop_names->Length(); ++i) {
                auto key = prop_names->Get(context, i).ToLocalChecked();
                clone->Set(context, key, self->Get(context, key).ToLocalChecked()).Check();
            }
            args.GetReturnValue().Set(clone);
        }).ToLocalChecked();

        auto signal_obj = v8::Object::New(isolate);
        signal_obj->Set(context, utilities::StringToV8String(isolate, "aborted"), v8::Boolean::New(isolate, false)).Check();

        request_obj->Set(context, utilities::StringToV8String(isolate, "text"), text_fn).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "json"), json_fn).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "arrayBuffer"), arraybuffer_fn).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "clone"), clone_fn).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "signal"), signal_obj).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "credentials"), utilities::StringToV8String(isolate, "same-origin")).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "cache"), utilities::StringToV8String(isolate, "default")).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "redirect"), utilities::StringToV8String(isolate, "follow")).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "referrer"), utilities::StringToV8String(isolate, "about:client")).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "referrerPolicy"), utilities::StringToV8String(isolate, "")).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "mode"), utilities::StringToV8String(isolate, "cors")).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "integrity"), utilities::StringToV8String(isolate, "")).Check();
        request_obj->Set(context, utilities::StringToV8String(isolate, "keepalive"), v8::Boolean::New(isolate, false)).Check();

        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
        return request_obj;
    }

} // namespace slim::plugin::http
