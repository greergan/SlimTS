#include <filesystem>
#include <string>
#include <duckdb.hpp>
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

    duckdb::DuckDB db(nullptr);
    duckdb::Connection con(db);
    auto prepared = con.Prepare("SELECT * FROM read_json_auto($1)");
    auto result = prepared->Execute(default_configuration_file);

    if(result->HasError()) {
        return;
    }

//    for(auto& row : *result) {
//        std::string key = row.GetValue<std::string>(0);
//        bool value = row.GetValue<bool>(1);
//        memory_mapper::write("config_map", key, value);
//    }
}
