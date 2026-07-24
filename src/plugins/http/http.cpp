#include <v8.h>
#include <deque>
#include <format>
#include <memory>
#include <string>
#include <slim/common/log.h>
#include <slim/common/http/request.h>
#include <slim/common/network/client/http.h>
#include <slim/common/network/server/tcp.h>
#include <slim/isolate_wake.h>
#include <slim/plugin.hpp>
#include <slim/runtime.h>
#include <slim/slim.h>
#include <slim/utilities.h>

namespace slim::plugin::http {
    using namespace slim::common;

    struct ListenerState {
        v8::Isolate* isolate;
        v8::Global<v8::Promise::Resolver> pending_resolver;
        std::deque<v8::Global<v8::Object>> pending_requests;
        std::unique_ptr<slim::common::network::server::Tcp> tcp_server;
        explicit ListenerState(v8::Isolate* iso) : isolate(iso) {}
    };

    v8::Local<v8::Object> make_headers_object(v8::Isolate* isolate, const slim::common::http::Headers& headers) {
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
        auto context = isolate->GetCurrentContext();
        auto headers_obj = v8::Object::New(isolate);

        // build internal entries array
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

        // build cookies array
        auto cookies_arr = v8::Array::New(isolate);
        uint32_t cidx = 0;
        if (headers.get_cookies()) {
            for (const auto& cookie : headers.get_cookies()->entries()) {
                cookies_arr->Set(context, cidx++, utilities::StringToV8String(isolate, cookie->serialize())).Check();
            }
        }

        // store entries and cookies as non-enumerable
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

    v8::Local<v8::Object> make_request_object(v8::Isolate* isolate, slim::common::http::Request& request) {
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
        auto context = isolate->GetCurrentContext();
        auto request_obj = v8::Object::New(isolate);

        // url
        request_obj->Set(context, utilities::StringToV8String(isolate, "url"), utilities::StringToV8String(isolate, std::string(request.url().href()))).Check();

        // method
        request_obj->Set(context, utilities::StringToV8String(isolate, "method"), utilities::StringToV8String(isolate, std::string(request.method()))).Check();

        // headers
        request_obj->Set(context, utilities::StringToV8String(isolate, "headers"), make_headers_object(isolate, request.headers())).Check();

        // bodyUsed
        request_obj->Set(context, utilities::StringToV8String(isolate, "bodyUsed"), v8::Boolean::New(isolate, false)).Check();

        // body bytes for closures
        auto body_span = request.get_body();
        auto body_str = std::make_shared<std::string>(reinterpret_cast<const char*>(body_span.data()), body_span.size());

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

        // signal stub
        auto signal_obj = v8::Object::New(isolate);
        signal_obj->Set(context, utilities::StringToV8String(isolate, "aborted"), v8::Boolean::New(isolate, false)).Check();

        request_obj->Set(context, utilities::StringToV8String(isolate, "__body__"), utilities::StringToV8String(isolate, *body_str)).Check();
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

    slim::common::io::Task<void> connection_handler(slim::common::io::Scheduler& scheduler, int fd, SSL_CTX* ssl_ctx, std::shared_ptr<ListenerState> state) {
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
        log::debug(log::Message(__func__, "fd => " + std::to_string(fd), __FILE__, __LINE__));

        auto connection = co_await slim::common::network::client::http::Connection::create(scheduler, fd, nullptr);
        log::debug(log::Message(__func__, "connection created", __FILE__, __LINE__));

        std::vector<uint8_t> buf;
        log::debug(log::Message(__func__, "calling read", __FILE__, __LINE__));
        connection.read(buf);
        log::debug(log::Message(__func__, "read complete, bytes => " + std::to_string(buf.size()), __FILE__, __LINE__));

        slim::common::http::Request request;
        request.set_body(std::move(buf));
        auto parse_status = request.parse();
        if (parse_status != slim::common::http::ErrorStatus::OK) {
            log::error(log::Message(__func__, std::string("request parse error => ") + std::string(slim::common::http::error::status::to_string(parse_status)), __FILE__, __LINE__));
            co_return;
        }
        std::string path = std::string(request.url().pathname());
        log::debug(log::Message(__func__, "parsed path => " + path, __FILE__, __LINE__));

        constexpr std::string_view body = "<html><body><h1>slim is alive</h1><p>request received</p></body></html>";
        auto response = std::format("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{}", body.size(), body);
        log::debug(log::Message(__func__, "writing response", __FILE__, __LINE__));
        connection.write(response);
        log::debug(log::Message(__func__, "response written", __FILE__, __LINE__));

        slim::isolate_wake::post(state->isolate, [state, request = std::move(request)]() mutable {
            log::trace(log::Message(__func__, "resolve task begins", __FILE__, __LINE__));
            auto* isolate = state->isolate;
            auto context = isolate->GetCurrentContext();

            auto request_obj = make_request_object(isolate, request);
            auto event_obj = v8::Object::New(isolate);
            if (event_obj->Set(context, utilities::StringToV8String(isolate, "request"), request_obj).IsNothing()) {
                log::error(log::Message(__func__, "failed to set request on event", __FILE__, __LINE__));
                return;
            }

            if (state->pending_resolver.IsEmpty()) {
                log::debug(log::Message(__func__, "no resolver waiting, buffering request", __FILE__, __LINE__));
                v8::Global<v8::Object> global_event(isolate, event_obj);
                state->pending_requests.push_back(std::move(global_event));
                return;
            }

            v8::Local<v8::Promise::Resolver> resolver = state->pending_resolver.Get(isolate);
            state->pending_resolver.Reset();

            auto iter_result = v8::Object::New(isolate);
            if (iter_result->Set(context, utilities::StringToV8String(isolate, "value"), event_obj).IsNothing()) {
                log::error(log::Message(__func__, "failed to set value", __FILE__, __LINE__));
                return;
            }
            if (iter_result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, false)).IsNothing()) {
                log::error(log::Message(__func__, "failed to set done", __FILE__, __LINE__));
                return;
            }
            if (resolver->Resolve(context, iter_result).IsNothing()) {
                log::error(log::Message(__func__, "failed to resolve promise", __FILE__, __LINE__));
                return;
            }
            log::debug(log::Message(__func__, "promise resolved", __FILE__, __LINE__));
            log::trace(log::Message(__func__, "resolve task ends", __FILE__, __LINE__));
        });

        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
    }

