#define ENABLE_LOGGING
#include <filesystem>
#include <format>
#include <string>
#include <unordered_set>
#include <v8.h>
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/common/utilities.h>
#include <slim/exception_handler.h>
#include <slim/fetch.h>
#include <slim/module/import_specifier.h>
#include <slim/module/resolver.h>
#include <slim/utilities.h>
#include <slim/configuration_handler.h>

namespace slim::module {
    using namespace slim;
    using namespace slim::common;
    std::unordered_set<std::string> file_extensions = {".ts", ".mjs"};
}

slim::module::import_specifier::import_specifier(v8::Isolate* isolate, std::string_view specifier_string, v8::Local<v8::Module> synthetic_module)
    : isolate_(isolate), v8_module_(synthetic_module), context_(isolate->GetCurrentContext()) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
    log::debug({__func__, std::format("specifier_string => {}", specifier_string), __FILE__, __LINE__});
    log::debug({__func__, std::format("synthetic_module.IsEmpty => {}", synthetic_module.IsEmpty() ? "true" : "false"), __FILE__, __LINE__});
#endif
    specifier_uri_ = specifier_string;
    instantiate_module();
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}

slim::module::import_specifier::import_specifier(v8::Isolate* isolate, std::string_view specifier_string, bool is_entry_point, v8::Local<v8::Module> referrer)
        : isolate_(isolate), referrer_(referrer), is_entry_point_(is_entry_point), context_(isolate->GetCurrentContext()) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
    log::debug({__func__, std::format("specifier_string => {}", specifier_string), __FILE__, __LINE__});
    log::debug({__func__, std::format("is_entry_point => {}", is_entry_point ? "true" : "false"), __FILE__, __LINE__});
    log::debug({__func__, std::format("referrer_.IsEmpty => {}", referrer_.IsEmpty() ? "true" : "false"), __FILE__, __LINE__});
#endif
    if(specifier_string.starts_with("http://") || specifier_string.starts_with("https://")) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "taking http/https path", __FILE__, __LINE__});
#endif
        specifier_uri(specifier_string);
    }
    else if((specifier_string.starts_with("./") || specifier_string.starts_with("../")) && !referrer_.IsEmpty()) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "taking relative path with referrer", __FILE__, __LINE__});
#endif
        auto referrer_import_specifier = slim::module::resolver::get_import_specifier_by_hash_id(referrer_->GetIdentityHash());
        if(!referrer_import_specifier.has_value()) {
#ifdef ENABLE_LOGGING
            log::debug({__func__, "referrer not found in cache", __FILE__, __LINE__});
#endif
            isolate_->ThrowException(slim::utilities::StringToV8String(isolate_, "referrer not found in cache"));
            return;
        }
        import_specifier& ref = referrer_import_specifier.value().get();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("referrer specifier_uri => {}", ref.specifier_uri_), __FILE__, __LINE__});
#endif
        if(ref.specifier_uri_.starts_with("http://") || ref.specifier_uri_.starts_with("https://")) {
#ifdef ENABLE_LOGGING
            log::debug({__func__, "taking http/https relative path", __FILE__, __LINE__});
#endif
            std::string referrer_uri = ref.specifier_uri_;
            auto last_slash = referrer_uri.rfind('/');
            std::string base = referrer_uri.substr(0, last_slash + 1);
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("referrer_uri => {}", referrer_uri), __FILE__, __LINE__});
            log::debug({__func__, std::format("base => {}", base), __FILE__, __LINE__});
#endif
            if(specifier_string.starts_with("./")) {
                specifier_uri(base + std::string(specifier_string.substr(2)));
            }
            else {
                auto second_last = base.rfind('/', base.length() - 2);
                specifier_uri(base.substr(0, second_last + 1) + std::string(specifier_string.substr(3)));
            }
        }
        else {
#ifdef ENABLE_LOGGING
            log::debug({__func__, "taking relative file path", __FILE__, __LINE__});
#endif
            resolve_module_path(specifier_string);
        }
    }
    else {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "taking default resolve_module_path", __FILE__, __LINE__});
