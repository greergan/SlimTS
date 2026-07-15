#include <filesystem>
#include <memory>
#include <regex>
#include <array>
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
	static std::set<std::string> plugins_set{"console","fs","http_server","kafka","os","path","process","memoryAdaptor","queue"};
	static specifier_cache_by_specifier by_specifier_cache;
	static specifier_cache_by_hash_id by_hash_id_cache;
}
v8::MaybeLocal<v8::Module> slim::module::resolver::module_call_back_resolver(v8::Local<v8::Context> _context, const v8::Local<v8::String> _v8_specifier_string,
		v8::Local<v8::FixedArray> _import_assertions, v8::Local<v8::Module> _referrer) {
	auto isolate = _context->GetIsolate();
	log::trace(log::Message("slim::module::resolver::module_call_back_resolver()","begins => " + utilities::v8StringToString(isolate, _v8_specifier_string), __FILE__, __LINE__));
	log::debug(log::Message("slim::module::resolver::module_call_back_resolver()","import_assertions->Length() => " + std::to_string(_import_assertions->Length()), __FILE__, __LINE__));
	v8::TryCatch try_catch(isolate);
	std::string specifier_name_string = utilities::v8StringToString(isolate, _v8_specifier_string);
	int current_module_hash_id = -1;
	try {
		if(plugins_set.contains(specifier_name_string)) { // have to reload module
			log::trace(log::Message("slim::module::resolver::module_call_back_resolver()","loading plugin from set => " + specifier_name_string,__FILE__, __LINE__));
			if(by_specifier_cache.contains(specifier_name_string)) {
				for(auto& [id, specifier] : by_hash_id_cache) {
					if(specifier->specifier_string() == specifier_name_string) {
						by_hash_id_cache.erase(id);
						by_specifier_cache.erase(specifier_name_string);
						break;
					}
				}
			}
			{
				auto create_SyntheticModuleEvaluationSteps = [](v8::Local<v8::Context> _context, v8::Local<v8::Module> _v8_module)-> v8::MaybeLocal<v8::Value> {
					auto isolate = _context->GetIsolate();
					v8::TryCatch try_catch(isolate);
					auto plugin_name_string = utilities::v8ValueToString(isolate, _context->GetEmbedderData(0));
					log::debug(log::Message("slim::module::resolver::module_call_back_resolver()","create_SyntheticModuleEvaluationSteps[]() => " + plugin_name_string,__FILE__, __LINE__));
					auto plugin_v8_object = utilities::GetObject(isolate, plugin_name_string, _context->Global());
					_v8_module->SetSyntheticModuleExport(isolate, utilities::StringToV8String(isolate, "default"), plugin_v8_object).ToChecked();
					auto property_names_array = plugin_v8_object->GetOwnPropertyNames(_context);
					if(!property_names_array.IsEmpty()) {
						auto property_names_array_local = property_names_array.ToLocalChecked();
						for(int array_index = 0; array_index < property_names_array_local->Length(); array_index++) {
							auto v8_property_name_string = property_names_array_local->Get(_context, array_index).ToLocalChecked()->ToString(_context);
							if(!v8_property_name_string.IsEmpty()) {
								auto v8_property_value = plugin_v8_object->Get(_context, v8_property_name_string.ToLocalChecked());
								if(!v8_property_value.IsEmpty()) {
									_v8_module->SetSyntheticModuleExport(isolate, v8_property_name_string.ToLocalChecked(), v8_property_value.ToLocalChecked()).ToChecked();
								}
							}
						}
					}
					if(try_catch.HasCaught()) {
						log::error(log::Message("slim::module::resolver::module_call_back_resolver()", "try_catch.HasCaught()",__FILE__, __LINE__));
						slim::exception_handler::v8_try_catch_handler(&try_catch);
					}
					return v8::MaybeLocal<v8::Value>(True(isolate));
				};
				log::debug(log::Message("slim::module::resolver::module_call_back_resolver()","loading plugin => " + specifier_name_string,__FILE__, __LINE__));
				slim::plugin::loader::load_plugin(isolate, specifier_name_string, true);
				log::debug(log::Message("slim::module::resolver::module_call_back_resolver()","loaded plugin => " + specifier_name_string,__FILE__, __LINE__));
				if(try_catch.HasCaught()) {
					auto message = slim::utilities::v8StringToString(isolate, try_catch.Message()->Get());
					log::error(log::Message("slim::module::resolver::module_call_back_resolver()", "try_catch.HasCaught() => " + message,__FILE__, __LINE__));
					slim::exception_handler::v8_try_catch_handler(&try_catch);
				}
				const v8::Local<v8::String> v8_default_string = utilities::StringToV8String(isolate, "default");
				std::vector<v8::Local<v8::String>> v8_string_exports_vector;
				v8_string_exports_vector.push_back(v8_default_string);
				utilities::V8KeysToVector(isolate, v8_string_exports_vector, utilities::GetObject(isolate, specifier_name_string, _context->Global()));
				_context->SetEmbedderData(0, _v8_specifier_string); //needed in create_SyntheticModuleEvaluationSteps
				const v8::MemorySpan<const v8::Local<v8::String>> memory_span(v8_string_exports_vector.data(), v8_string_exports_vector.size());
				log::debug(log::Message("slim::module::resolver::module_call_back_resolver()", "setting synthetic plugin", __FILE__, __LINE__));
				import_specifier module_specifier(isolate, specifier_name_string, v8::Module::CreateSyntheticModule(isolate,
									utilities::StringToV8String(isolate, specifier_name_string), memory_span, create_SyntheticModuleEvaluationSteps));
				cache_import_specifier(std::make_shared<import_specifier>(module_specifier));
				current_module_hash_id = module_specifier.v8_module()->GetIdentityHash();
				log::debug(log::Message("slim::module::resolver::module_call_back_resolver()", "done setting synthetic plugin", __FILE__, __LINE__));
				if(try_catch.HasCaught()) {
					log::error(log::Message("slim::module::resolver::module_call_back_resolver()", "try_catch.HasCaught()",__FILE__, __LINE__));
					slim::exception_handler::v8_try_catch_handler(&try_catch);
				}
			}
		}
		else {
			log::trace(log::Message("slim::module::resolver::module_call_back_resolver()","loading module from disk/url => " + specifier_name_string,__FILE__, __LINE__));
			if(_referrer.IsEmpty()) {
				log::debug(log::Message("slim::module::resolver::module_call_back_resolver()", "referrer not present",__FILE__, __LINE__));
			}
			else {
				auto referrer_import_specifier = get_import_specifier_by_hash_id(_referrer->GetIdentityHash());
				log::debug(log::Message("slim::module::resolver::module_call_back_resolver()", "referrer specifier path => " + referrer_import_specifier->specifier_path().string(),__FILE__, __LINE__));
			}
			import_specifier module_specifier(isolate, specifier_name_string, false, _referrer);
			module_specifier.compile_module(); // compile module so we can get at the hash id during future module imports
			if(module_specifier.v8_module()->GetStatus() == v8::Module::Status::kErrored) {
				isolate->ThrowException(module_specifier.v8_module()->GetException());
			}
			else {
				cache_import_specifier(std::make_shared<import_specifier>(module_specifier)); // now cache it before instantiate_module
				module_specifier.instantiate_module(); // instantiate_module causes import recursion where we need hash id to get at the parent path of current import
				if(module_specifier.v8_module()->GetStatus() == v8::Module::Status::kErrored) {
					isolate->ThrowException(module_specifier.v8_module()->GetException());
				}
				current_module_hash_id = module_specifier.v8_module()->GetIdentityHash();
				log::debug(log::Message("slim::module::resolver::module_call_back_resolver()","v8_module_status_string() => " + module_specifier.v8_module_status_string(),__FILE__, __LINE__));
			}
		}
	}
    catch(const slim::common::SlimFileException& _error) {
        std::string error_message = _error.message + ", path => " + _error.path;
		auto referrer_import_specifier = get_import_specifier_by_hash_id(_referrer->GetIdentityHash());
		log::error(log::Message("slim::module::resolver::module_call_back_resolver()", "referrer specifier path => " + referrer_import_specifier->specifier_path().string(),__FILE__, __LINE__));
        log::error(log::Message(_error.call, error_message,__FILE__, __LINE__));
		isolate->ThrowException(utilities::StringToV8String(isolate, "Module not found: " + specifier_name_string));
    }
	catch (...) {
		log::error(log::Message("slim::module::resolver::module_call_back_resolver()","caught unknown error => " + specifier_name_string, __FILE__, __LINE__));
	}
	if(by_hash_id_cache.contains(current_module_hash_id)) {
		log::trace(log::Message("slim::module::resolver::module_call_back_resolver()","returns by_hash_id_cache => " + by_hash_id_cache[current_module_hash_id]->specifier_url(), __FILE__, __LINE__));
		return by_hash_id_cache[current_module_hash_id]->v8_module();
	}

	isolate->ThrowError(utilities::StringToV8String(isolate, "Module is empty: " + specifier_name_string));
	isolate->ThrowException(utilities::StringToV8String(isolate, "Module is empty: " + specifier_name_string));
	if(try_catch.HasCaught()) {
		log::error(log::Message("slim::module::resolver::module_call_back_resolver()", "try_catch.HasCaught()",__FILE__, __LINE__));
		log::error(log::Message("slim::module::resolver::module_call_back_resolver()", utilities::v8ValueToString(isolate, try_catch.Exception()),__FILE__, __LINE__));
		slim::exception_handler::v8_try_catch_handler(&try_catch);
	}
	log::error(log::Message("slim::module::resolver::module_call_back_resolver()","ends empty module => " + specifier_name_string, __FILE__, __LINE__));
	return(v8::MaybeLocal<v8::Module>());
}
std::shared_ptr<slim::module::import_specifier> slim::module::resolver::resolve_imports(v8::Isolate* _isolate,
		slim::module::variant_specifier _script_name_string_or_specifier_stub, const bool _is_entry_point= false) {
	log::trace(log::Message("slim::module::resolver::resolve_imports()","begins", __FILE__, __LINE__));
	import_specifier entry_script_specifier(_isolate, _script_name_string_or_specifier_stub, _is_entry_point, v8::Local<v8::Module>());
	log::debug(log::Message("slim::module::resolver::resolve_imports()","begins => " + entry_script_specifier.specifier_string(), __FILE__, __LINE__));
	entry_script_specifier.compile_module();
	if(entry_script_specifier.v8_module()->GetStatus() == v8::Module::Status::kErrored) {
		log::error(log::Message("slim::module::resolver::resolve_imports()","error => " + entry_script_specifier.specifier_string(), __FILE__, __LINE__));
		_isolate->ThrowException(entry_script_specifier.v8_module()->GetException());
	}
	log::debug(log::Message("slim::module::resolver::resolve_imports()","compiled => " + entry_script_specifier.specifier_string(), __FILE__, __LINE__));
	cache_import_specifier(std::make_shared<import_specifier>(entry_script_specifier)); // now cache it before instantiate_module
	log::debug(log::Message("slim::module::resolver::resolve_imports()","cached => " + entry_script_specifier.specifier_string(), __FILE__, __LINE__));
	entry_script_specifier.instantiate_module(); // instantiate_module causes import recursion where we need hash id to get at the parent path of current import
	log::debug(log::Message("slim::module::resolver::resolve_imports()","instantiated => " + entry_script_specifier.specifier_string(), __FILE__, __LINE__));
	log::trace(log::Message("slim::module::resolver::resolve_imports()","ends => " + entry_script_specifier.specifier_url(), __FILE__, __LINE__));
	return by_specifier_cache[entry_script_specifier.specifier_string()];
}
void slim::module::resolver::cache_import_specifier(std::shared_ptr<import_specifier> _module_import_specifier) {
	log::trace(log::Message("slim::module::resolver::cache_import_specifier()", "begins => " + _module_import_specifier->specifier_string(), __FILE__, __LINE__));
	log::trace(log::Message("slim::module::resolver::cache_import_specifier()", "begins => " + std::to_string(_module_import_specifier->v8_module()->GetIdentityHash()), __FILE__, __LINE__));
	by_specifier_cache[_module_import_specifier->specifier_string()] = _module_import_specifier;
	by_hash_id_cache[_module_import_specifier->v8_module()->GetIdentityHash()] = _module_import_specifier;
	log::trace(log::Message("slim::module::resolver::cache_import_specifier()", "ends => " + _module_import_specifier->specifier_string(), __FILE__, __LINE__));
	log::trace(log::Message("slim::module::resolver::cache_import_specifier()", "ends => " + std::to_string(_module_import_specifier->v8_module()->GetIdentityHash()), __FILE__, __LINE__));
}
std::shared_ptr<slim::module::import_specifier> slim::module::resolver::get_import_specifier_by_hash_id(const int _hash_id) {
	log::trace(log::Message("slim::module::resolver::get_import_specifier_by_hash_id()", "begins => " + std::to_string(_hash_id), __FILE__, __LINE__));
	if(by_hash_id_cache.contains(_hash_id)) {
		log::debug(log::Message("slim::module::resolver::get_import_specifier_by_hash_id()","found specifier => " + std::to_string(_hash_id), __FILE__, __LINE__));
		log::trace(log::Message("slim::module::resolver::get_import_specifier_by_hash_id()","ends => " + std::to_string(_hash_id), __FILE__, __LINE__));
		return by_hash_id_cache[_hash_id];
	}
	log::trace(log::Message("slim::module::resolver::get_import_specifier_by_hash_id()","ends => " + std::to_string(_hash_id), __FILE__, __LINE__));
	return std::make_shared<slim::module::import_specifier>(slim::module::import_specifier());
}
std::shared_ptr<slim::module::import_specifier> slim::module::resolver::get_import_specifier_by_specifier_string(const std::string _specifier_string) {
	log::trace(log::Message("slim::module::resolver::get_import_specifier_by_specifier_string()","begins => " + _specifier_string, __FILE__, __LINE__));
	if(by_specifier_cache.contains(_specifier_string)) {
		log::debug(log::Message("slim::module::resolver::get_import_specifier_by_hash_id()","found specifier => " + _specifier_string, __FILE__, __LINE__));
		log::trace(log::Message("slim::module::resolver::get_import_specifier_by_specifier_string()", "ends => " + _specifier_string, __FILE__, __LINE__));
		return by_specifier_cache[_specifier_string];
	}
	log::trace(log::Message("slim::module::resolver::get_import_specifier_by_specifier_string()", "ends => " + _specifier_string, __FILE__, __LINE__));
	return std::make_shared<slim::module::import_specifier>(slim::module::import_specifier());
}