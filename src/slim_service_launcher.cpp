#include <format>
#include <memory>
#include <variant>
#include <vector>
#include <queue>
#include <chrono>
#include <thread>
#include <signal.h>
#include <v8.h>
#include <slim/builtins/dummy_console_provider.h>
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/common/memory/mapper.h>
#include <slim/module/import_specifier.h>
#include <slim/module/resolver.h>
#include <slim/plugin.hpp>
#include <slim/plugin/loader.h>
#include <slim/service/launcher.h>
#include <slim/queue/queue.h>
#include <slim/slim_v8.h>
#include <slim/exception_handler.h>
#include <slim/isolate_wake.h>
#include <slim/slim.h>
#include <slim/utilities.h>
#include <slim/slim_v8.h>
#include <slim/service/handles.h>

namespace slim::service::launcher {
    using namespace slim::common;
}

void slim::service::launcher::launch(std::string_view specifier_uri) {
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
#endif
    auto* isolate = slim::v_8::new_isolate(std::string(specifier_uri));
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__,"created new isolate",__FILE__, __LINE__));
#endif

    v8::Isolate::Scope isolate_scope(isolate);
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__,"created isolate scope",__FILE__, __LINE__));
#endif

    v8::HandleScope handle_scope(isolate);
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__,"created handle scope",__FILE__, __LINE__));
#endif

    auto global_template = v8::ObjectTemplate::New(isolate);
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__,"created global_template",__FILE__, __LINE__));
#endif

    auto no_content = [](const v8::FunctionCallbackInfo<v8::Value>& args){};
    global_template->Set(isolate, "setTimeout", v8::FunctionTemplate::New(isolate, no_content));
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__,"created setTimeout stub on the global_template",__FILE__, __LINE__));
#endif

    global_template->Set(isolate, "clearTimeout", v8::FunctionTemplate::New(isolate, no_content));
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__,"created clearTimeout stub on the global_template",__FILE__, __LINE__));
#endif

    auto context = v8::Context::New(isolate, NULL, global_template);
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__,"created context",__FILE__, __LINE__));
#endif

    v8::Context::Scope context_scope(context);
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__,"created context scope",__FILE__, __LINE__));
#endif

    slim::plugin::plugin slim_objects(isolate, "slim");
    slim_objects.add_function("load", slim::plugin::loader::load);
    slim_objects.expose_plugin();
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__,"created slim load function",__FILE__, __LINE__));
#endif

    auto module_import_specifier_optional = slim::module::resolver::resolve_imports(isolate, specifier_uri, true);
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__, std::format("resolve_imports returned has_value => {}", module_import_specifier_optional.has_value()), __FILE__, __LINE__));
#endif

    if(!module_import_specifier_optional.has_value()) {
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, "resolve_imports returned empty", __FILE__, __LINE__));
#endif
#ifdef ENABLE_LOGGING
        log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
#endif
        return;
    }
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__, "has_value check passed, getting module_import_specifier", __FILE__, __LINE__));
#endif
    slim::module::import_specifier& module_import_specifier = module_import_specifier_optional.value().get();

#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__, std::format("module status before evaluate => {}", static_cast<int>(module_import_specifier.v8_module()->GetStatus())), __FILE__, __LINE__));
#endif
    if(module_import_specifier.v8_module()->GetStatus() == v8::Module::Status::kErrored) {
        isolate->ThrowException(module_import_specifier.v8_module()->GetException());
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, "module status is kErrored before evaluate", __FILE__, __LINE__));
#endif
#ifdef ENABLE_LOGGING
        log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
