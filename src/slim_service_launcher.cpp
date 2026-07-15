#include <format>
#include <memory>
#include <variant>
#include <vector>
#include <queue>
#include <v8.h>
#include <slim/builtins/dummy_console_provider.h>
#include <slim/common/log.h>
#include <slim/common/memory_mapper.h>
#include <slim/module/import_specifier.h>
#include <slim/module/resolver.h>
#include <slim/plugin.hpp>
#include <slim/plugin/loader.h>
#include <slim/service/launcher.h>
#include <slim/queue/queue.h>
#include <slim/slim_v8.h>
namespace slim::launchable_service::bin {
	extern slim::common::memory_mapper::map_container bin_map_container;
}
namespace slim::launchable_service::typescript::library {
	extern slim::common::memory_mapper::map_container typescript_library_container;
}
namespace slim::service::launcher {
	using namespace slim::common;
}
void slim::service::launcher::launch(slim::module::variant_specifier _script_name_string_or_specifier_stub) {
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
	std::string specifier_handle;
	if(std::holds_alternative<slim::module::specifier_stub>(_script_name_string_or_specifier_stub)) {
		auto specifier_stub_ptr = std::get<slim::module::specifier_stub>(_script_name_string_or_specifier_stub);
		specifier_handle = specifier_stub_ptr.specifier_url();
	}
	else {
		specifier_handle = std::get<std::string>(_script_name_string_or_specifier_stub);
	}

	auto* isolate = slim::v_8::new_isolate(specifier_handle);
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

	// must support structuredClone() to deep copy objects

	auto context = v8::Context::New(isolate, NULL, global_template);
	log::debug(log::Message(__func__,"created context",__FILE__, __LINE__));

	v8::Context::Scope context_scope(context);
	log::debug(log::Message(__func__,"created context scope",__FILE__, __LINE__));

	slim::plugin::plugin slim_objects(isolate, "slim");
	slim_objects.add_function("load", slim::plugin::loader::load);
	slim_objects.expose_plugin();
	log::debug(log::Message(__func__,"created slim load function",__FILE__, __LINE__));

	auto module_import_specifier_pointer = slim::module::resolver::resolve_imports(isolate, _script_name_string_or_specifier_stub, true);
	log::debug(log::Message(__func__,"v8_module_status_string() => " + module_import_specifier_pointer->v8_module_status_string(),__FILE__, __LINE__));

	if(module_import_specifier_pointer->v8_module()->GetStatus() == v8::Module::Status::kErrored) {
		isolate->ThrowException(module_import_specifier_pointer->v8_module()->GetException());
		return;
	}
	else {
		auto result = module_import_specifier_pointer->v8_module()->Evaluate(context);
		switch(module_import_specifier_pointer->v8_module()->GetStatus()) {
			case v8::Module::Status::kErrored:
				isolate->ThrowException(module_import_specifier_pointer->v8_module()->GetException());
				break;
			default:
				log::debug(log::Message(__func__, "checking for stalled top level await modules",__FILE__, __LINE__));
				auto await_message_module_pair = module_import_specifier_pointer->v8_module()->GetStalledTopLevelAwaitMessages(isolate);
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
				log::debug(log::Message(__func__,"module status after evaluation => " + module_import_specifier_pointer->v8_module_status_string(),__FILE__, __LINE__));
		}
	}

	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
}
void slim::service::launcher::marshal_resources() {
	log::trace(log::Message("slim::service::launcher::marshal_resources()","begins",__FILE__, __LINE__));
/* 	memory_mapper::create("configurations");
	memory_mapper::create("raw_source_code_storage");
	memory_mapper::create("compiled_source_code_storage");
	memory_mapper::create("intermediate_source_code_storage"); */
	log::debug(log::Message("slim::service::launcher::marshal_resources()","attaching bin_map_container",__FILE__, __LINE__));
	slim::common::memory_mapper::attach("launchable_service_bin", std::make_shared<slim::common::memory_mapper::map_container>(slim::launchable_service::bin::bin_map_container));
	log::debug(log::Message("slim::service::launcher::marshal_resources()","attached bin_map_container",__FILE__, __LINE__));
	log::debug(log::Message("slim::service::launcher::marshal_resources()","attaching typescript_library_container",__FILE__, __LINE__));
	slim::common::memory_mapper::attach("typescript_library", std::make_shared<slim::common::memory_mapper::map_container>(slim::launchable_service::typescript::library::typescript_library_container));
	log::debug(log::Message("slim::service::launcher::marshal_resources()","attached typescript_library_container",__FILE__, __LINE__));
	log::trace(log::Message("slim::service::launcher::marshal_resources()","ends",__FILE__, __LINE__));
}
