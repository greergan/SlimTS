#ifndef __SLIM__MODULE__RESOLVER__H
#define __SLIM__MODULE__RESOLVER__H
#include <memory>
#include <string>
#include <unordered_map>
#include <v8.h>
#include <slim/module/import_specifier.h>
namespace slim::module::resolver {
	using specifier_cache_by_specifier = std::unordered_map<std::string, std::shared_ptr<import_specifier>>;
	using specifier_cache_by_hash_id = std::unordered_map<int, std::shared_ptr<import_specifier>>;
	std::shared_ptr<slim::module::import_specifier> resolve_imports(v8::Isolate* _isolate, variant_specifier _script_name_string_or_specifier_stub, const bool _is_entry_point);
	v8::MaybeLocal<v8::Module> module_call_back_resolver(v8::Local<v8::Context> _context,
		const v8::Local<v8::String> _v8_specifier_string, v8::Local<v8::FixedArray> _import_assertions, v8::Local<v8::Module> _referrer);
	static void cache_import_specifier(std::shared_ptr<import_specifier> _module_import_specifier);
	std::shared_ptr<slim::module::import_specifier> get_import_specifier_by_hash_id(const int _hash_id_int);
	std::shared_ptr<slim::module::import_specifier> get_import_specifier_by_specifier_string(const std::string _specificer_string);
}
#endif