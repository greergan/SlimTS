#include <future>
#include <set>
#include <string>
#include <sstream>
#include <unordered_set>
#include <vector>
#include <v8.h>
#include <slim/common/fetch.h>
#include <slim/common/log.h>
#include <slim/common/memory_mapper.h>
#include <slim/common/utilities.h>
#include <slim/common/validators.h>
#include <slim/exception_handler.h>
#include <slim/macros.h>
#include <slim/module/import_specifier.h>
#include <slim/module/resolver.h>
#include <slim/queue/queue.h>
#include <slim/utilities.h>
namespace slim::module {
	using namespace slim;
	using namespace slim::common;
	std::set<std::string> file_extensions = {".ts", ".mjs", ".js"};
	std::vector<std::string> search_paths;
}
slim::module::specifier_stub::specifier_stub() {}
slim::module::specifier_stub::specifier_stub(const std::string _specifier_url, std::shared_ptr<std::string> _raw_source_code_ptr, const bool _is_service = false)
	: __url_string(_specifier_url), __raw_source_code_ptr(_raw_source_code_ptr), __is_service(_is_service) {}
const bool& slim::module::specifier_stub::is_service() const {
	log::trace(log::Message("slim::module::specifier_stub::is_service()","returns => " + slim::common::utilities::to_string(__is_service),__FILE__, __LINE__));
	return __is_service;
}
const std::shared_ptr<std::string> slim::module::specifier_stub::raw_source_code() const {
	log::trace(log::Message("slim::module::specifier_stub::raw_source_code()","returns => __raw_source_code_ptr",__FILE__, __LINE__));
	return __raw_source_code_ptr;
}
void slim::module::specifier_stub::raw_source_code(const std::shared_ptr<std::string> _ptr) {
	log::trace(log::Message("slim::module::specifier_stub::raw_source_code(_ptr)","sets => __raw_source_code_ptr",__FILE__, __LINE__));
	__raw_source_code_ptr = _ptr;
}
const std::string& slim::module::specifier_stub::specifier_string() const {
	log::trace(log::Message("slim::module::specifier_stub::specifier_string()","returns => " + __specifier_string,__FILE__, __LINE__));
	return __specifier_string;
}
void slim::module::specifier_stub::specifier_string(const std::string _string) {
	log::trace(log::Message("slim::module::specifier_stub::specifier_string(string)","begins => " + _string,__FILE__, __LINE__));
	__specifier_string = _string;
	log::trace(log::Message("slim::module::specifier_stub::specifier_string(string)","ends => " + specifier_string(),__FILE__, __LINE__));
}
const std::string& slim::module::specifier_stub::specifier_url() const {
	log::trace(log::Message("slim::module::specifier_stub::specifier_url()","returns => " + __url_string,__FILE__, __LINE__));
	return __url_string;
}
void slim::module::specifier_stub::specifier_url(const std::string _url_string) {
	log::trace(log::Message("slim::module::specifier_stub::specifier_url(_url_string)","sets => " + _url_string,__FILE__, __LINE__));
	__url_string = _url_string;
	log::trace(log::Message("slim::module::specifier_stub::specifier_url(_url_string)","ends => " + specifier_url(),__FILE__, __LINE__));
}
slim::module::import_specifier::import_specifier() {}
slim::module::import_specifier::import_specifier(v8::Isolate* _isolate, const std::string _specifier_string, v8::Local<v8::Module> _synthetic_module)
	: __isolate(_isolate), __v8_module(_synthetic_module) {
	log::trace(log::Message("slim::module::import_specifier::import_specifier()","begins => " + specifier_string(),__FILE__, __LINE__));
	__specifier_string = _specifier_string;
	context(isolate()->GetCurrentContext());
	instantiate_module();
	v8_module()->Evaluate(context()).FromMaybe(v8::Local<v8::Value>());
	log::trace(log::Message("slim::module::import_specifier::import_specifier()","ends => " + specifier_string(),__FILE__, __LINE__));
}
slim::module::import_specifier::import_specifier(v8::Isolate* _isolate,  variant_specifier _script_name_string_or_specifier_stub_ptr, const bool _is_entry_point, v8::Local<v8::Module> _referrer)
		: __isolate(_isolate), __referrer(_referrer), __is_entry_point(_is_entry_point) {
	log::trace(log::Message("slim::module::import_specifier::import_specifier()","begins => variant_specifier" ,__FILE__, __LINE__));
	const std::string use_cache_string = memory_mapper::read_string("slim_runtime_environmental_variables", "use_cache");
	if(use_cache_string == "true") {
		use_cached_source(true);
		__cache_directory();
	}
	std::string slim_library_path_string = memory_mapper::read_string("slim_runtime_environmental_variables", "slim_library_path");
	log::debug(log::Message("slim::module::import_specifier::import_specifier()","library path => " + slim_library_path_string,__FILE__,__LINE__));
	std::stringstream slim_library_path_strings_stream(slim_library_path_string);
	std::string library_directory_string;
	for(;std::getline(slim_library_path_strings_stream, library_directory_string, ':');) {
        log::debug(log::Message("slim::module::import_specifier::import_specifier()","library directory => " + library_directory_string,__FILE__,__LINE__));
		if(std::filesystem::exists(library_directory_string)) {
			if(std::find(search_paths.begin(), search_paths.end(), library_directory_string) == search_paths.end()) {
				search_paths.push_back(library_directory_string);
			}
		}
	}
	context(isolate()->GetCurrentContext());
	std::shared_ptr<slim::module::import_specifier> module_import_specifier_pointer;
	// files stored in compiled "library files" such as javascript servers i.e. typescript, less etc...
	if(std::holds_alternative<specifier_stub>(_script_name_string_or_specifier_stub_ptr)) {
		auto stub = std::get<specifier_stub>(_script_name_string_or_specifier_stub_ptr);
		log::trace(log::Message("slim::module::import_specifier::import_specifier()","preparing new specifier from stored file => " + stub.specifier_url(),__FILE__, __LINE__));
		specifier_string(stub.specifier_url());
		raw_source_code(stub.raw_source_code());
		compiled_source_code((macros::apply(raw_source_code(), specifier_string())));
	}
	else {
		specifier_string(std::get<std::string>(_script_name_string_or_specifier_stub_ptr));
		log::trace(log::Message("slim::module::import_specifier::import_specifier()","preparing new specifier from disk file => " + specifier_string(),__FILE__, __LINE__));
		resolve_module_path();
		log::debug(log::Message("slim::module::import_specifier::import_specifier()","resolved url path => " + specifier_url(),__FILE__, __LINE__));
		log::debug(log::Message("slim::module::import_specifier::import_specifier()","resolved specifier path => " + specifier_path().string(),__FILE__, __LINE__));
		fetch_source();
		memory_mapper::write("intermediate_source_code_storage", specifier_path().string(), intermediate_source_code());
		std::string queue_name_string("typescript");
		slim::queue::job* transpile_typescript_job = new slim::queue::job(queue_name_string, "intermediate_source_code_storage", specifier_path().string());
		transpile_typescript_job->egress_job_file.storage_container_handle = "compiled_source_code_storage";
		(void)std::async(std::launch::async, slim::queue::submit, transpile_typescript_job);
		if(transpile_typescript_job->errored) {
			log::debug(log::Message("slim::module::import_specifier::import_specifier()","job completed with errors",__FILE__, __LINE__));
			std::string errors;
			for(auto&& error : transpile_typescript_job->errors) {
				errors += error + "\n";
			}
			delete transpile_typescript_job;
			log::debug(log::Message("slim::module::import_specifier::import_specifier()",errors,__FILE__, __LINE__));
			isolate()->ThrowException(slim::utilities::StringToV8String(isolate(), errors));
		}
		else {
			slim::queue::file_storage& egress_job_file = transpile_typescript_job->egress_job_file;
			compiled_source_code(memory_mapper::read(egress_job_file.storage_container_handle, egress_job_file.file_name_string));
			log::debug(log::Message("slim::module::import_specifier::import_specifier()","storage handle => " + egress_job_file.storage_container_handle,__FILE__, __LINE__));
			log::debug(log::Message("slim::module::import_specifier::import_specifier()","compiled module => " + egress_job_file.file_name_string,__FILE__, __LINE__));
			log::debug(log::Message("slim::module::import_specifier::import_specifier()","file size => " + std::to_string(compiled_source_code().get()->length()),__FILE__, __LINE__));
			delete transpile_typescript_job;
		}
	}
	log::trace(log::Message("slim::module::import_specifier::import_specifier()","ends => " + specifier_url(),__FILE__, __LINE__));
}
const std::filesystem::path& slim::module::import_specifier::cache_directory() const {
	log::trace(log::Message("slim::module::import_specifier::cache_directory()","returns => " + __cache_directory_path.string(),__FILE__, __LINE__));
	return __cache_directory_path;
}
void slim::module::import_specifier::__cache_directory() {
	log::trace(log::Message("slim::module::import_specifier::__cache_directory()","begins",__FILE__, __LINE__));
	const auto cache_directory_string = memory_mapper::read_string("slim_runtime_environmental_variables", "cache_directory");
	if(cache_directory_string.length() > 0 && std::filesystem::exists(cache_directory_string)) {
		log::debug(log::Message("slim::module::import_specifier::__cache_directory()","setting cache_directory => " + cache_directory_string,__FILE__, __LINE__));
		__cache_directory_path = std::filesystem::canonical(cache_directory_string);
		log::debug(log::Message("slim::module::import_specifier::__cache_directory()","set cache_directory => " + cache_directory().string(),__FILE__, __LINE__));
	}
	else {
		const std::string error_string = "slim::module::import_specifier::__cache_directory()|cache_directory => " + cache_directory_string + " <= does not exist";
		throw error_string;
	}
	log::trace(log::Message("slim::module::import_specifier::__cache_directory()","ends => " + cache_directory().string(),__FILE__, __LINE__));
}
void slim::module::import_specifier::compiled_source_code(std::shared_ptr<std::string> _ptr) {
	log::trace(log::Message("slim::module::import_specifier::compiled_source_code(_ptr)","sets => __compiled_source_code_ptr",__FILE__, __LINE__));
	__compiled_source_code_ptr = _ptr;
}
const std::shared_ptr<std::string> slim::module::import_specifier::compiled_source_code() const {
	log::trace(log::Message("slim::module::import_specifier::compiled_source_code()","returns => __compiled_source_code_ptr",__FILE__, __LINE__));
	return __compiled_source_code_ptr;
}
v8::Local<v8::Context> slim::module::import_specifier::context() const {
	log::trace(log::Message("slim::module::import_specifier::context()","returns => __context",__FILE__, __LINE__));
	return __context;
}
void slim::module::import_specifier::context(v8::Local<v8::Context> _context) {
	log::trace(log::Message("slim::module::import_specifier::context(_context)","sets => __context",__FILE__, __LINE__));
	__context = _context;
}
void slim::module::import_specifier::compile_module() {
	log::trace(log::Message("slim::module::import_specifier::compile_module()","begins => " + specifier_url(),__FILE__, __LINE__));
	log::trace(log::Message("slim::module::import_specifier::compile_module()","specifier path => " + specifier_path().string(),__FILE__, __LINE__));
	v8::TryCatch try_catch(isolate());
	v8::ScriptOrigin origin(slim::utilities::StringToV8Value(isolate(), specifier_path().string()), 0, 0, false, -1, slim::utilities::StringToV8Value(isolate(), ""), false, false, true);
	v8::ScriptCompiler::Source v8_module_source(slim::utilities::StringToV8String(isolate(), *compiled_source_code()), origin);
	v8::ScriptCompiler::CompileOptions module_compile_options(v8::ScriptCompiler::kProduceCompileHints);
	v8::ScriptCompiler::NoCacheReason module_no_cache_reason(v8::ScriptCompiler::kNoCacheNoReason);
	v8::MaybeLocal<v8::Module> temporary_module = v8::ScriptCompiler::CompileModule(isolate(), &v8_module_source, module_compile_options, module_no_cache_reason);
	if(!temporary_module.IsEmpty()) {
		v8_module(temporary_module.ToLocalChecked());
		log::debug(log::Message("slim::module::import_specifier::compile_module()", "compiled module hash id => " + std::to_string(v8_module()->GetIdentityHash()), __FILE__, __LINE__));
	}
	if(try_catch.HasCaught()) {
		log::error(log::Message("slim::module::import_specifier::compile_module()", "try_catch.HasCaught()",__FILE__, __LINE__));
		slim::exception_handler::v8_try_catch_handler(&try_catch);
	}
	log::trace(log::Message("slim::module::import_specifier::compile_module()","ends => " + specifier_url(),__FILE__, __LINE__));
}
void slim::module::import_specifier::fetch_source() {
	log::trace(log::Message("slim::module::import_specifier::fetch_source()","begins => " + specifier_url(),__FILE__, __LINE__));
	bool source_is_cached = false;
	if(use_cached_source()) {
		log::debug(log::Message("slim::module::import_specifier::fetch_source()", "looking for cached source => " + specifier_url(), __FILE__, __LINE__));
		//const auto cache_directory_path = get_cache_directory();
		log::debug(log::Message("slim::module::import_specifier::fetch_source()", "found cache directory => " + cache_directory().string(), __FILE__, __LINE__));
		const std::string cached_file_name ="";
	}
	else {
		raw_source_code(std::make_shared<std::string>(slim::common::fetch::stream(specifier_url())->str()));
		if(raw_source_code()) {
			intermediate_source_code(slim::macros::apply(raw_source_code(), specifier_url()));
			if(intermediate_source_code()) {
				log::debug(log::Message("slim::module::import_specifier::fetch_source()", specifier_url() + " size => " + std::to_string(intermediate_source_code().get()->size()), __FILE__, __LINE__));
			}
			else {
				log::debug(log::Message("slim::module::import_specifier::fetch_source()", specifier_url() + " intermediate_source_code() => nullptr", __FILE__, __LINE__));
			}
		}
		else {
			log::debug(log::Message("slim::module::import_specifier::fetch_source()", specifier_url() + " raw_source_code() => nullptr", __FILE__, __LINE__));
		}
	}
	log::trace(log::Message("slim::module::import_specifier::fetch_source()","ends => " + specifier_url(),__FILE__, __LINE__));
}
const bool slim::module::import_specifier::has_v8_module() const {
	log::trace(log::Message("slim::module::import_specifier::has_v8_module()","returns => " + slim::common::utilities::to_string(__v8_module.IsEmpty()),__FILE__, __LINE__));
	return __v8_module.IsEmpty();
}
void slim::module::import_specifier::instantiate_module() {
	log::trace(log::Message("slim::module::import_specifier::instantiate_module()","begins => " + specifier_url(),__FILE__, __LINE__));
	v8::TryCatch try_catch(isolate());
	auto result = v8_module()->InstantiateModule(context(), slim::module::resolver::module_call_back_resolver);
	if(result.IsNothing()) {
		log::error(log::Message("slim::module::import_specifier::instantiate_module()","InstantiateModule produced nothing for => " + specifier_url(), __FILE__, __LINE__));
	}
	if(try_catch.HasCaught()) {
		log::error(log::Message("slim::module::import_specifier::instantiate_module()", "try_catch.HasCaught()",__FILE__, __LINE__));
		slim::exception_handler::v8_try_catch_handler(&try_catch);
	}
	log::debug(log::Message("slim::module::import_specifier::instantiate_module()","InstantiateModule status => " + v8_module_status_string(),__FILE__, __LINE__));
	log::trace(log::Message("slim::module::import_specifier::instantiate_module()","ends => " + specifier_url(),__FILE__, __LINE__));
}
const bool slim::module::import_specifier::is_entry_point() const {
	log::trace(log::Message("slim::module::import_specifier::is_entry_point()","returns => " + slim::common::utilities::to_string(__is_entry_point),__FILE__, __LINE__));
	return __is_entry_point;
}
void slim::module::import_specifier::isolate(v8::Isolate* _isolate) {
	log::trace(log::Message("slim::module::import_specifier::isolate(_isolate)","setting => __isolate",__FILE__, __LINE__));
	__isolate = _isolate;
}
v8::Isolate* slim::module::import_specifier::isolate() const {
	log::trace(log::Message("slim::module::import_specifier::isolate()","returns => __isolate",__FILE__, __LINE__));
	return __isolate;
}
void slim::module::import_specifier::intermediate_source_code(std::shared_ptr<std::string> _ptr) {
	log::trace(log::Message("slim::module::import_specifier::intermediate_source_code(_ptr)","sets => __intermediate_source_code_ptr",__FILE__, __LINE__));
	__intermediate_source_code_ptr = _ptr;
}
const std::shared_ptr<std::string> slim::module::import_specifier::intermediate_source_code() const {
	log::trace(log::Message("slim::module::import_specifier::intermediate_source_code()","returns => __intermediate_source_code_ptr",__FILE__, __LINE__));
	return __intermediate_source_code_ptr;
}
const v8::Local<v8::Module>& slim::module::import_specifier::referrer() const {
	log::trace(log::Message("slim::module::import_specifier::referrer()","returns => __referrer",__FILE__, __LINE__));
	return __referrer;
}
void slim::module::import_specifier::resolve_module_path() {
	log::trace(log::Message("slim::module::import_specifier::resolve_module_path()","begins => " + specifier_string(), __FILE__, __LINE__));
	v8::TryCatch try_catch(isolate());
	bool module_file_found = false;
	if(!specifier_string().starts_with("../") && !specifier_string().starts_with("./") && !specifier_string().starts_with("/")) {
		log::debug(log::Message("slim::module::import_specifier::resolve_module_path()","specifier_string is relative => " + specifier_string(), __FILE__, __LINE__));
		log::debug(log::Message("slim::module::import_specifier::resolve_module_path()","search_paths contains => " + std::to_string(search_paths.size()) + " entries", __FILE__, __LINE__));
		log::info("todo ../ case");
		for(auto& current_search_path : search_paths) {
			log::debug(log::Message("slim::module::import_specifier::resolve_module_path()","current_search_path => " + current_search_path,__FILE__, __LINE__));
			auto current_working_search_path = std::filesystem::absolute(current_search_path + std::filesystem::path::preferred_separator + specifier_string());
			log::debug(log::Message("slim::module::import_specifier::resolve_module_path()","absolute path => " + current_working_search_path.string(),__FILE__, __LINE__));
			if(current_working_search_path.has_extension()) {
				if(std::filesystem::exists(current_working_search_path)) {
					log::debug(log::Message("slim::module::import_specifier::resolve_module_path()", "current_working_search_path exists() => " + current_working_search_path.string(),__FILE__, __LINE__));
					module_file_found = true;
					specifier_path(std::filesystem::canonical(current_working_search_path));
					specifier_url(specifier_path().string());
					break;
				}
			}
			else {
				log::debug(log::Message("slim::module::import_specifier::resolve_module_path()", "searching for => " + current_working_search_path.string(),__FILE__, __LINE__));
				std::unordered_set<std::string> possible_module_names = {
					current_working_search_path.string(),
					current_working_search_path.string() + "/index"
				};
				for(auto& possible_module_name : possible_module_names) {
					log::debug(log::Message("slim::module::import_specifier::resolve_module_path()", "searching possible module file => " + possible_module_name,__FILE__, __LINE__));
					for(auto& file_extension : file_extensions) {
						std::filesystem::path possible_module_file_path = possible_module_name + file_extension;
						log::debug(log::Message("slim::module::import_specifier::resolve_module_path()", "searching for possible module file on disk => " + possible_module_file_path.string(),__FILE__, __LINE__));
						if(std::filesystem::exists(possible_module_file_path)) {
							log::debug(log::Message("slim::module::import_specifier::resolve_module_path()", "found module file on disk => " + possible_module_file_path.string(),__FILE__, __LINE__));
							module_file_found = true;
							specifier_path(std::filesystem::canonical(possible_module_file_path));
							specifier_url(specifier_path().string());
							log::debug(log::Message("slim::module::import_specifier::resolve_module_path()", "specifier path set => " + specifier_path().string(),__FILE__, __LINE__));
							log::debug(log::Message("slim::module::import_specifier::resolve_module_path()", "specifier string url => " + specifier_url(),__FILE__, __LINE__));
							break;
						}
						else {
							log::debug(log::Message("slim::module::import_specifier::resolve_module_path()", "did not find possible module file on disk => " + possible_module_file_path.string(),__FILE__, __LINE__));
						}
					}
					if(module_file_found) {
						break;
					}
				}
			}
			if(module_file_found) {
				break;
			}
		}
	}
	else {
		log::debug(log::Message("slim::module::import_specifier::import_specifier()","specifier_string => " + specifier_string(), __FILE__, __LINE__));
		log::trace(log::Message("slim::module::import_specifier::import_specifier()","referrer.IsEmpty() => " + slim::common::utilities::to_string(referrer().IsEmpty()),__FILE__, __LINE__));
		const auto specifier_path_uri = std::filesystem::path(specifier_string());
		log::debug(log::Message("slim::module::import_specifier::import_specifier()","specifier_path_uri => " + specifier_path_uri.string(),__FILE__, __LINE__));
		if(referrer().IsEmpty()) {
			const auto temporary_specifier_path = std::filesystem::absolute(specifier_path_uri);
			if(std::filesystem::exists(temporary_specifier_path)) {
				specifier_path(std::filesystem::canonical(temporary_specifier_path));
				specifier_url(specifier_path().string());
			}
			log::debug(log::Message("slim::module::import_specifier::import_specifier()","absolute specifier_path => " +  specifier_path().string(),__FILE__, __LINE__));
		}
		else {
			const auto referrer_import_specifier = slim::module::resolver::get_import_specifier_by_hash_id(referrer()->GetIdentityHash());
			log::debug(log::Message("slim::module::import_specifier::import_specifier()","referrer_import_specifier->specifier_url() => " + referrer_import_specifier->specifier_url(),__FILE__, __LINE__));
			log::debug(log::Message("slim::module::import_specifier::import_specifier()","referrer_import_specifier->specifier_string() => " + referrer_import_specifier->specifier_string(),__FILE__, __LINE__));
			std::string temporary_specifier_string = specifier_path_uri.string();
			if(specifier_path_uri.string().starts_with("../")) {
				temporary_specifier_string = "/" + specifier_path_uri.string();
			}
			else if(specifier_path_uri.string().starts_with("./")) {
				temporary_specifier_string = specifier_path_uri.string().substr(1);
			}
			log::debug(log::Message("slim::module::import_specifier::import_specifier()","referrer_import_specifier->specifier_path() => " +  referrer_import_specifier->specifier_path().string(),__FILE__, __LINE__));
			const auto new_specifier_path = std::filesystem::path(referrer_import_specifier->specifier_path().parent_path().string() + temporary_specifier_string);
			specifier_path(new_specifier_path);
			specifier_url(specifier_path().string());
			log::debug(log::Message("slim::module::import_specifier::import_specifier()","specifier_path formed from parent path  => " +  specifier_path().string(),__FILE__, __LINE__));
		}
		log::debug(log::Message("slim::module::import_specifier::import_specifier()","specifier_path => " +  specifier_path().string(),__FILE__, __LINE__));
		if(specifier_path().has_extension()) { 
			module_file_found = true;
			specifier_url(specifier_path().string());
		}
		else {
			std::unordered_set<std::string> possible_module_names = {
				specifier_path().string(),
				specifier_path().string() + "/index"
			};
			for(auto& file_extension : file_extensions) {
				for(auto& possible_module_name : possible_module_names) {
					std::string module_file_path = possible_module_name + file_extension;
					if(std::filesystem::exists(module_file_path)) {
						module_file_found = true;
						specifier_path(std::filesystem::canonical(module_file_path));
						specifier_url(specifier_path().string());
						break;
					}
				}
			}
		}
	}
	if(!module_file_found) {
		log::error(log::Message("slim::module::import_specifier::import_specifier()","module not found => " + specifier_string(),__FILE__, __LINE__));
		isolate()->ThrowException(slim::utilities::StringToV8String(isolate(), "module not found => " + specifier_string()));
	}
	log::trace(log::Message("slim::module::import_specifier::resolve_module_path()","ends => " + specifier_url(), __FILE__, __LINE__));
}
const std::filesystem::path& slim::module::import_specifier::specifier_path() const {
	log::trace(log::Message("slim::module::import_specifier::specifier_path()","returns => " + __specifier_path.string(),__FILE__, __LINE__));
	return __specifier_path;
}
void slim::module::import_specifier::specifier_path(const std::filesystem::path _path) {
	log::trace(log::Message("slim::module::import_specifier::specifier_path(path)","begins => " + _path.string(),__FILE__, __LINE__));
	__specifier_path = _path;
	log::trace(log::Message("slim::module::import_specifier::specifier_path(path)","ends => " + specifier_path().string(),__FILE__, __LINE__));
}
const std::string& slim::module::import_specifier::specifier_url() const {
	log::trace(log::Message("slim::module::import_specifier::specifier_url()","returns => " + __url_string,__FILE__, __LINE__));
	return __url_string;
}
void slim::module::import_specifier::specifier_url(const std::string _string) {
	log::trace(log::Message("slim::module::import_specifier::specifier_url(string)","begins => " + _string,__FILE__, __LINE__));
	if(_string.length() == 0) {
		const std::string error_message = "slim::module::import_specifier::specifier_url(string) requires a non-zero length string";
		throw error_message;
	}
	if(!slim::common::validators::is_url(_string)) {
		log::debug(log::Message("slim::module::import_specifier::specifier_url(string)","creating specifier_url => " + _string,__FILE__, __LINE__));
		__url_string = "file://" + _string;
		log::debug(log::Message("slim::module::import_specifier::specifier_url(string)","created specifier_url => " + specifier_url(),__FILE__, __LINE__));
	}
	else {
		log::debug(log::Message("slim::module::import_specifier::specifier_url(string)","already a url => " + _string,__FILE__, __LINE__));
		__url_string = _string;
	}
	log::trace(log::Message("slim::module::import_specifier::specifier_url(string)","ends => " + specifier_url(),__FILE__, __LINE__));
}
const bool slim::module::import_specifier::use_cached_source() const {
	log::trace(log::Message("slim::module::import_specifier::use_cached_source()","returns => " + slim::common::utilities::to_string(__use_cached_source),__FILE__, __LINE__));
	return __use_cached_source;
}
void slim::module::import_specifier::use_cached_source(const bool _use_cache) {
	log::trace(log::Message("slim::module::import_specifier::use_cached_source(bool)","begins => " + slim::common::utilities::to_string(_use_cache),__FILE__, __LINE__));
	__use_cached_source = _use_cache;
	log::trace(log::Message("slim::module::import_specifier::use_cached_source(bool)","ends => " + slim::common::utilities::to_string(use_cached_source()),__FILE__, __LINE__));
}
v8::Local<v8::Module>& slim::module::import_specifier::v8_module() {
	return __v8_module;
}
void slim::module::import_specifier::v8_module(v8::Local<v8::Module> _v8_module) {
	log::trace(log::Message("slim::module::import_specifier::v8_module()","begins",__FILE__, __LINE__));
	__v8_module = _v8_module;
}
const std::string slim::module::import_specifier::v8_module_status_string() {
	std::string __v8_module_status;
	switch(__v8_module->GetStatus()) {
		case v8::Module::Status::kUninstantiated: __v8_module_status = "v8::Module::Status::kUninstantiated"; break;
		case v8::Module::Status::kInstantiating: __v8_module_status = "v8::Module::Status::kInstantiating"; break;
		case v8::Module::Status::kInstantiated: __v8_module_status = "v8::Module::Status::kInstantiated"; break;
		case v8::Module::Status::kEvaluating: __v8_module_status = "v8::Module::Status::kEvaluating"; break;
		case v8::Module::Status::kErrored: __v8_module_status = "v8::Module::Status::kErrored"; break;
	}
	log::trace(log::Message("slim::module::import_specifier::module_status_string()","returns => " + __v8_module_status,__FILE__, __LINE__));
	return __v8_module_status;
}

