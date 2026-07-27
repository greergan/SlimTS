#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <v8.h>
#include <libtsgo.h>
#include <tsgo.h>
namespace slim::module {
	class import_specifier {
	    public:
		import_specifier() = default;
		import_specifier(v8::Isolate* isolate, std::string_view specifier_string, v8::Local<v8::Module> synthetic_module);
		import_specifier(v8::Isolate* isolate, std::string_view specifier_string, bool is_entry_point, v8::Local<v8::Module> referrer);
		void compile_module();
		void instantiate_module();
		v8::Local<v8::Module>& v8_module();
		const std::string& specifier_uri() const;
		private:
			bool is_entry_point_ = false;
			bool is_synthetic_module_ = false;
			v8::Isolate* isolate_;
			v8::Local<v8::Context> context_;
			v8::Local<v8::Module> v8_module_;
			v8::Local<v8::Module> referrer_;
			std::string specifier_uri_;
			std::string compiled_specifier_string_url_;
			std::string specifier_protocol_;
			std::filesystem::path cache_directory_path_;
			std::filesystem::path specifier_path_;
			GoStr transpiled_source_code_;
			void resolve_module_path(std::string_view specifier);
			void specifier_uri(std::string_view s);
	};
}
