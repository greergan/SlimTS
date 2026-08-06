#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <vector>
#include <v8.h>
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/configuration_handler.h>
#include <slim/exception.h>
#include <slim/exception_handler.h>
#include <slim/file/watcher.h>
#include <slim/module/import_specifier.h>
#include <slim/module/resolver.h>
#include <slim/plugin/loader.h>
#include <slim/utilities.h>

namespace slim::module::resolver {
	using namespace slim;
	using namespace slim::common;
	static std::set<std::string> plugins_set{"console","fs","http","kafka","os","module","path","process","memoryAdaptor","queue","require"};
	static specifier_cache cache;
	// maps synthetic module identity hash to plugin name, used by synthetic_module_evaluation_steps
	static std::map<int, std::string> synthetic_module_plugin_names;
}
namespace slim::module::resolver {
namespace {
v8::MaybeLocal<v8::Value> synthetic_module_evaluation_steps(v8::Local<v8::Context> context, v8::Local<v8::Module> v8_module) {
#ifdef ENABLE_LOGGING
	log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
	auto isolate = context->GetIsolate();
	v8::TryCatch try_catch(isolate);
	int hash_id = v8_module->GetIdentityHash();
#ifdef ENABLE_LOGGING
	log::debug({__func__, std::format("hash_id => {}", hash_id), __FILE__, __LINE__});
#endif
	auto it = slim::module::resolver::synthetic_module_plugin_names.find(hash_id);
	if(it == slim::module::resolver::synthetic_module_plugin_names.end()) {
#ifdef ENABLE_LOGGING
		log::debug({__func__, std::format("no plugin name found for hash_id => {}", hash_id), __FILE__, __LINE__});
		log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
		return v8::MaybeLocal<v8::Value>(True(isolate));
	}
	std::string plugin_name_string = it->second;
#ifdef ENABLE_LOGGING
	log::debug({__func__, std::format("plugin_name_string => {}", plugin_name_string), __FILE__, __LINE__});
#endif
	auto plugin_v8_object = utilities::GetObject(isolate, plugin_name_string, context->Global());
	auto default_export_result = v8_module->SetSyntheticModuleExport(isolate, utilities::StringToV8String(isolate, "default"), plugin_v8_object);
	if(default_export_result.IsNothing()) {
#ifdef ENABLE_LOGGING
		log::debug({__func__, "SetSyntheticModuleExport returned Nothing for => default", __FILE__, __LINE__});
#endif
	}
	if(try_catch.HasCaught()) {
#ifdef ENABLE_LOGGING
		log::debug({__func__, "exception caught", __FILE__, __LINE__});
#endif
		slim::exception_handler::v8_try_catch_handler(&try_catch);
	}
#ifdef ENABLE_LOGGING
	log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
	return v8::MaybeLocal<v8::Value>(True(isolate));
}
} // namespace
} // namespace slim::module::resolver

