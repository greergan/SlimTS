#include <filesystem>
#include <string>
#include <slim/slim_duckdb.h>
#include <slim/common/memory/mapper.h>
#include <slim/configuration_handler.h>

namespace slim::configuration_handler {
    std::string default_configuration_file_name = "./slim_configuration.json";
    std::string default_configuration_file =
        std::filesystem::current_path().string() +
        std::filesystem::path::preferred_separator +
        default_configuration_file_name;
}

void slim::configuration_handler::load() {
    if(!std::filesystem::exists(default_configuration_file)) {
        return;
    }
    slim::duck_db::Database db;
    slim::duck_db::Connection con(db);
    auto result = con.query("SELECT * FROM read_json_auto($1)", default_configuration_file);
    if(result.has_error()) {
        return;
    }
//    for(idx_t row = 0; row < result.row_count(); row++) {
//        auto key = result.get_string("key", row);
//        auto value = result.get_bool("value", row);
//        memory_mapper::write("config_map", key, value);
//    }
}