#endif
        resolve_module_path(specifier_string);
    }
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("specifier_uri_ => {}", specifier_uri_), __FILE__, __LINE__});
    log::debug({__func__, std::format("specifier_path_ => {}", specifier_path_.string()), __FILE__, __LINE__});
    log::debug({__func__, std::format("specifier_protocol_ => {}", specifier_protocol_), __FILE__, __LINE__});
#endif
    if(specifier_uri_.ends_with(".ts")) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "calling fetch_and_transpile", __FILE__, __LINE__});
#endif
        transpiled_source_ = fetch_and_transpile((char*)specifier_uri_.c_str());
        is_src_transpiled = true;
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("transpiled_source_ size => {}", transpiled_source_.view().size()), __FILE__, __LINE__});
#endif
    }
    else if(specifier_uri_.ends_with(".js")) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "taking cjs path, calling fetch", __FILE__, __LINE__});
#endif
        fetched_mjs_source_ = slim::fetch::fetch_file(specifier_uri_).get();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("fetched_mjs_source_.code => {}", fetched_mjs_source_.code), __FILE__, __LINE__});
        log::debug({__func__, std::format("fetched_mjs_source_.body.size => {}", fetched_mjs_source_.body.size()), __FILE__, __LINE__});
#endif
        if(fetched_mjs_source_.code != 200) {
            std::string error_string = std::format("fetch failed: {}, error: {}, text: {}", specifier_uri_,
                fetched_mjs_source_.code, fetched_mjs_source_.code_text);
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("fetch failed => {}", error_string), __FILE__, __LINE__});
#endif
            isolate_->ThrowException(slim::utilities::StringToV8String(isolate_, error_string.data()));
            return;
        }
        is_cjs_ = true;
#ifdef ENABLE_LOGGING
        log::debug({__func__, "is_cjs_ set true, calling evaluate_as_cjs", __FILE__, __LINE__});
#endif
        evaluate_as_cjs();
    }
    else {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "calling fetch", __FILE__, __LINE__});
#endif
        fetched_mjs_source_ = slim::fetch::fetch_file(specifier_uri_).get();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("fetched_mjs_source_.code => {}", fetched_mjs_source_.code), __FILE__, __LINE__});
        log::debug({__func__, std::format("fetched_mjs_source_.body.size => {}", fetched_mjs_source_.body.size()), __FILE__, __LINE__});
#endif
        if(fetched_mjs_source_.code != 200) {
            std::string error_string = std::format("fetch failed: {}, error: {}, text: {}", specifier_uri_,
                fetched_mjs_source_.code, fetched_mjs_source_.code_text);
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("fetch failed => {}", error_string), __FILE__, __LINE__});
#endif
            isolate_->ThrowException(slim::utilities::StringToV8String(isolate_, error_string.data()));
        }
    }
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}

void slim::module::import_specifier::compile_module() {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
    log::debug({__func__, std::format("specifier_uri_ => {}", specifier_uri_), __FILE__, __LINE__});
    log::debug({__func__, std::format("is_src_transpiled => {}", is_src_transpiled ? "true" : "false"), __FILE__, __LINE__});
    log::debug({__func__, std::format("is_cjs_ => {}", is_cjs_ ? "true" : "false"), __FILE__, __LINE__});
#endif
    // CJS modules are already compiled and wrapped as synthetic modules in evaluate_as_cjs()
    if(is_cjs_) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "is_cjs_ true, skipping compile", __FILE__, __LINE__});
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
        return;
    }
    v8::TryCatch try_catch(isolate_);
    std::string_view origin_string = specifier_uri_;
    std::string_view src;
    if(is_src_transpiled) {
        src = transpiled_source_.view();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("using transpiled source, size => {}", src.size()), __FILE__, __LINE__});
#endif
    }
    else {
        src = std::string_view(reinterpret_cast<const char*>(fetched_mjs_source_.body.data()), fetched_mjs_source_.body.size());
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("using fetched source, size => {}", src.size()), __FILE__, __LINE__});
#endif
    }
    v8::Local<v8::String> source_string = v8::String::NewFromUtf8(
        isolate_,
        src.data(),
        v8::NewStringType::kNormal,
        static_cast<int>(src.size())
    ).ToLocalChecked();
#ifdef ENABLE_LOGGING
    log::debug({__func__, "source_string created, compiling module", __FILE__, __LINE__});
