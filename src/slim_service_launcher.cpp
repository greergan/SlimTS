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
#include <slim/common/log.h>
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

namespace slim::service::launcher {
    using namespace slim::common;
}

void slim::service::launcher::launch(std::string_view specifier_uri) {
    log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
    // block signals on this thread — main thread handles them
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &mask, nullptr);

    auto* isolate = slim::v_8::new_isolate(std::string(specifier_uri));
    log::debug(log::Message(__func__,"created new isolate",__FILE__, __LINE__));

    v8::Isolate::Scope isolate_scope(isolate);
    log::debug(log::Message(__func__,"created isolate scope",__FILE__, __LINE__));

    v8::HandleScope handle_scope(isolate);
    log::debug(log::Message(__func__,"created handle scope",__FILE__, __LINE__));

    auto global_template = v8::ObjectTemplate::New(isolate);
    log::debug(log::Message(__func__,"created global_template",__FILE__, __LINE__));

    auto no_content = [](const v8::FunctionCallbackInfo<v8::Value>& args){};
    global_template->Set(isolate, "setTimeout", v8::FunctionTemplate::New(isolate, no_content));
    log::debug(log::Message(__func__,"created setTimeout stub on the global_template",__FILE__, __LINE__));

    global_template->Set(isolate, "clearTimeout", v8::FunctionTemplate::New(isolate, no_content));
    log::debug(log::Message(__func__,"created clearTimeout stub on the global_template",__FILE__, __LINE__));

    auto context = v8::Context::New(isolate, NULL, global_template);
    log::debug(log::Message(__func__,"created context",__FILE__, __LINE__));

    v8::Context::Scope context_scope(context);
    log::debug(log::Message(__func__,"created context scope",__FILE__, __LINE__));

    slim::plugin::plugin slim_objects(isolate, "slim");
    slim_objects.add_function("load", slim::plugin::loader::load);
    slim_objects.expose_plugin();
    log::debug(log::Message(__func__,"created slim load function",__FILE__, __LINE__));

    auto module_import_specifier_optional = slim::module::resolver::resolve_imports(isolate, specifier_uri, true);
    log::debug(log::Message(__func__, std::format("resolve_imports returned has_value => {}", module_import_specifier_optional.has_value()), __FILE__, __LINE__));

    if(!module_import_specifier_optional.has_value()) {
        log::debug(log::Message(__func__, "resolve_imports returned empty", __FILE__, __LINE__));
        log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
        return;
    }
    log::debug(log::Message(__func__, "has_value check passed, getting module_import_specifier", __FILE__, __LINE__));
    slim::module::import_specifier& module_import_specifier = module_import_specifier_optional.value().get();

    log::debug(log::Message(__func__, std::format("module status before evaluate => {}", static_cast<int>(module_import_specifier.v8_module()->GetStatus())), __FILE__, __LINE__));
    if(module_import_specifier.v8_module()->GetStatus() == v8::Module::Status::kErrored) {
        isolate->ThrowException(module_import_specifier.v8_module()->GetException());
        log::debug(log::Message(__func__, "module status is kErrored before evaluate", __FILE__, __LINE__));
        log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
        return;
    }
    else {
        log::debug(log::Message(__func__, "calling Evaluate on context", __FILE__, __LINE__));
        v8::TryCatch try_catch(isolate);
        auto result = module_import_specifier.v8_module()->Evaluate(context);
        log::debug(log::Message(__func__, std::format("Evaluate returned is_empty => {}", result.IsEmpty()), __FILE__, __LINE__));
        log::debug(log::Message(__func__, "pumping microtask queue", __FILE__, __LINE__));
        isolate->PerformMicrotaskCheckpoint();
        log::debug(log::Message(__func__, "microtask queue pumped", __FILE__, __LINE__));

        // keep alive until stop is requested
        auto stop_token = slim::get_stop_token();
        auto& wake = slim::isolate_wake::register_isolate(isolate);
        std::stop_callback stop_cb(stop_token, [isolate]{
            log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
            log::debug(log::Message(__func__, "stop_callback fired, signaling isolate wake", __FILE__, __LINE__));
            slim::isolate_wake::signal(isolate);
            log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
        });
        while (!stop_token.stop_requested()) {
            wake.semaphore.acquire();
            slim::isolate_wake::drain(isolate);
            isolate->PerformMicrotaskCheckpoint();
        }
        slim::isolate_wake::unregister_isolate(isolate);
        log::debug(log::Message(__func__, "stop requested, exiting keep alive loop", __FILE__, __LINE__));

        if(try_catch.HasCaught()) {
            log::debug(log::Message(__func__, std::format("Evaluate threw an exception => {}", utilities::v8StringToString(isolate, try_catch.Message()->Get())), __FILE__, __LINE__));
            slim::exception_handler::v8_try_catch_handler(&try_catch);
        }
        log::debug(log::Message(__func__, std::format("module status after evaluate => {}", static_cast<int>(module_import_specifier.v8_module()->GetStatus())), __FILE__, __LINE__));
        switch(module_import_specifier.v8_module()->GetStatus()) {
            case v8::Module::Status::kErrored:
                log::debug(log::Message(__func__, std::format("module exception after evaluate => {}", utilities::v8ValueToString(isolate, module_import_specifier.v8_module()->GetException())), __FILE__, __LINE__));
                log::debug(log::Message(__func__, "module status is kErrored after evaluate", __FILE__, __LINE__));
                {
                    v8::TryCatch module_try_catch(isolate);
                    isolate->ThrowException(module_import_specifier.v8_module()->GetException());
                    slim::exception_handler::v8_try_catch_handler(&module_try_catch);
                }
                log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
                break;
            default:
                log::debug(log::Message(__func__, "checking for stalled top level await modules",__FILE__, __LINE__));
                auto await_message_module_pair = module_import_specifier.v8_module()->GetStalledTopLevelAwaitMessages(isolate);
                auto number_of_stalled_to_level_await_modules = await_message_module_pair.first.size();
                if(number_of_stalled_to_level_await_modules > 0) {
                    log::debug(log::Message(__func__,"number of stalled top level await modules => "
                        + std::to_string(number_of_stalled_to_level_await_modules),__FILE__, __LINE__));
                    auto number_of_messages = await_message_module_pair.second.size();
                    log::debug(log::Message(__func__,"number of stalled top level await messages => "
                        + std::to_string(number_of_messages),__FILE__, __LINE__));
                }
                else {
                    log::debug(log::Message(__func__, "no stalled top level await objects",__FILE__, __LINE__));
                }
        }
    }

    log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
}
