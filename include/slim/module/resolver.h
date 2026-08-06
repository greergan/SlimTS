#pragma once
#include <optional>
#include <string_view>
#include <unordered_map>
#include <v8.h>
#include <slim/module/import_specifier.h>
namespace slim::module::resolver {
	using specifier_cache = std::unordered_map<int, import_specifier>;
	std::optional<std::reference_wrapper<import_specifier>> resolve_imports(v8::Isolate* isolate, std::string_view specifier_uri,
	    bool is_entry_point);
	v8::MaybeLocal<v8::Module> module_call_back_resolver(v8::Local<v8::Context> context, v8::Local<v8::String> v8_specifier_string,
	    v8::Local<v8::FixedArray> import_assertions, v8::Local<v8::Module> referrer);
	void cache_import_specifier(import_specifier module_import_specifier, int hash_id);
	std::optional<std::reference_wrapper<import_specifier>> get_import_specifier_by_hash_id(int hash_id);
	std::optional<std::reference_wrapper<import_specifier>> get_import_specifier_by_specifier_uri(std::string_view specifier_uri);
}
