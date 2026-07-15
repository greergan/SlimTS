#ifndef __SLIM__MODULE__IMPORT__SPECIFIER__H
#define __SLIM__MODULE__IMPORT__SPECIFIER__H
#include <filesystem>
#include <string>
#include <variant>
#include <v8.h>
namespace slim::module {
	struct specifier_stub {
		specifier_stub();
		specifier_stub(const std::string _specifier_url, std::shared_ptr<std::string> _raw_source_code_ptr, const bool _is_service);
		const bool& is_service() const;
		const std::string& specifier_string() const;
		const std::string& specifier_url() const;
		const std::shared_ptr<std::string> raw_source_code() const;
		protected:
			bool __is_service;
			std::string __specifier_string;
			std::string __url_string;
			std::shared_ptr<std::string> __raw_source_code_ptr;
			void specifier_string(const std::string _string);
			void specifier_url(const std::string _string);
			void raw_source_code(const std::shared_ptr<std::string> _ptr);
	};
	using variant_specifier = std::variant<std::string, specifier_stub>;
	struct import_specifier: public specifier_stub {
		import_specifier();
		import_specifier(v8::Isolate* _isolate, const std::string _specifier_string, v8::Local<v8::Module> _synthetic_module);
		import_specifier(v8::Isolate* _isolate, variant_specifier _script_name_string_or_specifier_stub, const bool _is_entry_point, v8::Local<v8::Module> _referrer);
		void compile_module();
		void instantiate_module();
		v8::Local<v8::Context> context() const;
		v8::Local<v8::Module>& v8_module();
		v8::Isolate* isolate() const;
		void isolate(v8::Isolate* _isolate);
		const std::string v8_module_status_string();
		const std::string& specifier_url() const;
		const std::filesystem::path& specifier_path() const;
		const std::filesystem::path& cache_directory() const;
		const std::shared_ptr<std::string> compiled_source_code() const;
		const std::shared_ptr<std::string> intermediate_source_code() const;
		const v8::Local<v8::Module>& referrer() const;
		const bool has_v8_module() const;
		const bool is_entry_point() const;
		const bool use_cached_source() const;
		private:
			bool __is_entry_point = false;
			bool __is_synthetic_module = false;
			bool __use_cached_source = false;
			v8::Isolate* __isolate;
			v8::Local<v8::Context> __context;
			v8::Local<v8::Module> __v8_module;
			v8::Local<v8::Module> __referrer;
			std::string __compiled_specifier_string_url;
			std::string __specifier_protocol;
			std::shared_ptr<std::string> __compiled_source_code_ptr;
			std::shared_ptr<std::string> __intermediate_source_code_ptr;
			std::filesystem::path __cache_directory_path;
			std::filesystem::path __specifier_path;
			void __cache_directory();
			void compiled_source_code(std::shared_ptr<std::string> _ptr);
			void context(v8::Local<v8::Context> _context);
			void fetch_source();		
			void intermediate_source_code(std::shared_ptr<std::string> _ptr);
			void resolve_module_path();
			void specifier_path(const std::filesystem::path _path);
			void specifier_url(const std::string _string);
			void use_cached_source(const bool _boolean);
			void v8_module(v8::Local<v8::Module> _v8_module);
	};
}
#endif