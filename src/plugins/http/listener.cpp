#include "http_plugin.h"
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/runtime.h>
#include <slim/service/handles.h>
#include <slim/slim.h>
#include <slim/slim_v8.h>
#include <slim/utilities.h>

namespace slim::plugin::http {
    using namespace slim::common;

    struct WeakListenerData {
        std::shared_ptr<ListenerState>* persistent_state;
        v8::Global<v8::Object>*         persistent_listener;
    };

    void listener_async_iterator(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
        args.GetReturnValue().Set(args.This());
#ifdef ENABLE_LOGGING
        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
    }

    void listener_next(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
        auto* isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        auto maybe_resolver = v8::Promise::Resolver::New(context);

        v8::Local<v8::Promise::Resolver> resolver;
        if (!maybe_resolver.ToLocal(&resolver)) {
#ifdef ENABLE_LOGGING
            log::error(log::Message(__func__, "failed to create promise resolver", __FILE__, __LINE__));
#endif
            return;
        }
        args.GetReturnValue().Set(resolver->GetPromise());

        auto* state = static_cast<ListenerState*>(args.Data().As<v8::External>()->Value());

        if (!state->tcp_server) {
#ifdef ENABLE_LOGGING
            log::debug(log::Message(__func__, "tcp_server is null, resolving done", __FILE__, __LINE__));
#endif
            auto iter_result = v8::Object::New(isolate);
            iter_result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, true)).Check();
            resolver->Resolve(context, iter_result).Check();
            return;
        }

        if (!state->pending_requests.empty()) {
            auto event_global = std::move(state->pending_requests.front());
            state->pending_requests.pop_front();
            auto event_obj = event_global.Get(isolate);
#ifdef ENABLE_LOGGING
            log::debug(log::Message(__func__, "draining buffered request", __FILE__, __LINE__));
#endif
            auto iter_result = v8::Object::New(isolate);
            if (iter_result->Set(context, utilities::StringToV8String(isolate, "value"), event_obj).IsNothing()) {
#ifdef ENABLE_LOGGING
                log::error(log::Message(__func__, "failed to set value", __FILE__, __LINE__));
#endif
                resolver->Reject(context, utilities::StringToV8String(isolate, "failed to set value")).Check();
                return;
            }
            if (iter_result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, false)).IsNothing()) {
#ifdef ENABLE_LOGGING
                log::error(log::Message(__func__, "failed to set done", __FILE__, __LINE__));
#endif
                resolver->Reject(context, utilities::StringToV8String(isolate, "failed to set done")).Check();
                return;
            }
            if (resolver->Resolve(context, iter_result).IsNothing()) {
#ifdef ENABLE_LOGGING
                log::error(log::Message(__func__, "failed to resolve promise", __FILE__, __LINE__));
#endif
                return;
            }
#ifdef ENABLE_LOGGING
            log::debug(log::Message(__func__, "promise resolved from buffer", __FILE__, __LINE__));
#endif
        } else {
#ifdef ENABLE_LOGGING
            log::debug(log::Message(__func__, "storing pending resolver", __FILE__, __LINE__));
#endif
            state->pending_resolver.Reset(isolate, resolver);
        }
#ifdef ENABLE_LOGGING
        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
    }

    void listener_stop(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
        auto* isolate = args.GetIsolate();
        auto* state = static_cast<ListenerState*>(args.Data().As<v8::External>()->Value());

        if (state && state->tcp_server) {
            if (!state->pending_resolver.IsEmpty()) {
                auto context = isolate->GetCurrentContext();
                auto resolver = state->pending_resolver.Get(isolate);
                auto iter_result = v8::Object::New(isolate);
                iter_result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, true)).Check();
                resolver->Resolve(context, iter_result).Check();
                state->pending_resolver.Reset();
            }
            state->tcp_server.reset();
            slim::service::handles::decrement();
        }
