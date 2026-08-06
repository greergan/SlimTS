#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>
//#include <slim/slim_duckdb.h>
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/common/memory/mapper.h>
#include <slim/configuration_handler.h>
#include <slim/file/watcher.h>
#include <slim/slim.h>
#include <slim/slim_v8.h>

namespace {
using namespace slim::common;
std::string default_configuration_file_name = "./slim_configuration.json";
std::string default_configuration_file = std::filesystem::current_path().string() + std::filesystem::path::preferred_separator
    + default_configuration_file_name;
std::jthread watcher_thread;
slim::file::watcher::Watcher* file_watcher{nullptr};
std::vector<std::string> module_lib_path;

// keep the order in which module_lib_path is loaded
void make_lib_path() {
    module_lib_path.reserve(2);
    auto prog_dir = std::filesystem::path(slim::configuration_handler::get_script_name()).parent_path();
    if(prog_dir.filename() == "src") {
        module_lib_path.push_back(std::format("{}", prog_dir.string() + "/lib"));
        prog_dir = prog_dir.parent_path();
    }
    module_lib_path.push_back(std::format("{}", prog_dir.string() + "/lib"));
    module_lib_path.push_back((std::filesystem::path(std::getenv("HOME")) / "slimts/lib").string());
    if(const char* env = std::getenv("SLIMTS_LIB")) {
    	std::string_view libs(env);
    	for(size_t start = 0, end; ; start = end + 1) {
    		end = libs.find(':', start);
    		auto path = libs.substr(start, end == std::string_view::npos ? end : end - start);
    		if(!path.empty()) module_lib_path.emplace_back(path);
    		if(end == std::string_view::npos) break;
    	}
    }
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__, std::format("project directory => {}", prog_dir.string()), __FILE__, __LINE__));
    for(auto& p : module_lib_path) {
        log::debug(log::Message(__func__, std::format("project library path => {}", p), __FILE__, __LINE__));
    }
#endif
}

void watch_file_changes() {
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
    static slim::file::watcher::Watcher watcher([]() {
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, "file change detected, restarting", __FILE__, __LINE__));
#endif
        slim::restart();
        slim::file::watcher::clear();
        slim::file::watcher::add(slim::configuration_handler::get_script_name());
    });
    file_watcher = &watcher;
    slim::file::watcher::add(slim::configuration_handler::get_script_name());
    watcher_thread = std::jthread(slim::file::watcher::watch);
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
}
} // namespace

std::string slim::configuration_handler::get_script_name() {
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
    auto result = slim::common::memory_mapper::read_string("slim_runtime_environmental_variables", "script_name");
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
    return result;
}

bool slim::configuration_handler::is_daemon() {
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
    auto result = slim::common::memory_mapper::read_bool("slim_runtime_environmental_variables", "daemon");
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
    return result;
}

bool slim::configuration_handler::is_watching() {
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
    auto result = slim::common::memory_mapper::read_bool("slim_runtime_environmental_variables", "watching_files");
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
    return result;
}

std::span<std::string> slim::configuration_handler::library_path() {
    return module_lib_path;
}

void slim::configuration_handler::load() {
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
    if(is_watching()) {
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, "starting file watcher", __FILE__, __LINE__));
#endif
        watch_file_changes();
    }
    make_lib_path();
    if(!std::filesystem::exists(default_configuration_file)) {
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, std::format("configuration file not found => {}", default_configuration_file), __FILE__, __LINE__));
#endif
#ifdef ENABLE_LOGGING
        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
        return;
    }
    // slim::duck_db::Database db;
    // slim::duck_db::Connection con(db);
    // auto result = con.query("SELECT * FROM read_json_auto($1)", default_configuration_file);
    // if(result.has_error()) {
    //     return;
    // }
//    for(idx_t row = 0; row < result.row_count(); row++) {
//        auto key = result.get_string("key", row);
//        auto value = result.get_bool("value", row);
//        memory_mapper::write("config_map", key, value);
//    }
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
}
