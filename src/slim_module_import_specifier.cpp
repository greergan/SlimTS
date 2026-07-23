#include <filesystem>
#include <set>
#include <string>
#include <sstream>
#include <unordered_set>
#include <vector>
#include <v8.h>
#include <slim/common/log.h>
#include <slim/common/memory/mapper.h>
#include <slim/common/utilities.h>
#include <slim/exception_handler.h>
#include <slim/module/import_specifier.h>
#include <slim/module/resolver.h>
#include <slim/utilities.h>

namespace slim::module {
    using namespace slim;
    using namespace slim::common;
    std::set<std::string> file_extensions = {".ts", ".mjs", ".js"};
    std::vector<std::string> search_paths;
}

slim::module::import_specifier::import_specifier(v8::Isolate* isolate, std::string_view specifier_string, v8::Local<v8::Module> synthetic_module)
    : isolate_(isolate), v8_module_(synthetic_module), context_(isolate->GetCurrentContext()) {
    specifier_uri_ = specifier_string;
    instantiate_module();
}

slim::module::import_specifier::import_specifier(v8::Isolate* isolate, std::string_view specifier_string, bool is_entry_point, v8::Local<v8::Module> referrer)
        : isolate_(isolate), referrer_(referrer), is_entry_point_(is_entry_point), context_(isolate->GetCurrentContext()) {
    std::string slim_library_path_string = memory_mapper::read_string("slim_runtime_environmental_variables", "slim_library_path");
    std::stringstream slim_library_path_strings_stream(slim_library_path_string);
    std::string library_directory_string;
    for(;std::getline(slim_library_path_strings_stream, library_directory_string, ':');) {
        if(std::filesystem::exists(library_directory_string)) {
            if(std::find(search_paths.begin(), search_paths.end(), library_directory_string) == search_paths.end()) {
                search_paths.push_back(library_directory_string);
            }
        }
    }
    log::debug(log::Message(__func__, "specifier_string => " + std::string(specifier_string), __FILE__, __LINE__));
    log::debug(log::Message(__func__, "is_entry_point => " + std::string(is_entry_point ? "true" : "false"), __FILE__, __LINE__));
    log::debug(log::Message(__func__, "referrer_.IsEmpty => " + std::string(referrer_.IsEmpty() ? "true" : "false"), __FILE__, __LINE__));
    log::debug(log::Message(__func__, "slim_library_path => " + slim_library_path_string, __FILE__, __LINE__));
    if(specifier_string.starts_with("http://") || specifier_string.starts_with("https://")) {
        log::debug(log::Message(__func__, "taking http/https path", __FILE__, __LINE__));
        specifier_uri(specifier_string);
    }
    else if((specifier_string.starts_with("./") || specifier_string.starts_with("../")) && !referrer_.IsEmpty()) {
        log::debug(log::Message(__func__, "taking relative path with referrer", __FILE__, __LINE__));
        auto referrer_import_specifier = slim::module::resolver::get_import_specifier_by_hash_id(referrer_->GetIdentityHash());
        if(!referrer_import_specifier.has_value()) {
            isolate_->ThrowException(slim::utilities::StringToV8String(isolate_, "referrer not found in cache"));
            return;
        }
        import_specifier& ref = referrer_import_specifier.value().get();
        log::debug(log::Message(__func__, "referrer specifier_uri => " + ref.specifier_uri_, __FILE__, __LINE__));
        if(ref.specifier_uri_.starts_with("http://") || ref.specifier_uri_.starts_with("https://")) {
            log::debug(log::Message(__func__, "taking http/https relative path", __FILE__, __LINE__));
            std::string referrer_uri = ref.specifier_uri_;
            auto last_slash = referrer_uri.rfind('/');
            std::string base = referrer_uri.substr(0, last_slash + 1);
            if(specifier_string.starts_with("./")) {
                specifier_uri(base + std::string(specifier_string.substr(2)));
            }
            else {
                auto second_last = base.rfind('/', base.length() - 2);
                specifier_uri(base.substr(0, second_last + 1) + std::string(specifier_string.substr(3)));
            }
        }
        else {
            log::debug(log::Message(__func__, "taking relative file path", __FILE__, __LINE__));
            resolve_module_path(specifier_string);
        }
    }
    else {
        log::debug(log::Message(__func__, "taking default resolve_module_path", __FILE__, __LINE__));
        resolve_module_path(specifier_string);
    }
    log::debug(log::Message(__func__, "specifier_uri_ => " + specifier_uri_, __FILE__, __LINE__));
    log::debug(log::Message(__func__, "specifier_path_ => " + specifier_path_.string(), __FILE__, __LINE__));
    log::debug(log::Message(__func__, "specifier_protocol_ => " + specifier_protocol_, __FILE__, __LINE__));
    log::debug(log::Message(__func__, "calling fetch_and_transpile", __FILE__, __LINE__));
    transpiled_source_code_ = fetch_and_transpile((char*)specifier_uri_.c_str());
    //log::debug(log::Message(__func__, "transpiled_source_code_ => " + std::string(transpiled_source_code_.view()), __FILE__, __LINE__));
}

