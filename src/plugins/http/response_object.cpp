#include "http_plugin.h"

#include <format>
#include <sys/socket.h>
#include <slim/common/log.h>
#include <slim/utilities.h>

namespace slim::plugin::http {
    using namespace slim::common;

    // forward-declared here so both reply() lambda and the weak callback can use it
    struct WeakData {
        std::shared_ptr<ConnectionState>* conn_state_ptr;
        v8::Global<v8::Object>*           persistent_response;
    };

    v8::Local<v8::Object> make_response_object(v8::Isolate* isolate, std::shared_ptr<ConnectionState> conn_state) {
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
        auto context = isolate->GetCurrentContext();
        auto response_obj = v8::Object::New(isolate);

        // standard response properties
        response_obj->Set(context, utilities::StringToV8String(isolate, "status"), v8::Integer::New(isolate, 200)).Check();
        response_obj->Set(context, utilities::StringToV8String(isolate, "statusText"), utilities::StringToV8String(isolate, "OK")).Check();
        response_obj->Set(context, utilities::StringToV8String(isolate, "ok"), v8::Boolean::New(isolate, true)).Check();
        response_obj->Set(context, utilities::StringToV8String(isolate, "redirected"), v8::Boolean::New(isolate, false)).Check();
        response_obj->Set(context, utilities::StringToV8String(isolate, "type"), utilities::StringToV8String(isolate, "basic")).Check();
        response_obj->Set(context, utilities::StringToV8String(isolate, "url"), utilities::StringToV8String(isolate, "")).Check();
        response_obj->Set(context, utilities::StringToV8String(isolate, "bodyUsed"), v8::Boolean::New(isolate, false)).Check();
        response_obj->Set(context, utilities::StringToV8String(isolate, "headers"), make_headers_object(isolate, slim::common::http::Headers{})).Check();

        // store conn_state as non-enumerable external; heap-allocated to outlive this stack frame
        auto* conn_state_ptr = new std::shared_ptr<ConnectionState>(conn_state);
        auto conn_external = v8::External::New(isolate, conn_state_ptr);
        response_obj->DefineOwnProperty(context, utilities::StringToV8String(isolate, "__conn__"),
            conn_external, v8::PropertyAttribute::DontEnum).Check();

        // register weak callback on response_obj to delete conn_state_ptr when GC collects it.
        // this covers the case where reply() is never called (conn_state_ptr would otherwise leak).
        auto* persistent_response = new v8::Global<v8::Object>(isolate, response_obj);
        auto* weak_data = new WeakData{ conn_state_ptr, persistent_response };
        persistent_response->SetWeak(weak_data,
            [](const v8::WeakCallbackInfo<WeakData>& data) {
                log::debug(log::Message(__func__, "response GC'd, releasing conn_state_ptr and persistent_response",
                    __FILE__, __LINE__));
                auto* wd = data.GetParameter();
                // null-guard: reply() may have already cleaned up
                if (wd->conn_state_ptr) {
                    delete wd->conn_state_ptr;
                    wd->conn_state_ptr = nullptr;
                }
                if (wd->persistent_response) {
                    delete wd->persistent_response;
                    wd->persistent_response = nullptr;
                }
                delete wd;
            },
            v8::WeakCallbackType::kParameter);

        // store weak_data as non-enumerable external so reply() can clean up explicitly
        auto wd_external = v8::External::New(isolate, weak_data);
        response_obj->DefineOwnProperty(context, utilities::StringToV8String(isolate, "__wd__"),
            wd_external, v8::PropertyAttribute::DontEnum).Check();

        // reply(body, init?)
        auto reply_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();

            auto conn_external = self->Get(context, utilities::StringToV8String(isolate, "__conn__")).ToLocalChecked().As<v8::External>();
            auto* conn_state_ptr = static_cast<std::shared_ptr<ConnectionState>*>(conn_external->Value());
            auto& conn_state = *conn_state_ptr;

            // body
            std::string body_str;
            if (args.Length() > 0 && !args[0]->IsUndefined() && !args[0]->IsNull()) {
                v8::String::Utf8Value utf8(isolate, args[0]);
                if (*utf8) body_str = std::string(*utf8, utf8.length());
            }

            // init: { status, statusText, headers }
            int status = self->Get(context, utilities::StringToV8String(isolate, "status")).ToLocalChecked().As<v8::Int32>()->Value();
            std::string status_text = utilities::v8StringToString(isolate,
                self->Get(context, utilities::StringToV8String(isolate, "statusText")).ToLocalChecked().As<v8::String>());

            if (args.Length() > 1 && args[1]->IsObject()) {
                auto init = args[1].As<v8::Object>();
                auto status_val = init->Get(context, utilities::StringToV8String(isolate, "status")).ToLocalChecked();
                if (!status_val->IsUndefined()) status = status_val->ToInt32(context).ToLocalChecked()->Value();
                auto status_text_val = init->Get(context, utilities::StringToV8String(isolate, "statusText")).ToLocalChecked();
                if (!status_text_val->IsUndefined())
                    status_text = utilities::v8StringToString(isolate, status_text_val.As<v8::String>());
            }

            // build header string from response headers object
            std::string headers_str;
            auto headers_obj = self->Get(context, utilities::StringToV8String(isolate, "headers")).ToLocalChecked().As<v8::Object>();
            auto entries_arr = headers_obj->Get(context, utilities::StringToV8String(isolate, "__entries__")).ToLocalChecked().As<v8::Array>();
            for (uint32_t i = 0; i < entries_arr->Length(); ++i) {
                auto pair = entries_arr->Get(context, i).ToLocalChecked().As<v8::Array>();
                v8::String::Utf8Value key(isolate, pair->Get(context, 0).ToLocalChecked());
                v8::String::Utf8Value val(isolate, pair->Get(context, 1).ToLocalChecked());
                headers_str += std::string(*key) + ": " + std::string(*val) + "\r\n";
            }

            auto response = std::format("HTTP/1.1 {} {}\r\nContent-Type: text/html\r\nContent-Length: {}\r\nConnection: close\r\n{}\r\n{}",
                status, status_text, body_str.size(), headers_str, body_str);

            // raw blocking send — reply_fn runs on V8 thread, cannot co_await
            log::debug(log::Message(__func__, "writing reply", __FILE__, __LINE__));
            {
                const char* data      = response.data();
                size_t      remaining = response.size();
                while (remaining > 0) {
                    ssize_t sent = ::send(conn_state->fd, data, remaining, MSG_NOSIGNAL);
                    if (sent < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                        log::error(log::Message(__func__, "send failed => " + std::string(strerror(errno)), __FILE__, __LINE__));
                        break;
                    }
                    data      += sent;
                    remaining -= static_cast<size_t>(sent);
                }
            }
            log::debug(log::Message(__func__, "reply written", __FILE__, __LINE__));

            // close connection unless client requested keep-alive
            if (conn_state->connection_header != "keep-alive") {
                log::debug(log::Message(__func__, "closing connection (not keep-alive), connection_header => '" +
                    conn_state->connection_header + "'", __FILE__, __LINE__));
                conn_state_ptr->reset(); // destructor closes socket
            } else {
                log::debug(log::Message(__func__, "keeping connection alive, connection_header => '" +
                    conn_state->connection_header + "'", __FILE__, __LINE__));
                conn_state_ptr->reset(); // destructor closes socket // temporary
            }

            self->Set(context, utilities::StringToV8String(isolate, "bodyUsed"), v8::Boolean::New(isolate, true)).Check();

            // clean up WeakData bookkeeping explicitly so the weak callback is a no-op if GC fires later
            auto wd_external = self->Get(context, utilities::StringToV8String(isolate, "__wd__")).ToLocalChecked().As<v8::External>();
            auto* wd = static_cast<WeakData*>(wd_external->Value());
            if (wd && wd->conn_state_ptr) { // conn_state_ptr null → reply already called
                delete wd->conn_state_ptr;
                wd->conn_state_ptr = nullptr;
                wd->persistent_response->ClearWeak();
                wd->persistent_response->Reset();
                delete wd->persistent_response;
                wd->persistent_response = nullptr;
                delete wd;
                // null out the external so a second call is a no-op
                wd_external->Value(); // can't mutate External; zero the property instead
                self->DefineOwnProperty(context, utilities::StringToV8String(isolate, "__wd__"),
                    v8::External::New(isolate, nullptr), v8::PropertyAttribute::DontEnum).Check();
            }

            auto resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
            resolver->Resolve(context, v8::Undefined(isolate)).Check();
            args.GetReturnValue().Set(resolver->GetPromise());
        }).ToLocalChecked();

        // text()
        auto text_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
            resolver->Resolve(context, utilities::StringToV8String(isolate, "")).Check();
            args.GetReturnValue().Set(resolver->GetPromise());
        }).ToLocalChecked();

        // json()
        auto json_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
            resolver->Resolve(context, v8::Null(isolate)).Check();
            args.GetReturnValue().Set(resolver->GetPromise());
        }).ToLocalChecked();

        // arrayBuffer()
        auto arraybuffer_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
            resolver->Resolve(context, v8::ArrayBuffer::New(isolate, 0)).Check();
            args.GetReturnValue().Set(resolver->GetPromise());
        }).ToLocalChecked();

        // clone()
        auto clone_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto self = args.This();
            auto clone = v8::Object::New(isolate);
            // ONLY_ENUMERABLE excludes __conn__ and __wd__ (both DontEnum)
            auto prop_names = self->GetOwnPropertyNames(context, v8::PropertyFilter::ONLY_ENUMERABLE).ToLocalChecked();
            for (uint32_t i = 0; i < prop_names->Length(); ++i) {
                auto key = prop_names->Get(context, i).ToLocalChecked();
                clone->Set(context, key, self->Get(context, key).ToLocalChecked()).Check();
            }
            args.GetReturnValue().Set(clone);
        }).ToLocalChecked();

        // error() static-style
        auto error_fn = v8::Function::New(context, [](const v8::FunctionCallbackInfo<v8::Value>& args) {
            auto* isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto err_obj = v8::Object::New(isolate);
            err_obj->Set(context, utilities::StringToV8String(isolate, "status"), v8::Integer::New(isolate, 0)).Check();
            err_obj->Set(context, utilities::StringToV8String(isolate, "type"), utilities::StringToV8String(isolate, "error")).Check();
            err_obj->Set(context, utilities::StringToV8String(isolate, "ok"), v8::Boolean::New(isolate, false)).Check();
            args.GetReturnValue().Set(err_obj);
        }).ToLocalChecked();

        response_obj->Set(context, utilities::StringToV8String(isolate, "reply"), reply_fn).Check();
        response_obj->Set(context, utilities::StringToV8String(isolate, "text"), text_fn).Check();
        response_obj->Set(context, utilities::StringToV8String(isolate, "json"), json_fn).Check();
        response_obj->Set(context, utilities::StringToV8String(isolate, "arrayBuffer"), arraybuffer_fn).Check();
        response_obj->Set(context, utilities::StringToV8String(isolate, "clone"), clone_fn).Check();
        response_obj->Set(context, utilities::StringToV8String(isolate, "error"), error_fn).Check();

        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
        return response_obj;
    }

} // namespace slim::plugin::http
