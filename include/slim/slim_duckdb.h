#ifndef SLIM_DUCKDB_HP
#define SLIM_DUCKDB_HP

#include <duckdb.h>
#include <string>

namespace slim::duck_db {

class Database;

class Result {
public:
    Result() = default;
    ~Result() { duckdb_destroy_result(&result); }
    bool has_error() const;
    std::string error() const;
    idx_t row_count();
    idx_t column_count();
    idx_t column_index(const std::string& name);
    bool get_bool(const std::string& key, idx_t row = 0);
    std::string get_string(const std::string& key, idx_t row = 0);
private:
    friend class Connection;
    duckdb_result result{};
    std::string error_message;
};

class Connection {
public:
    explicit Connection(Database& db);
    ~Connection() { duckdb_disconnect(&connection); }
    Result query(const std::string& sql);
    Result query(const std::string& sql, const std::string& param);
private:
    duckdb_connection connection{};
};

class Database {
public:
    explicit Database(const std::string& path = ":memory:");
    ~Database() { duckdb_close(&db); }
private:
    friend class Connection;
    duckdb_database db{};
};

} // namespace slim::duck_db

#endif // SLIM_DUCKDB_HP