void slim::module::import_specifier::compile_module() {
    v8::TryCatch try_catch(isolate_);
    std::string origin_string = specifier_path_.empty() ? specifier_uri_ : specifier_path_.string();
    v8::ScriptOrigin origin(slim::utilities::StringToV8Value(isolate_, origin_string), 0, 0, false, -1, slim::utilities::StringToV8Value(isolate_, ""), false, false, true);
    v8::ScriptCompiler::Source v8_module_source(slim::utilities::StringToV8String(isolate_, std::string(transpiled_source_code_.view())), origin);
    v8::ScriptCompiler::CompileOptions module_compile_options(v8::ScriptCompiler::kProduceCompileHints);
    v8::ScriptCompiler::NoCacheReason module_no_cache_reason(v8::ScriptCompiler::kNoCacheNoReason);
    v8::MaybeLocal<v8::Module> temporary_module = v8::ScriptCompiler::CompileModule(isolate_, &v8_module_source, module_compile_options, module_no_cache_reason);
    if(!temporary_module.IsEmpty()) {
        v8_module_ = temporary_module.ToLocalChecked();
    }
    if(try_catch.HasCaught()) {
        slim::exception_handler::v8_try_catch_handler(&try_catch);
    }
}

void slim::module::import_specifier::instantiate_module() {
    v8::TryCatch try_catch(isolate_);
    auto result = v8_module_->InstantiateModule(context_, slim::module::resolver::module_call_back_resolver);
    if(result.IsNothing()) {}
    if(try_catch.HasCaught()) {
        slim::exception_handler::v8_try_catch_handler(&try_catch);
    }
}

void slim::module::import_specifier::resolve_module_path(std::string_view specifier) {
    v8::TryCatch try_catch(isolate_);
    bool module_file_found = false;
    if(!specifier.starts_with("../") && !specifier.starts_with("./") && !specifier.starts_with("/")) {
        for(auto& current_search_path : search_paths) {
            auto current_working_search_path = std::filesystem::absolute(current_search_path + std::filesystem::path::preferred_separator + std::string(specifier));
            if(current_working_search_path.has_extension()) {
                if(std::filesystem::exists(current_working_search_path)) {
                    module_file_found = true;
                    specifier_path_ = std::filesystem::canonical(current_working_search_path);
                    specifier_uri(specifier_path_.string());
                    break;
                }
            }
            else {
                std::unordered_set<std::string> possible_module_names = {
                    current_working_search_path.string(),
                    current_working_search_path.string() + "/index"
                };
                for(auto& possible_module_name : possible_module_names) {
                    for(auto& file_extension : file_extensions) {
                        std::filesystem::path possible_module_file_path = possible_module_name + file_extension;
                        if(std::filesystem::exists(possible_module_file_path)) {
                            module_file_found = true;
                            specifier_path_ = std::filesystem::canonical(possible_module_file_path);
                            specifier_uri(specifier_path_.string());
                            break;
                        }
                    }
                    if(module_file_found) { break; }
                }
            }
            if(module_file_found) { break; }
        }
    }
    else {
        auto specifier_path_uri = std::filesystem::path(specifier);
        if(referrer_.IsEmpty()) {
            auto temporary_specifier_path = std::filesystem::absolute(specifier_path_uri);
            if(std::filesystem::exists(temporary_specifier_path)) {
                specifier_path_ = std::filesystem::canonical(temporary_specifier_path);
                specifier_uri(specifier_path_.string());
            }
        }
        else {
            auto referrer_import_specifier = slim::module::resolver::get_import_specifier_by_hash_id(referrer_->GetIdentityHash());
            if(!referrer_import_specifier.has_value()) {
                isolate_->ThrowException(slim::utilities::StringToV8String(isolate_, "referrer not found in cache"));
                return;
            }
            import_specifier& ref = referrer_import_specifier.value().get();
            std::string temporary_specifier_string = specifier_path_uri.string();
            if(specifier_path_uri.string().starts_with("../")) {
                temporary_specifier_string = "/" + specifier_path_uri.string();
            }
            else if(specifier_path_uri.string().starts_with("./")) {
                temporary_specifier_string = specifier_path_uri.string().substr(1);
            }
            specifier_path_ = std::filesystem::path(ref.specifier_path_.parent_path().string() + temporary_specifier_string);
            specifier_uri(specifier_path_.string());
        }
        if(specifier_path_.has_extension()) {
            module_file_found = true;
            specifier_uri(specifier_path_.string());
        }
        else {
            std::unordered_set<std::string> possible_module_names = {
                specifier_path_.string(),
                specifier_path_.string() + "/index"
            };
            for(auto& file_extension : file_extensions) {
                for(auto& possible_module_name : possible_module_names) {
                    std::string module_file_path = possible_module_name + file_extension;
                    if(std::filesystem::exists(module_file_path)) {
                        module_file_found = true;
                        specifier_path_ = std::filesystem::canonical(module_file_path);
                        specifier_uri(specifier_path_.string());
                        break;
                    }
                }
            }
        }
    }
    if(!module_file_found) {
        isolate_->ThrowException(slim::utilities::StringToV8String(isolate_, "module not found => " + std::string(specifier)));
    }
}

void slim::module::import_specifier::specifier_uri(std::string_view s) {
    if(s.length() == 0) {
        std::string error_message = "slim::module::import_specifier::specifier_uri requires a non-zero length string";
        throw error_message;
    }
    if(s.find("://") != std::string_view::npos) {
        specifier_uri_ = s;
    }
    else {
        specifier_uri_ = "file://" + std::string(s);
    }
    specifier_protocol_ = specifier_uri_.substr(0, specifier_uri_.find("://"));
}

const std::string& slim::module::import_specifier::specifier_uri() const {
    return specifier_uri_;
}

v8::Local<v8::Module>& slim::module::import_specifier::v8_module() {
    return v8_module_;
}