#ifdef ENABLE_LOGGING
        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
    }

    void serve(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
        slim::service::handles::increment();
        auto* isolate = args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        v8::HandleScope handle_scope(isolate);

        auto config_obj = args[0].As<v8::Object>();
        auto host = utilities::StringValue(isolate, "host", config_obj);
        auto port = utilities::V8ValueToInt(isolate, utilities::GetValue(isolate, "port", config_obj));
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, "host => " + host + ", port => " + std::to_string(port), __FILE__, __LINE__));
#endif
        auto state = std::make_shared<ListenerState>(isolate);
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, "ListenerState created", __FILE__, __LINE__));
#endif
        auto stop_token = slim::get_stop_token();
        slim::common::network::server::tcp::Config listen_config{host, port, "", ""};
        state->tcp_server = std::make_unique<slim::common::network::server::Tcp>(listen_config, slim::runtime::instance(),
            stop_token, [state](slim::common::io::Scheduler& sched, int fd, SSL_CTX* ssl_ctx) {
                return connection_handler(sched, fd, ssl_ctx, state);
            });
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, "tcp server started", __FILE__, __LINE__));
#endif
        auto* persistent_state = new std::shared_ptr<ListenerState>(state);
        v8::Local<v8::External> state_external = v8::External::New(isolate, persistent_state->get());
        v8::Local<v8::Object> listener = v8::Object::New(isolate);
        v8::Local<v8::FunctionTemplate> next_tpl = v8::FunctionTemplate::New(isolate, listener_next, state_external);
        v8::Local<v8::Function> next_func;
        if (!next_tpl->GetFunction(context).ToLocal(&next_func)) {
#ifdef ENABLE_LOGGING
            log::error(log::Message(__func__, "failed to get next function", __FILE__, __LINE__));
#endif
            delete persistent_state;
            return;
        }
        if (listener->Set(context, utilities::StringToV8String(isolate, "next"), next_func).IsNothing()) {
#ifdef ENABLE_LOGGING
            log::error(log::Message(__func__, "failed to set next on listener", __FILE__, __LINE__));
#endif
            delete persistent_state;
            return;
        }
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, "next function set on listener", __FILE__, __LINE__));
#endif
        v8::Local<v8::FunctionTemplate> iter_tpl = v8::FunctionTemplate::New(isolate, listener_async_iterator);
        v8::Local<v8::Function> iter_func;
        if (!iter_tpl->GetFunction(context).ToLocal(&iter_func)) {
#ifdef ENABLE_LOGGING
            log::error(log::Message(__func__, "failed to get asyncIterator function", __FILE__, __LINE__));
#endif
            delete persistent_state;
            return;
        }
        if (listener->Set(context, v8::Symbol::GetAsyncIterator(isolate), iter_func).IsNothing()) {
#ifdef ENABLE_LOGGING
            log::error(log::Message(__func__, "failed to set asyncIterator on listener", __FILE__, __LINE__));
#endif
            delete persistent_state;
            return;
        }
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, "asyncIterator set on listener", __FILE__, __LINE__));
#endif
        v8::Local<v8::FunctionTemplate> stop_tpl = v8::FunctionTemplate::New(isolate, listener_stop, state_external);
        v8::Local<v8::Function> stop_func;
        if (stop_tpl->GetFunction(context).ToLocal(&stop_func)) {
            listener->Set(context, utilities::StringToV8String(isolate, "stop"), stop_func).Check();
        }

        auto* persistent_listener = new v8::Global<v8::Object>(isolate, listener);
        auto* weak_listener_data = new WeakListenerData{ persistent_state, persistent_listener };

        // weak callback: safety net for GC-before-shutdown; no-op if cleanup hook already ran
        persistent_listener->SetWeak(weak_listener_data, [](const v8::WeakCallbackInfo<WeakListenerData>& data) {
#ifdef ENABLE_LOGGING
                log::debug(log::Message(__func__, "listener GC'd, releasing persistent_state and persistent_listener", __FILE__, __LINE__));
#endif
                auto* wld = data.GetParameter();
                if (wld->persistent_state) {
                    delete wld->persistent_state;
                    wld->persistent_state = nullptr;
                }
                if (wld->persistent_listener) {
                    delete wld->persistent_listener;
                    wld->persistent_listener = nullptr;
                }
                //delete wld;
            },
            v8::WeakCallbackType::kParameter);

        // register explicit cleanup hook: runs after stop is requested, before isolate disposal.
        // breaks ListenerState → tcp_server → lambda → ListenerState cycle, then frees all bookkeeping.
        slim::v_8::register_cleanup(isolate, [weak_listener_data]() {
#ifdef ENABLE_LOGGING
            log::debug(log::Message(__func__, "cleanup hook: releasing ListenerState and persistent_listener", __FILE__, __LINE__));
#endif
            if (weak_listener_data->persistent_state) {
                delete weak_listener_data->persistent_state;
                weak_listener_data->persistent_state = nullptr;
            }
            if (weak_listener_data->persistent_listener) {
                weak_listener_data->persistent_listener->ClearWeak();
                weak_listener_data->persistent_listener->Reset();
                delete weak_listener_data->persistent_listener;
                weak_listener_data->persistent_listener = nullptr;
            }
            delete weak_listener_data;
        });

#ifdef ENABLE_LOGGING
        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
        args.GetReturnValue().Set(listener);
    }
} // namespace slim::plugin::http