v8::MaybeLocal<v8::Module> slim::module::resolver::module_call_back_resolver(v8::Local<v8::Context> context,
    v8::Local<v8::String> v8_specifier_string, v8::Local<v8::FixedArray> import_assertions, v8::Local<v8::Module> referrer) {
#ifdef ENABLE_LOGGING
	log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
	auto isolate = context->GetIsolate();
	v8::TryCatch try_catch(isolate);
	std::string specifier_name_string = utilities::v8StringToString(isolate, v8_specifier_string);
#ifdef ENABLE_LOGGING
	log::debug({__func__, std::format("resolving specifier => {}", specifier_name_string), __FILE__, __LINE__});
	log::debug({__func__, std::format("referrer hash_id => {}", referrer.IsEmpty() ? -1 : referrer->GetIdentityHash()), __FILE__, __LINE__});
#endif
	int current_module_hash_id = -1;
	// strip node: prefix for built-in aliases
	if(specifier_name_string.starts_with("node:")) {
		specifier_name_string = specifier_name_string.substr(5);
	}
	if(plugins_set.contains(specifier_name_string)) {
#ifdef ENABLE_LOGGING
		log::debug({__func__, std::format("specifier is a plugin => {}", specifier_name_string), __FILE__, __LINE__});
#endif
		for(auto& [id, specifier] : cache) {
			if(specifier.specifier_uri() == specifier_name_string) {
				cache.erase(id);
				break;
			}
		}
		slim::plugin::loader::load_plugin(isolate, specifier_name_string, true);
		if(try_catch.HasCaught()) {
			slim::exception_handler::v8_try_catch_handler(&try_catch);
		}
		v8::Local<v8::String> v8_default_string = utilities::StringToV8String(isolate, "default");
		std::vector<v8::Local<v8::String>> v8_string_exports_vector;
		v8_string_exports_vector.push_back(v8_default_string);
		v8::MemorySpan<const v8::Local<v8::String>> memory_span(v8_string_exports_vector.data(), v8_string_exports_vector.size());
		import_specifier module_specifier(isolate, specifier_name_string, v8::Module::CreateSyntheticModule(isolate,
							utilities::StringToV8String(isolate, specifier_name_string), memory_span, synthetic_module_evaluation_steps));
		auto& mod = module_specifier.v8_module();
		current_module_hash_id = mod->GetIdentityHash();
		synthetic_module_plugin_names[current_module_hash_id] = specifier_name_string;
#ifdef ENABLE_LOGGING
		log::debug({__func__, std::format("synthetic module created with hash_id => {}", current_module_hash_id), __FILE__, __LINE__});
#endif
		cache_import_specifier(std::move(module_specifier), current_module_hash_id);
		cache[current_module_hash_id].instantiate_module();
		auto& cached_mod = cache[current_module_hash_id].v8_module();
		if(cached_mod->GetStatus() == v8::Module::Status::kErrored) {
#ifdef ENABLE_LOGGING
			log::debug({__func__, std::format("synthetic module errored for hash_id => {}", current_module_hash_id), __FILE__, __LINE__});
#endif
			isolate->ThrowException(cached_mod->GetException());
		}
		if(try_catch.HasCaught()) {
			slim::exception_handler::v8_try_catch_handler(&try_catch);
		}
	}
	else {
#ifdef ENABLE_LOGGING
		log::debug({__func__, std::format("specifier is a file module => {}", specifier_name_string), __FILE__, __LINE__});
#endif
		import_specifier module_specifier(isolate, specifier_name_string, false, referrer);
		if(slim::configuration_handler::is_watching()) {
			std::string watch_path = module_specifier.specifier_uri();
			if(watch_path.starts_with("file://")) {
				watch_path = watch_path.substr(7);
			}
			slim::file::watcher::add(watch_path);
		}
		module_specifier.compile_module();
		auto& mod = module_specifier.v8_module();
		if(mod->GetStatus() == v8::Module::Status::kErrored) {
#ifdef ENABLE_LOGGING
			log::debug({__func__, std::format("file module compile errored for specifier => {}", specifier_name_string), __FILE__, __LINE__});
#endif
			isolate->ThrowException(mod->GetException());
		}
		else {
			current_module_hash_id = mod->GetIdentityHash();
#ifdef ENABLE_LOGGING
			log::debug({__func__, std::format("file module compiled with hash_id => {}", current_module_hash_id), __FILE__, __LINE__});
#endif
			cache_import_specifier(std::move(module_specifier), current_module_hash_id);
			cache[current_module_hash_id].instantiate_module();
			auto& cached_mod = cache[current_module_hash_id].v8_module();
			if(cached_mod->GetStatus() == v8::Module::Status::kErrored) {
#ifdef ENABLE_LOGGING
				log::debug({__func__, std::format("file module instantiate errored for hash_id => {}", current_module_hash_id), __FILE__, __LINE__});
#endif
				isolate->ThrowException(cached_mod->GetException());
			}
		}
	}
	if(cache.contains(current_module_hash_id)) {
#ifdef ENABLE_LOGGING
		log::debug({__func__, std::format("returning cached module for hash_id => {}", current_module_hash_id), __FILE__, __LINE__});
		log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
		auto& cached_mod = cache[current_module_hash_id].v8_module();
		return cached_mod;
	}
#ifdef ENABLE_LOGGING
	log::debug({__func__, std::format("module was empty, returning null for specifier => {}", specifier_name_string), __FILE__, __LINE__});
#endif
	isolate->ThrowException(utilities::StringToV8String(isolate, std::format("Module is empty: {}", specifier_name_string).c_str()));
	if(try_catch.HasCaught()) {
		slim::exception_handler::v8_try_catch_handler(&try_catch);
	}
#ifdef ENABLE_LOGGING
	log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
	return v8::MaybeLocal<v8::Module>();
}

v8::MaybeLocal<v8::Promise> slim::module::resolver::dynamic_import_callback(v8::Local<v8::Context> context,
        v8::Local<v8::Data> host_defined_options, v8::Local<v8::Value> resource_name, v8::Local<v8::String> specifier,
        v8::Local<v8::FixedArray> import_assertions) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    auto isolate = context->GetIsolate();
    v8::TryCatch try_catch(isolate);
    std::string specifier_string = utilities::v8StringToString(isolate, specifier);
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("specifier => {}", specifier_string), __FILE__, __LINE__});
#endif
    auto resolver_local = v8::Promise::Resolver::New(context).ToLocalChecked();
    auto promise = resolver_local->GetPromise();

    // resolve the module through the existing static import pipeline
    v8::MaybeLocal<v8::Module> maybe_module = module_call_back_resolver(
        context, specifier, import_assertions, v8::Local<v8::Module>());

    if(maybe_module.IsEmpty() || try_catch.HasCaught()) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("module resolution failed => {}", specifier_string), __FILE__, __LINE__});