#endif
        return;
    }
    else {
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, "calling Evaluate on context", __FILE__, __LINE__));
#endif
        auto& wake = slim::isolate_wake::register_isolate(isolate);
        v8::TryCatch try_catch(isolate);
        auto result = module_import_specifier.v8_module()->Evaluate(context);
        v8::Local<v8::Promise> module_promise;
        if (!result.IsEmpty() && result.ToLocalChecked()->IsPromise()) {
            module_promise = result.ToLocalChecked().As<v8::Promise>();
        }
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, std::format("Evaluate returned is_empty => {}", result.IsEmpty()), __FILE__, __LINE__));
#endif

        auto stop_token = slim::get_stop_token();
        int gc_counter = 0;
        while (!stop_token.stop_requested()) {
            bool module_finished = module_promise.IsEmpty() || module_promise->State() != v8::Promise::PromiseState::kPending;
            if (module_finished && slim::service::handles::count() == 0) {
                slim::stop();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            slim::isolate_wake::drain(isolate);
            v8::platform::PumpMessageLoop(slim::v_8::get_platform(), isolate);
            isolate->PerformMicrotaskCheckpoint();
            if (++gc_counter >= 1000) {
                isolate->LowMemoryNotification();
#ifdef ENABLE_LOGGING
                v8::HeapStatistics hs;
                isolate->GetHeapStatistics(&hs);
                log::debug(log::Message(__func__,
                    "heap used => " + std::to_string(hs.used_heap_size() / 1024) + "kb"
                    " total => " + std::to_string(hs.total_heap_size() / 1024) + "kb", __FILE__, __LINE__));
#endif
                gc_counter = 0;
            }
        }
        slim::v_8::run_cleanup(isolate);
        slim::isolate_wake::unregister_isolate(isolate);
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, "stop requested, exiting keep alive loop", __FILE__, __LINE__));
#endif

        if(try_catch.HasCaught()) {
#ifdef ENABLE_LOGGING
            log::debug(log::Message(__func__, std::format("Evaluate threw an exception => {}", utilities::v8StringToString(isolate, try_catch.Message()->Get())), __FILE__, __LINE__));
#endif
            slim::exception_handler::v8_try_catch_handler(&try_catch);
        }
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, std::format("module status after evaluate => {}", static_cast<int>(module_import_specifier.v8_module()->GetStatus())), __FILE__, __LINE__));
#endif
        switch(module_import_specifier.v8_module()->GetStatus()) {
            case v8::Module::Status::kErrored:
#ifdef ENABLE_LOGGING
                log::debug(log::Message(__func__, std::format("module exception after evaluate => {}", utilities::v8ValueToString(isolate, module_import_specifier.v8_module()->GetException())), __FILE__, __LINE__));
#endif
#ifdef ENABLE_LOGGING
                log::debug(log::Message(__func__, "module status is kErrored after evaluate", __FILE__, __LINE__));
#endif
                {
                    v8::TryCatch module_try_catch(isolate);
                    isolate->ThrowException(module_import_specifier.v8_module()->GetException());
                    slim::exception_handler::v8_try_catch_handler(&module_try_catch);
                }
#ifdef ENABLE_LOGGING
                log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
#endif
                break;
            default:
#ifdef ENABLE_LOGGING
                log::debug(log::Message(__func__, "checking for stalled top level await modules",__FILE__, __LINE__));
#endif
                auto await_message_module_pair = module_import_specifier.v8_module()->GetStalledTopLevelAwaitMessages(isolate);
                auto num_stalled_awaits = await_message_module_pair.first.size();
                if(num_stalled_awaits > 0) {
#ifdef ENABLE_LOGGING
                    log::debug(log::Message(__func__,"number of stalled top level await modules => "
                        + std::to_string(num_stalled_awaits),__FILE__, __LINE__));
#endif
                    auto number_of_messages = await_message_module_pair.second.size();
#ifdef ENABLE_LOGGING
                    log::debug(log::Message(__func__,"number of stalled top level await messages => "
                        + std::to_string(number_of_messages),__FILE__, __LINE__));
#endif
                }
                else {
#ifdef ENABLE_LOGGING
                    log::debug(log::Message(__func__, "no stalled top level await objects",__FILE__, __LINE__));
#endif
                }
        }
    }

#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
#endif
}
