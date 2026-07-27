#include <slim/slim_duckdb.h>

namespace slim::duck_db {

Database::Database(const std::string& path) {
    const char* p = (path == ":memory:") ? nullptr : path.c_str();
    duckdb_open(p, &db);
}

Connection::Connection(Database& db) {
    duckdb_connect(db.db, &connection);
}

Result Connection::query(const std::string& sql) {
    Result r;
    if(duckdb_query(connection, sql.c_str(), &r.result) != DuckDBSuccess) {
        r.error_message = duckdb_result_error(&r.result);
    }
    return r;
}

Result Connection::query(const std::string& sql, const std::string& param) {
    Result r;
    duckdb_prepared_statement stmt;
    if(duckdb_prepare(connection, sql.c_str(), &stmt) != DuckDBSuccess) {
        r.error_message = duckdb_prepare_error(stmt);
        duckdb_destroy_prepare(&stmt);
        return r;
    }
    duckdb_bind_varchar(stmt, 1, param.c_str());
    if(duckdb_execute_prepared(stmt, &r.result) != DuckDBSuccess) {
        r.error_message = duckdb_result_error(&r.result);
    }
    duckdb_destroy_prepare(&stmt);
    return r;
}

bool Result::has_error() const {
    return !error_message.empty();
}

std::string Result::error() const {
    return error_message;
}

idx_t Result::row_count() {
    return duckdb_row_count(&result);
}

idx_t Result::column_count() {
    return duckdb_column_count(&result);
}

idx_t Result::column_index(const std::string& name) {
    for(idx_t i = 0; i < column_count(); i++) {
        if(duckdb_column_name(&result, i) == name) return i;
    }
    return DUCKDB_ERROR_INDEX;
}

bool Result::get_bool(const std::string& key, idx_t row) {
    return duckdb_value_boolean(&result, column_index(key), row);
}

std::string Result::get_string(const std::string& key, idx_t row) {
    auto val = duckdb_value_varchar(&result, column_index(key), row);
    std::string s = val ? val : "";
    duckdb_free(val);
    return s;
}

} // namespace slim::duck_db
