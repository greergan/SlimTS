#include <v8.h>
#include <deque>
#include <format>
#include <memory>
#include <string>
#include <slim/common/log.h>
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
        std::deque<std::string> pending_paths;
        std::unique_ptr<slim::common::network::server::Tcp> tcp_server;
        explicit ListenerState(v8::Isolate* iso) : isolate(iso) {}
    };

    slim::common::io::Task<void> connection_handler(slim::common::io::Scheduler& scheduler, int fd, SSL_CTX* ssl_ctx, std::shared_ptr<ListenerState> state) {
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
        log::debug(log::Message(__func__, "fd => " + std::to_string(fd), __FILE__, __LINE__));

        auto connection = co_await slim::common::network::client::http::Connection::create(scheduler, fd, nullptr);
        log::debug(log::Message(__func__, "connection created", __FILE__, __LINE__));

        std::vector<uint8_t> buf;
        log::debug(log::Message(__func__, "calling read", __FILE__, __LINE__));
        connection.read(buf);
        log::debug(log::Message(__func__, "read complete, bytes => " + std::to_string(buf.size()), __FILE__, __LINE__));

        std::string request(buf.begin(), buf.end());
        auto first_line_end = request.find("\r\n");
        std::string first_line = request.substr(0, first_line_end);
        log::debug(log::Message(__func__, "first line => " + first_line, __FILE__, __LINE__));

        auto path_start = first_line.find(' ') + 1;
        auto path_end = first_line.find(' ', path_start);
        std::string path = first_line.substr(path_start, path_end - path_start);
        log::debug(log::Message(__func__, "parsed path => " + path, __FILE__, __LINE__));

        constexpr std::string_view body = "<html><body><h1>slim is alive</h1><p>request received</p></body></html>";
        auto response = std::format("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{}", body.size(), body);
        log::debug(log::Message(__func__, "writing response", __FILE__, __LINE__));
        connection.write(response);
        log::debug(log::Message(__func__, "response written", __FILE__, __LINE__));

        slim::isolate_wake::post(state->isolate, [state, path_val = std::move(path)]() {
            log::trace(log::Message(__func__, "resolve task begins", __FILE__, __LINE__));
            auto* isolate = state->isolate;
            auto context = isolate->GetCurrentContext();

            if (state->pending_resolver.IsEmpty()) {
                log::debug(log::Message(__func__, "no resolver waiting, buffering path => " + path_val, __FILE__, __LINE__));
                state->pending_paths.push_back(std::move(path_val));
                return;
            }

            v8::Local<v8::Promise::Resolver> resolver = state->pending_resolver.Get(isolate);
            state->pending_resolver.Reset();
            log::debug(log::Message(__func__, "resolving promise for path => " + path_val, __FILE__, __LINE__));

            v8::Local<v8::Object> request_obj = v8::Object::New(isolate);
            if (request_obj->Set(context, utilities::StringToV8String(isolate, "url"), utilities::StringToV8String(isolate, path_val)).IsNothing()) {
                log::error(log::Message(__func__, "failed to set url", __FILE__, __LINE__));
                return;
            }
            v8::Local<v8::Object> event_obj = v8::Object::New(isolate);
            if (event_obj->Set(context, utilities::StringToV8String(isolate, "request"), request_obj).IsNothing()) {
                log::error(log::Message(__func__, "failed to set request", __FILE__, __LINE__));
                return;
            }
            v8::Local<v8::Object> iter_result = v8::Object::New(isolate);
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

        if (!state->pending_paths.empty()) {
            auto path_val = std::move(state->pending_paths.front());
            state->pending_paths.pop_front();
            log::debug(log::Message(__func__, "draining buffered path => " + path_val, __FILE__, __LINE__));

            auto request_obj = v8::Object::New(isolate);
            if (request_obj->Set(context, utilities::StringToV8String(isolate, "url"), utilities::StringToV8String(isolate, path_val)).IsNothing()) {
                log::error(log::Message(__func__, "failed to set url", __FILE__, __LINE__));
                return;
            }
            auto event_obj = v8::Object::New(isolate);
            if (event_obj->Set(context, utilities::StringToV8String(isolate, "request"), request_obj).IsNothing()) {
                log::error(log::Message(__func__, "failed to set request", __FILE__, __LINE__));
                return;
            }
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
            }
        );
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