#endif
    v8::ScriptOrigin origin(slim::utilities::StringToV8Value(isolate_, origin_string.data()), 0, 0, false, -1, slim::utilities::StringToV8Value(isolate_, ""), false, false, true);
    v8::ScriptCompiler::Source v8_module_source(source_string, origin);
    v8::ScriptCompiler::CompileOptions module_compile_options(v8::ScriptCompiler::kProduceCompileHints);
    v8::ScriptCompiler::NoCacheReason module_no_cache_reason(v8::ScriptCompiler::kNoCacheNoReason);
    v8::MaybeLocal<v8::Module> temporary_module = v8::ScriptCompiler::CompileModule(isolate_, &v8_module_source, module_compile_options, module_no_cache_reason);
    if(!temporary_module.IsEmpty()) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "module compiled successfully", __FILE__, __LINE__});
#endif
        v8_module_ = temporary_module.ToLocalChecked();
    }
    if(try_catch.HasCaught()) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "exception caught during compile", __FILE__, __LINE__});
#endif
        slim::exception_handler::v8_try_catch_handler(&try_catch);
    }
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}

void slim::module::import_specifier::instantiate_module() {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
    log::debug({__func__, std::format("v8_module_.IsEmpty => {}", v8_module_.IsEmpty() ? "true" : "false"), __FILE__, __LINE__});
#endif
    v8::TryCatch try_catch(isolate_);
    auto result = v8_module_->InstantiateModule(context_, slim::module::resolver::module_call_back_resolver);
    if(result.IsNothing()) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "InstantiateModule returned nothing", __FILE__, __LINE__});
#endif
    }
    if(try_catch.HasCaught()) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "exception caught during instantiate", __FILE__, __LINE__});
#endif
        slim::exception_handler::v8_try_catch_handler(&try_catch);
    }
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}

void slim::module::import_specifier::resolve_module_path(std::string_view specifier) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    static auto search_paths = slim::configuration_handler::library_path();
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("specifier => {}", specifier), __FILE__, __LINE__});
    log::debug({__func__, std::format("referrer_.IsEmpty => {}", referrer_.IsEmpty() ? "true" : "false"), __FILE__, __LINE__});
    log::debug({__func__, std::format("search_paths.size => {}", search_paths.size()), __FILE__, __LINE__});
#endif
    v8::TryCatch try_catch(isolate_);
    bool module_file_found = false;
    if(!specifier.starts_with("../") && !specifier.starts_with("./") && !specifier.starts_with("/")) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "taking bare specifier search path resolution", __FILE__, __LINE__});