#endif
        v8::Local<v8::Value> exception = try_catch.HasCaught()
            ? try_catch.Exception()
            : utilities::StringToV8String(isolate, std::format("dynamic import failed: {}", specifier_string).c_str()).As<v8::Value>();
        try_catch.Reset();
        resolver_local->Reject(context, exception).Check();
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
        return promise;
    }

    v8::Local<v8::Module> mod = maybe_module.ToLocalChecked();

    // evaluate if not already evaluated
    if(mod->GetStatus() == v8::Module::Status::kInstantiated) {
        auto eval_result = mod->Evaluate(context);
        if(eval_result.IsEmpty() || try_catch.HasCaught()) {
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("module evaluate failed => {}", specifier_string), __FILE__, __LINE__});
#endif
            v8::Local<v8::Value> exception = try_catch.HasCaught()
                ? try_catch.Exception()
                : utilities::StringToV8String(isolate, std::format("dynamic import evaluate failed: {}", specifier_string).c_str()).As<v8::Value>();
            try_catch.Reset();
            resolver_local->Reject(context, exception).Check();
#ifdef ENABLE_LOGGING
            log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
            return promise;
        }
    }

    if(mod->GetStatus() == v8::Module::Status::kErrored) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("module errored after evaluate => {}", specifier_string), __FILE__, __LINE__});
#endif
        resolver_local->Reject(context, mod->GetException()).Check();
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
        return promise;
    }

    // resolve with the module namespace object
    v8::Local<v8::Value> ns = mod->GetModuleNamespace();
    resolver_local->Resolve(context, ns).Check();
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("dynamic import resolved => {}", specifier_string), __FILE__, __LINE__});
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    return promise;
}

std::optional<std::reference_wrapper<slim::module::import_specifier>> slim::module::resolver::resolve_imports(v8::Isolate* isolate,
		std::string_view specifier_uri, bool is_entry_point = false) {
#ifdef ENABLE_LOGGING
	log::trace({__func__, "begins", __FILE__, __LINE__});
	log::debug({__func__, std::format("specifier_uri => {}, is_entry_point => {}", specifier_uri, is_entry_point), __FILE__, __LINE__});
#endif
	import_specifier entry_script_specifier(isolate, specifier_uri, is_entry_point, v8::Local<v8::Module>());
	entry_script_specifier.compile_module();
	auto& mod = entry_script_specifier.v8_module();
	if(mod->GetStatus() == v8::Module::Status::kErrored) {
#ifdef ENABLE_LOGGING
		log::debug({__func__, std::format("module compile errored for specifier_uri => {}", specifier_uri), __FILE__, __LINE__});
		log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
		isolate->ThrowException(mod->GetException());
		return std::nullopt;
	}
	if(slim::configuration_handler::is_watching()) {
		slim::file::watcher::add(specifier_uri);
	}
	int hash_id = mod->GetIdentityHash();
#ifdef ENABLE_LOGGING
	log::debug({__func__, std::format("module compiled with hash_id => {}", hash_id), __FILE__, __LINE__});
#endif
	cache_import_specifier(std::move(entry_script_specifier), hash_id);
	cache[hash_id].instantiate_module();
#ifdef ENABLE_LOGGING
	log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
	return std::ref(cache[hash_id]);
}

void slim::module::resolver::cache_import_specifier(import_specifier module_import_specifier, int hash_id) {
#ifdef ENABLE_LOGGING
	log::trace({__func__, "begins", __FILE__, __LINE__});
	log::debug({__func__, std::format("caching import specifier with hash_id => {}", hash_id), __FILE__, __LINE__});
#endif
	cache[hash_id] = std::move(module_import_specifier);
#ifdef ENABLE_LOGGING
	log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}

std::optional<std::reference_wrapper<slim::module::import_specifier>> slim::module::resolver::get_import_specifier_by_hash_id(int hash_id) {
#ifdef ENABLE_LOGGING
	log::trace({__func__, "begins", __FILE__, __LINE__});
	log::debug({__func__, std::format("hash_id => {}", hash_id), __FILE__, __LINE__});
#endif
	if(cache.contains(hash_id)) {
#ifdef ENABLE_LOGGING
		log::debug({__func__, std::format("found specifier for hash_id => {}", hash_id), __FILE__, __LINE__});
		log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
		return std::ref(cache[hash_id]);
	}
#ifdef ENABLE_LOGGING
	log::debug({__func__, std::format("no specifier found for hash_id => {}", hash_id), __FILE__, __LINE__});
	log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
	return std::nullopt;
}

std::optional<std::reference_wrapper<slim::module::import_specifier>> slim::module::resolver::get_import_specifier_by_specifier_uri(std::string_view specifier_uri) {
#ifdef ENABLE_LOGGING
	log::trace({__func__, "begins", __FILE__, __LINE__});
	log::debug({__func__, std::format("searching for specifier_uri => {}", specifier_uri), __FILE__, __LINE__});
#endif
	for(auto& [id, specifier] : cache) {
		if(specifier.specifier_uri() == specifier_uri) {
#ifdef ENABLE_LOGGING
			log::debug({__func__, std::format("found specifier for specifier_uri => {}", specifier_uri), __FILE__, __LINE__});
			log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
			return std::ref(specifier);
		}
	}
#ifdef ENABLE_LOGGING
	log::debug({__func__, std::format("no specifier found for specifier_uri => {}", specifier_uri), __FILE__, __LINE__});
	log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
	return std::nullopt;
}