    void listener_next(const v8::FunctionCallbackInfo<v8::Value>& args) {
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
        auto* isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        auto maybe_resolver = v8::Promise::Resolver::New(context);

        v8::Local<v8::Promise::Resolver> resolver;
        if (!maybe_resolver.ToLocal(&resolver)) {
            log::error(log::Message(__func__, "failed to create promise resolver", __FILE__, __LINE__));
            return;
        }
        args.GetReturnValue().Set(resolver->GetPromise());

        auto* state = static_cast<ListenerState*>(args.Data().As<v8::External>()->Value());

        if (!state->pending_requests.empty()) {
            auto event_global = std::move(state->pending_requests.front());
            state->pending_requests.pop_front();
            auto event_obj = event_global.Get(isolate);
            log::debug(log::Message(__func__, "draining buffered request", __FILE__, __LINE__));

            auto iter_result = v8::Object::New(isolate);
            if (iter_result->Set(context, utilities::StringToV8String(isolate, "value"), event_obj).IsNothing()) {
                log::error(log::Message(__func__, "failed to set value", __FILE__, __LINE__));
                return;
            }
            if (iter_result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, false)).IsNothing()) {
                log::error(log::Message(__func__, "failed to set done", __FILE__, __LINE__));
                return;
            }
            if (resolver->Resolve(context, iter_result).IsNothing()) {
                log::error(log::Message(__func__, "failed to resolve promise", __FILE__, __LINE__));
                return;
            }
            log::debug(log::Message(__func__, "promise resolved from buffer", __FILE__, __LINE__));
        } else {
            log::debug(log::Message(__func__, "storing pending resolver", __FILE__, __LINE__));
            state->pending_resolver.Reset(isolate, resolver);
        }

        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
    }

    void listener_async_iterator(const v8::FunctionCallbackInfo<v8::Value>& args) {
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
        args.GetReturnValue().Set(args.This());
        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
    }

    void serve(const v8::FunctionCallbackInfo<v8::Value>& args) {
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
        auto* isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        v8::HandleScope handle_scope(isolate);

        auto config_obj = args[0].As<v8::Object>();
        auto host = utilities::StringValue(isolate, "host", config_obj);
        auto port = utilities::V8ValueToInt(isolate, utilities::GetValue(isolate, "port", config_obj));
        log::debug(log::Message(__func__, "host => " + host + ", port => " + std::to_string(port), __FILE__, __LINE__));

        auto state = std::make_shared<ListenerState>(isolate);
        log::debug(log::Message(__func__, "ListenerState created", __FILE__, __LINE__));

        auto stop_token = slim::get_stop_token();
        slim::common::network::server::tcp::Config listen_config{host, port, "", ""};
        state->tcp_server = std::make_unique<slim::common::network::server::Tcp>(listen_config, slim::runtime::instance(),
            stop_token, [state](slim::common::io::Scheduler& sched, int fd, SSL_CTX* ssl_ctx) {
                return connection_handler(sched, fd, ssl_ctx, state);
            });
        log::debug(log::Message(__func__, "tcp server started", __FILE__, __LINE__));

        auto* persistent_state = new std::shared_ptr<ListenerState>(state);
        v8::Local<v8::External> state_external = v8::External::New(isolate, state.get());

        v8::Local<v8::Object> listener = v8::Object::New(isolate);

        v8::Local<v8::FunctionTemplate> next_tpl = v8::FunctionTemplate::New(isolate, listener_next, state_external);
        v8::Local<v8::Function> next_func;
        if (!next_tpl->GetFunction(context).ToLocal(&next_func)) {
            log::error(log::Message(__func__, "failed to get next function", __FILE__, __LINE__));
            delete persistent_state;
            return;
        }
        if (listener->Set(context, utilities::StringToV8String(isolate, "next"), next_func).IsNothing()) {
            log::error(log::Message(__func__, "failed to set next on listener", __FILE__, __LINE__));
            delete persistent_state;
            return;
        }
        log::debug(log::Message(__func__, "next function set on listener", __FILE__, __LINE__));

        v8::Local<v8::FunctionTemplate> iter_tpl = v8::FunctionTemplate::New(isolate, listener_async_iterator);
        v8::Local<v8::Function> iter_func;
        if (!iter_tpl->GetFunction(context).ToLocal(&iter_func)) {
            log::error(log::Message(__func__, "failed to get asyncIterator function", __FILE__, __LINE__));
            delete persistent_state;
            return;
        }
        if (listener->Set(context, v8::Symbol::GetAsyncIterator(isolate), iter_func).IsNothing()) {
            log::error(log::Message(__func__, "failed to set asyncIterator on listener", __FILE__, __LINE__));
            delete persistent_state;
            return;
        }
        log::debug(log::Message(__func__, "asyncIterator set on listener", __FILE__, __LINE__));

        auto* persistent_listener = new v8::Global<v8::Object>(isolate, listener);
        persistent_listener->SetWeak(persistent_state,
            [](const v8::WeakCallbackInfo<std::shared_ptr<ListenerState>>& data) {
                log::debug(log::Message(__func__, "listener GC'd, releasing persistent_state", __FILE__, __LINE__));
                delete data.GetParameter();
            },
            v8::WeakCallbackType::kParameter);

        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
        args.GetReturnValue().Set(listener);
    }

    extern "C" void expose_plugin(v8::Isolate* isolate) {
        v8::HandleScope handle_scope(isolate);
        slim::plugin::plugin http_plugin(isolate, "http");
        http_plugin.add_function("serve", slim::plugin::http::serve);
        http_plugin.expose_plugin();
    }
}