#endif
        for(auto& current_search_path : search_paths) {
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("current_search_path => {}", current_search_path), __FILE__, __LINE__});
#endif
            auto current_working_search_path = std::filesystem::absolute(current_search_path + std::filesystem::path::preferred_separator + std::string(specifier));
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("current_working_search_path => {}", current_working_search_path.string()), __FILE__, __LINE__});
#endif
            if(current_working_search_path.has_extension()) {
                if(std::filesystem::exists(current_working_search_path)) {
                    module_file_found = true;
                    specifier_path_ = std::filesystem::canonical(current_working_search_path);
                    specifier_uri(specifier_path_.string());
#ifdef ENABLE_LOGGING
                    log::debug({__func__, std::format("found with extension => {}", specifier_path_.string()), __FILE__, __LINE__});
#endif
                    break;
                }
            }
            else {
#ifdef ENABLE_LOGGING
                log::debug({__func__, std::format("trying package.json in => {}", current_working_search_path.string()), __FILE__, __LINE__});
#endif
                std::filesystem::path package_json_path = current_working_search_path / "package.json";
                auto package_json_result = slim::fetch::fetch_file("file://" + package_json_path.string()).get();
                if(package_json_result.code != 200) {
#ifdef ENABLE_LOGGING
                    log::debug({__func__, std::format("package.json not found, skipping => {}", package_json_path.string()), __FILE__, __LINE__});
#endif
                    continue;
                }
#ifdef ENABLE_LOGGING
                log::debug({__func__, std::format("package.json fetched => {}", package_json_path.string()), __FILE__, __LINE__});
#endif
                std::string_view package_json_body(reinterpret_cast<const char*>(package_json_result.body.data()), package_json_result.body.size());
                v8::MaybeLocal<v8::Value> maybe_parsed = v8::JSON::Parse(context_, slim::utilities::StringViewToV8String(isolate_, package_json_body));
                if(maybe_parsed.IsEmpty()) {
                    isolate_->ThrowException(slim::utilities::StringToV8String(isolate_, std::format("failed to parse package.json => {}", package_json_path.string()).c_str()));
                    return;
                }
#ifdef ENABLE_LOGGING
                log::debug({__func__, "package.json parsed", __FILE__, __LINE__});
#endif
                v8::Local<v8::Object> root_obj = slim::utilities::GetObject(isolate_, maybe_parsed.ToLocalChecked());
                v8::Local<v8::Object> exports_obj = slim::utilities::GetObject(isolate_, "exports", root_obj);
                v8::Local<v8::Object> dot_obj = slim::utilities::GetObject(isolate_, ".", exports_obj);

                // GetValue returns JS undefined for missing keys; check IsString before converting
                auto get_string_export = [&](v8::Local<v8::Object> obj, const char* key) -> std::string {
                    v8::Local<v8::Value> val = slim::utilities::GetValue(isolate_, key, obj);
                    if(val.IsEmpty() || !val->IsString()) return {};
                    return slim::utilities::StringValue(isolate_, val);
                };

                // lookup order: exports[.][import] -> exports[.][default] -> main
                std::string import_value = get_string_export(dot_obj, "import");
                if(import_value.empty()) {
                    import_value = get_string_export(dot_obj, "default");
                }
                if(import_value.empty()) {
                    // no exports["."] at all — fall back to "main"
                    import_value = get_string_export(root_obj, "main");
                }
                if(import_value.empty()) {
                    // fallback to index.js
                    std::filesystem::path index_path = current_working_search_path / "index.js";
                    if(std::filesystem::exists(index_path)) {
                        import_value = "index.js";
                    }
                }
                if(import_value.empty()) {
                    isolate_->ThrowException(slim::utilities::StringToV8String(isolate_,
                        std::format("no resolvable export entry in package.json => {}", package_json_path.string()).c_str()));
                    return;
                }
#ifdef ENABLE_LOGGING
                log::debug({__func__, std::format("import_value => {}", import_value), __FILE__, __LINE__});
#endif
                std::filesystem::path import_path = current_working_search_path / import_value;
                if(!std::filesystem::exists(import_path)) {
                    isolate_->ThrowException(slim::utilities::StringToV8String(isolate_, std::format("import file not found => {}", import_path.string()).c_str()));
                    return;
                }
                module_file_found = true;
                specifier_path_ = std::filesystem::canonical(import_path);
                specifier_uri(specifier_path_.string());
#ifdef ENABLE_LOGGING
                log::debug({__func__, std::format("found via package.json => {}", specifier_path_.string()), __FILE__, __LINE__});
#endif
            }
            if(module_file_found) { break; }
        }
    }
    else {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "taking relative/absolute path resolution", __FILE__, __LINE__});
