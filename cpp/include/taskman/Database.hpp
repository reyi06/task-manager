#pragma once

#include <string>

struct sqlite3;

namespace taskman {

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    sqlite3* handle() const { return db_; }
    void initializeSchema();

private:
    sqlite3* db_ = nullptr;
};

}  // namespace taskman
