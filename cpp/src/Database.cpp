#include "taskman/Database.hpp"

#include <sqlite3.h>

#include <set>
#include <stdexcept>
#include <string>

namespace taskman {

namespace {

void execute(sqlite3* database, const std::string& sql) {
    char* error = nullptr;
    if (sqlite3_exec(database, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
        const std::string message = error != nullptr ? error : "unknown database error";
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}

std::set<std::string> tableColumns(sqlite3* database) {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, "PRAGMA table_info(tasks)", -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }

    std::set<std::string> columns;
    int result = SQLITE_ROW;
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(statement, 1);
        if (name != nullptr) {
            columns.insert(reinterpret_cast<const char*>(name));
        }
    }
    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return columns;
}

}  // namespace

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        const std::string message = db_ != nullptr ? sqlite3_errmsg(db_) : "unknown database error";
        if (db_ != nullptr) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
        throw std::runtime_error("Could not open database: " + message);
    }
    sqlite3_busy_timeout(db_, 5000);
}

Database::~Database() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
    }
}

void Database::initializeSchema() {
    execute(db_,
            "CREATE TABLE IF NOT EXISTS tasks ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "title TEXT NOT NULL,"
            "priority INTEGER NOT NULL DEFAULT 2,"
            "category TEXT NOT NULL DEFAULT 'General',"
            "due_date TEXT NOT NULL DEFAULT '',"
            "done INTEGER NOT NULL DEFAULT 0,"
            "created_at TEXT NOT NULL DEFAULT '',"
            "completed_at TEXT NOT NULL DEFAULT ''"
            ")");

    const std::set<std::string> columns = tableColumns(db_);
    if (columns.find("priority") == columns.end()) {
        execute(db_, "ALTER TABLE tasks ADD COLUMN priority INTEGER NOT NULL DEFAULT 2");
    }
    if (columns.find("category") == columns.end()) {
        execute(db_, "ALTER TABLE tasks ADD COLUMN category TEXT NOT NULL DEFAULT 'General'");
    }
    if (columns.find("due_date") == columns.end()) {
        execute(db_, "ALTER TABLE tasks ADD COLUMN due_date TEXT NOT NULL DEFAULT ''");
    }
    if (columns.find("created_at") == columns.end()) {
        execute(db_, "ALTER TABLE tasks ADD COLUMN created_at TEXT NOT NULL DEFAULT ''");
    }
    if (columns.find("completed_at") == columns.end()) {
        execute(db_, "ALTER TABLE tasks ADD COLUMN completed_at TEXT NOT NULL DEFAULT ''");
    }

    execute(db_,
            "UPDATE tasks SET created_at = strftime('%Y-%m-%dT%H:%M:%SZ', 'now') "
            "WHERE created_at = ''");
}

}  // namespace taskman
