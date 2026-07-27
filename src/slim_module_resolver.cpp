#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <vector>
#include <v8.h>
#include <slim/common/exception.h>
#include <slim/common/log.h>
#include <slim/exception_handler.h>
#include <slim/module/import_specifier.h>
#include <slim/module/resolver.h>
#include <slim/plugin/loader.h>
#include <slim/utilities.h>
namespace slim::module::resolver {
	using namespace slim;
	using namespace slim::common;
	static std::set<std::string> plugins_set{"console","fs","http","kafka","os","path","process","memoryAdaptor","queue"};
	static specifier_cache cache;
	// maps synthetic module identity hash to plugin name, used by synthetic_module_evaluation_steps
	static std::map<int, std::string> synthetic_module_plugin_names;
}
namespace slim::module::resolver {
namespace {
// free function replacing the lambda — looks up plugin name by module identity hash
// to avoid the shared context embedder data race when multiple synthetic modules are evaluated
v8::MaybeLocal<v8::Value> synthetic_module_evaluation_steps(v8::Local<v8::Context> context, v8::Local<v8::Module> v8_module) {
	auto isolate = context->GetIsolate();
	v8::TryCatch try_catch(isolate);
	int hash_id = v8_module->GetIdentityHash();
	auto it = slim::module::resolver::synthetic_module_plugin_names.find(hash_id);
	if(it == slim::module::resolver::synthetic_module_plugin_names.end()) {
		log::debug(log::Message(__func__, std::format("synthetic_module_evaluation_steps => no plugin name found for hash_id => {}", hash_id), __FILE__, __LINE__));
		return v8::MaybeLocal<v8::Value>(True(isolate));
	}
	std::string plugin_name_string = it->second;
	log::debug(log::Message(__func__, std::format("synthetic_module_evaluation_steps => plugin_name_string => {}", plugin_name_string), __FILE__, __LINE__));
	auto plugin_v8_object = utilities::GetObject(isolate, plugin_name_string, context->Global());
	auto default_export_result = v8_module->SetSyntheticModuleExport(isolate, utilities::StringToV8String(isolate, "default"), plugin_v8_object);
	if(default_export_result.IsNothing()) {
		log::debug(log::Message(__func__, std::format("SetSyntheticModuleExport returned Nothing for => default"), __FILE__, __LINE__));
	}
	if(try_catch.HasCaught()) {
		slim::exception_handler::v8_try_catch_handler(&try_catch);
	}
	return v8::MaybeLocal<v8::Value>(True(isolate));
}
} // namespace
} // namespace slim::module::resolver
v8::MaybeLocal<v8::Module> slim::module::resolver::module_call_back_resolver(v8::Local<v8::Context> context,
    v8::Local<v8::String> v8_specifier_string, v8::Local<v8::FixedArray> import_assertions, v8::Local<v8::Module> referrer) {
	log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
	auto isolate = context->GetIsolate();
	v8::TryCatch try_catch(isolate);
	std::string specifier_name_string = utilities::v8StringToString(isolate, v8_specifier_string);
	log::debug(log::Message(__func__, std::format("resolving specifier => {}", specifier_name_string), __FILE__, __LINE__));
	log::debug(log::Message(__func__, std::format("referrer hash_id => {}", referrer.IsEmpty() ? -1 : referrer->GetIdentityHash()), __FILE__, __LINE__));
	int current_module_hash_id = -1;
	try {
		if(plugins_set.contains(specifier_name_string)) {
			log::debug(log::Message(__func__, std::format("specifier is a plugin => {}", specifier_name_string), __FILE__, __LINE__));
			for(auto& [id, specifier] : cache) {
				if(specifier.specifier_uri() == specifier_name_string) {
					cache.erase(id);
					break;
				}
			}
			{
				slim::plugin::loader::load_plugin(isolate, specifier_name_string, true);
				if(try_catch.HasCaught()) {
					slim::exception_handler::v8_try_catch_handler(&try_catch);
				}
				// only declare "default" — plugins are always imported as default, named exports are not used
				v8::Local<v8::String> v8_default_string = utilities::StringToV8String(isolate, "default");
				std::vector<v8::Local<v8::String>> v8_string_exports_vector;
				v8_string_exports_vector.push_back(v8_default_string);
				v8::MemorySpan<const v8::Local<v8::String>> memory_span(v8_string_exports_vector.data(), v8_string_exports_vector.size());
				import_specifier module_specifier(isolate, specifier_name_string, v8::Module::CreateSyntheticModule(isolate,
									utilities::StringToV8String(isolate, specifier_name_string), memory_span, synthetic_module_evaluation_steps));
				current_module_hash_id = module_specifier.v8_module()->GetIdentityHash();
				// store plugin name keyed by hash so synthetic_module_evaluation_steps can look it up
				synthetic_module_plugin_names[current_module_hash_id] = specifier_name_string;
				log::debug(log::Message(__func__, std::format("synthetic module created with hash_id => {}", current_module_hash_id), __FILE__, __LINE__));
				cache_import_specifier(std::move(module_specifier));
				cache[current_module_hash_id].instantiate_module();
				if(cache[current_module_hash_id].v8_module()->GetStatus() == v8::Module::Status::kErrored) {
					isolate->ThrowException(cache[current_module_hash_id].v8_module()->GetException());
				}
				if(try_catch.HasCaught()) {
					slim::exception_handler::v8_try_catch_handler(&try_catch);
				}
			}
		}
		else {
			log::debug(log::Message(__func__, std::format("specifier is a file module => {}", specifier_name_string), __FILE__, __LINE__));
			import_specifier module_specifier(isolate, specifier_name_string, false, referrer);
			module_specifier.compile_module();
			if(module_specifier.v8_module()->GetStatus() == v8::Module::Status::kErrored) {
				isolate->ThrowException(module_specifier.v8_module()->GetException());
			}
			else {
				current_module_hash_id = module_specifier.v8_module()->GetIdentityHash();
				log::debug(log::Message(__func__, std::format("file module compiled with hash_id => {}", current_module_hash_id), __FILE__, __LINE__));
				cache_import_specifier(std::move(module_specifier));
				cache[current_module_hash_id].instantiate_module();
				if(cache[current_module_hash_id].v8_module()->GetStatus() == v8::Module::Status::kErrored) {
					isolate->ThrowException(cache[current_module_hash_id].v8_module()->GetException());
				}
			}
		}
	}
	catch(slim::common::SlimFileException& error) {
		std::string error_message = error.message + ", path => " + error.path;
		isolate->ThrowException(utilities::StringToV8String(isolate, "Module not found: " + specifier_name_string));
	}
	catch (...) {}
	if(cache.contains(current_module_hash_id)) {
		log::debug(log::Message(__func__, std::format("returning cached module for hash_id => {}", current_module_hash_id), __FILE__, __LINE__));
		log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
		return cache[current_module_hash_id].v8_module();
	}
	isolate->ThrowError(utilities::StringToV8String(isolate, "Module is empty: " + specifier_name_string));
	isolate->ThrowException(utilities::StringToV8String(isolate, "Module is empty: " + specifier_name_string));
	if(try_catch.HasCaught()) {
		slim::exception_handler::v8_try_catch_handler(&try_catch);
	}
	log::debug(log::Message(__func__, std::format("module was empty, returning null for specifier => {}", specifier_name_string), __FILE__, __LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
	return(v8::MaybeLocal<v8::Module>());
}
std::optional<std::reference_wrapper<slim::module::import_specifier>> slim::module::resolver::resolve_imports(v8::Isolate* isolate,
		std::string_view specifier_uri, bool is_entry_point = false) {
	log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
	log::debug(log::Message(__func__, std::format("specifier_uri => {}, is_entry_point => {}", specifier_uri, is_entry_point), __FILE__, __LINE__));
	import_specifier entry_script_specifier(isolate, specifier_uri, is_entry_point, v8::Local<v8::Module>());
	entry_script_specifier.compile_module();
	if(entry_script_specifier.v8_module()->GetStatus() == v8::Module::Status::kErrored) {
		isolate->ThrowException(entry_script_specifier.v8_module()->GetException());
		log::debug(log::Message(__func__, std::format("module compile errored for specifier_uri => {}", specifier_uri), __FILE__, __LINE__));
		log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
		return std::nullopt;
	}
	int hash_id = entry_script_specifier.v8_module()->GetIdentityHash();
	log::debug(log::Message(__func__, std::format("module compiled with hash_id => {}", hash_id), __FILE__, __LINE__));
	cache_import_specifier(std::move(entry_script_specifier));
	cache[hash_id].instantiate_module();
	log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
	return std::ref(cache[hash_id]);
}
void slim::module::resolver::cache_import_specifier(import_specifier module_import_specifier) {
	log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
	int hash_id = module_import_specifier.v8_module()->GetIdentityHash();
	log::debug(log::Message(__func__, std::format("caching import specifier with hash_id => {}", hash_id), __FILE__, __LINE__));
	cache[hash_id] = std::move(module_import_specifier);
	log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}
std::optional<std::reference_wrapper<slim::module::import_specifier>> slim::module::resolver::get_import_specifier_by_hash_id(int hash_id) {
	log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
	if(cache.contains(hash_id)) {
		log::debug(log::Message(__func__, std::format("found specifier for hash_id => {}", hash_id), __FILE__, __LINE__));
		log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
		return std::ref(cache[hash_id]);
	}
	log::debug(log::Message(__func__, std::format("no specifier found for hash_id => {}", hash_id), __FILE__, __LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
	return std::nullopt;
}
std::optional<std::reference_wrapper<slim::module::import_specifier>> slim::module::resolver::get_import_specifier_by_specifier_uri(std::string_view specifier_uri) {
	log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
	log::debug(log::Message(__func__, std::format("searching for specifier_uri => {}", specifier_uri), __FILE__, __LINE__));
	for(auto& [id, specifier] : cache) {
		if(specifier.specifier_uri() == specifier_uri) {
			log::debug(log::Message(__func__, std::format("found specifier for specifier_uri => {}", specifier_uri), __FILE__, __LINE__));
			log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
			return std::ref(specifier);
		}
	}
	log::debug(log::Message(__func__, std::format("no specifier found for specifier_uri => {}", specifier_uri), __FILE__, __LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
	return std::nullopt;
}
