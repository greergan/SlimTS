#include "http_plugin.h"

#include <slim/common/log.h>
#include <slim/utilities.h>
#include <slim/runtime.h>
#include <slim/slim.h>

namespace slim::plugin::http {
    using namespace slim::common;

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
        std::weak_ptr<ListenerState> weak_state = state;
        state->tcp_server = std::make_unique<slim::common::network::server::Tcp>(
            listen_config, slim::runtime::instance(), stop_token,
            [weak_state](slim::common::io::Scheduler& sched, int fd, SSL_CTX* ssl_ctx)
                -> slim::common::io::Task<void> {
                auto s = weak_state.lock();
                if (!s) co_return;
                co_return co_await connection_handler(sched, fd, ssl_ctx, std::move(s));
            });
        log::debug(log::Message(__func__, "tcp server started", __FILE__, __LINE__));

        // build listener object — all error paths return here before any heap ownership is established
        v8::Local<v8::Object> listener = v8::Object::New(isolate);
        v8::Local<v8::External> state_external = v8::External::New(isolate, state.get());

        v8::Local<v8::FunctionTemplate> next_tpl = v8::FunctionTemplate::New(isolate, listener_next, state_external);
        v8::Local<v8::Function> next_func;
        if (!next_tpl->GetFunction(context).ToLocal(&next_func)) {
            log::error(log::Message(__func__, "failed to get next function", __FILE__, __LINE__));
            return;
        }
        if (listener->Set(context, utilities::StringToV8String(isolate, "next"), next_func).IsNothing()) {
            log::error(log::Message(__func__, "failed to set next on listener", __FILE__, __LINE__));
            return;
        }
        log::debug(log::Message(__func__, "next function set on listener", __FILE__, __LINE__));

        v8::Local<v8::FunctionTemplate> iter_tpl = v8::FunctionTemplate::New(isolate, listener_async_iterator);
        v8::Local<v8::Function> iter_func;
        if (!iter_tpl->GetFunction(context).ToLocal(&iter_func)) {
            log::error(log::Message(__func__, "failed to get asyncIterator function", __FILE__, __LINE__));
            return;
        }
        if (listener->Set(context, v8::Symbol::GetAsyncIterator(isolate), iter_func).IsNothing()) {
            log::error(log::Message(__func__, "failed to set asyncIterator on listener", __FILE__, __LINE__));
            return;
        }
        log::debug(log::Message(__func__, "asyncIterator set on listener", __FILE__, __LINE__));

        // all setup succeeded — now establish heap ownership
        auto* persistent_state = new std::shared_ptr<ListenerState>(state);

        // persistent_listener stored on state so stop callback can clean it up
        // before the isolate is disposed — GC weak is not used because the
        // isolate tears down before GC runs, leaving the Global leaked.
        auto* persistent_listener = new v8::Global<v8::Object>(isolate, listener);
        state->persistent_listener = persistent_listener;

        state->stop_cb = new std::stop_callback<std::function<void()>>(slim::get_stop_token(), [persistent_state]() {
            log::debug(log::Message(__func__, "stop requested, releasing ListenerState", __FILE__, __LINE__));
            auto& state = *persistent_state;
            if (state->persistent_listener) {
                state->persistent_listener->Reset();
                delete state->persistent_listener;
                state->persistent_listener = nullptr;
            }
            // reset all v8::Globals while the isolate is still alive — the stop
            // callback fires before dispose_isolate, but ~ListenerState may run
            // after it if other shared_ptr refs are still outstanding at that point.
            // ~v8::Global() on a disposed isolate is UB / hang.
            state->pending_resolver.Reset();
            state->pending_requests.clear();
            // null out stop_cb before delete persistent_state — if this is the last
            // shared_ptr ref, ~ListenerState runs here and would call delete stop_cb,
            // but std::stop_callback dtor blocks until the running callback returns →
            // deadlock. nulling first skips the delete in ~ListenerState.
            // the stop_callback itself leaks, which is acceptable at process shutdown.
            state->stop_cb = nullptr;
            delete persistent_state;
        });

        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
        args.GetReturnValue().Set(listener);
    }

} // namespace slim::plugin::http