#endif
        auto specifier_path_uri = std::filesystem::path(specifier);
        if(referrer_.IsEmpty()) {
#ifdef ENABLE_LOGGING
            log::debug({__func__, "no referrer, resolving against cwd", __FILE__, __LINE__});
#endif
            auto temporary_specifier_path = std::filesystem::absolute(specifier_path_uri);
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("temporary_specifier_path => {}", temporary_specifier_path.string()), __FILE__, __LINE__});
#endif
            if(std::filesystem::exists(temporary_specifier_path)) {
                specifier_path_ = std::filesystem::canonical(temporary_specifier_path);
                specifier_uri(specifier_path_.string());
#ifdef ENABLE_LOGGING
                log::debug({__func__, std::format("resolved specifier_path_ => {}", specifier_path_.string()), __FILE__, __LINE__});
#endif
            }
        }
        else {
            auto referrer_import_specifier = slim::module::resolver::get_import_specifier_by_hash_id(referrer_->GetIdentityHash());
            if(!referrer_import_specifier.has_value()) {
#ifdef ENABLE_LOGGING
                log::debug({__func__, "referrer not found in cache", __FILE__, __LINE__});
#endif
                isolate_->ThrowException(slim::utilities::StringToV8String(isolate_, "referrer not found in cache"));
                return;
            }
            import_specifier& ref = referrer_import_specifier.value().get();
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("referrer specifier_path_ => {}", ref.specifier_path_.string()), __FILE__, __LINE__});
#endif
            std::string temporary_specifier_string = specifier_path_uri.string();
            if(specifier_path_uri.string().starts_with("../")) {
                temporary_specifier_string = "/" + specifier_path_uri.string();
            }
            else if(specifier_path_uri.string().starts_with("./")) {
                temporary_specifier_string = specifier_path_uri.string().substr(1);
            }
            else {
                // bare /name with referrer — resolve filename against referrer's directory
                temporary_specifier_string = "/" + specifier_path_uri.filename().string();
            }
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("temporary_specifier_string => {}", temporary_specifier_string), __FILE__, __LINE__});
#endif
            specifier_path_ = std::filesystem::path(ref.specifier_path_.parent_path().string() + temporary_specifier_string);
            specifier_uri(specifier_path_.string());
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("resolved specifier_path_ => {}", specifier_path_.string()), __FILE__, __LINE__});
#endif
        }
        if(specifier_path_.has_extension()) {
            module_file_found = true;
            specifier_uri(specifier_path_.string());
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("path has extension, module_file_found => {}", specifier_path_.string()), __FILE__, __LINE__});
#endif
        }
        else {
            std::unordered_set<std::string> possible_module_names = {
                specifier_path_.string(),
                specifier_path_.string() + "/index"
            };
            for(auto& file_extension : file_extensions) {
                for(auto& possible_module_name : possible_module_names) {
                    std::string module_file_path = possible_module_name + file_extension;
#ifdef ENABLE_LOGGING
                    log::debug({__func__, std::format("trying => {}", module_file_path), __FILE__, __LINE__});
#endif
                    if(std::filesystem::exists(module_file_path)) {
                        module_file_found = true;
                        specifier_path_ = std::filesystem::canonical(module_file_path);
                        specifier_uri(specifier_path_.string());
#ifdef ENABLE_LOGGING
                        log::debug({__func__, std::format("found => {}", specifier_path_.string()), __FILE__, __LINE__});
#endif
                        break;
                    }
                }
            }
        }
    }
    if(!module_file_found) {
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("module not found => {}", specifier), __FILE__, __LINE__});
#endif
        isolate_->ThrowException(slim::utilities::StringToV8String(isolate_, std::format("module not found => {}", specifier).c_str()));
    }
#ifdef ENABLE_LOGGING
    else {
        log::debug({__func__, std::format("module_file_found => {}", specifier_path_.string()), __FILE__, __LINE__});
        log::debug({__func__, std::format("resolved specifier_uri_ => {}", specifier_uri_), __FILE__, __LINE__});
    }
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}

void slim::module::import_specifier::specifier_uri(std::string_view s) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
    log::debug({__func__, std::format("s => {}", s), __FILE__, __LINE__});
#endif
    if(s.length() == 0) {
        std::string error_message = "slim::module::import_specifier::specifier_uri requires a non-zero length string";
        throw error_message;
    }
    if(s.find("://") != std::string_view::npos) {
        specifier_uri_ = s;
    }
    else {
        specifier_uri_ = std::format("file://{}", s);
    }
    specifier_protocol_ = specifier_uri_.substr(0, specifier_uri_.find("://"));
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("specifier_uri_ => {}", specifier_uri_), __FILE__, __LINE__});
    log::debug({__func__, std::format("specifier_protocol_ => {}", specifier_protocol_), __FILE__, __LINE__});
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}

const std::string& slim::module::import_specifier::specifier_uri() const {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
    log::debug({__func__, std::format("specifier_uri_ => {}", specifier_uri_), __FILE__, __LINE__});
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    return specifier_uri_;
}

v8::Local<v8::Module>& slim::module::import_specifier::v8_module() {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
    log::debug({__func__, std::format("v8_module_.IsEmpty => {}", v8_module_.IsEmpty() ? "true" : "false"), __FILE__, __LINE__});
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    return v8_module_;
}
