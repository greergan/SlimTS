#include <filesystem>
#include <string>
#include <thread>
//#include <slim/slim_duckdb.h>
#include <slim/common/log.h>
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
static std::jthread watcher_thread;
static slim::file::watcher::Watcher* file_watcher{nullptr};

void watch_file_changes() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    static slim::file::watcher::Watcher watcher([]() {
        log::debug(log::Message(__func__, "file change detected, restarting", __FILE__, __LINE__));
        slim::restart();
        slim::file::watcher::clear();
        slim::file::watcher::add(slim::configuration_handler::get_script_name());
    });
    file_watcher = &watcher;
    slim::file::watcher::add(slim::configuration_handler::get_script_name());
    watcher_thread = std::jthread(slim::file::watcher::watch);
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}
} // namespace

std::string slim::configuration_handler::get_script_name() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    auto result = slim::common::memory_mapper::read_string("slim_runtime_environmental_variables", "script_name");
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
    return result;
}

bool slim::configuration_handler::is_daemon() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    auto result = slim::common::memory_mapper::read_bool("slim_runtime_environmental_variables", "daemon");
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
    return result;
}

bool slim::configuration_handler::is_watching() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    auto result = slim::common::memory_mapper::read_bool("slim_runtime_environmental_variables", "watching_files");
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
    return result;
}

void slim::configuration_handler::load() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    if(is_watching()) {
        log::debug(log::Message(__func__, "starting file watcher", __FILE__, __LINE__));
        watch_file_changes();
    }
    if(!std::filesystem::exists(default_configuration_file)) {
        log::debug(log::Message(__func__, std::format("configuration file not found => {}", default_configuration_file), __FILE__, __LINE__));
        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
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
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}
