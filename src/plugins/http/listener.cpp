#include "http_plugin.h"

#include <slim/common/log.h>
#include <slim/utilities.h>
#include <slim/runtime.h>
#include <slim/slim.h>
#include <slim/slim_v8.h>

namespace slim::plugin::http {
    using namespace slim::common;

    struct WeakListenerData {
        std::shared_ptr<ListenerState>* persistent_state;
        v8::Global<v8::Object>*         persistent_listener;
    };

    void listener_async_iterator(const v8::FunctionCallbackInfo<v8::Value>& args) {
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
        args.GetReturnValue().Set(args.This());
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
                resolver->Reject(context, utilities::StringToV8String(isolate, "failed to set value")).Check();
                return;
            }
            if (iter_result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, false)).IsNothing()) {
                log::error(log::Message(__func__, "failed to set done", __FILE__, __LINE__));
                resolver->Reject(context, utilities::StringToV8String(isolate, "failed to set done")).Check();
                return;
            }
            if (resolver->Resolve(context, iter_result).IsNothing()) {
                log::error(log::Message(__func__, "failed to resolve promise", __FILE__, __LINE__));
                // Resolve already failed; nothing to reject — just log and return
                return;
            }
            log::debug(log::Message(__func__, "promise resolved from buffer", __FILE__, __LINE__));
        } else {
            log::debug(log::Message(__func__, "storing pending resolver", __FILE__, __LINE__));
            state->pending_resolver.Reset(isolate, resolver);
        }

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

        // use persistent_state->get() so the raw pointer remains valid after local state goes out of scope
        v8::Local<v8::External> state_external = v8::External::New(isolate, persistent_state->get());

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
        auto* weak_listener_data = new WeakListenerData{ persistent_state, persistent_listener };

        // weak callback: safety net for GC-before-shutdown; no-op if cleanup hook already ran
        persistent_listener->SetWeak(weak_listener_data,
            [](const v8::WeakCallbackInfo<WeakListenerData>& data) {
                log::debug(log::Message(__func__, "listener GC'd, releasing persistent_state and persistent_listener", __FILE__, __LINE__));
                auto* wld = data.GetParameter();
                if (wld->persistent_state) {
                    (*wld->persistent_state)->tcp_server.reset();
                    delete wld->persistent_state;
                    wld->persistent_state = nullptr;
                }
                if (wld->persistent_listener) {
                    delete wld->persistent_listener;
                    wld->persistent_listener = nullptr;
                }
                delete wld;
            },
            v8::WeakCallbackType::kParameter);

        // register explicit cleanup hook: runs after stop is requested, before isolate disposal.
        // breaks ListenerState → tcp_server → lambda → ListenerState cycle, then frees all bookkeeping.
        slim::v_8::register_cleanup(isolate, [weak_listener_data]() {
            log::debug(log::Message(__func__, "cleanup hook: releasing ListenerState and persistent_listener", __FILE__, __LINE__));
            if (weak_listener_data->persistent_state) {
                (*weak_listener_data->persistent_state)->tcp_server.reset();
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

        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
        args.GetReturnValue().Set(listener);
    }

} // namespace slim::plugin::http
