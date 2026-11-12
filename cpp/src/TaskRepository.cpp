#include "taskman/TaskRepository.hpp"

#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace taskman {

namespace {

class Statement {
public:
    Statement(sqlite3* database, const char* sql) : database_(database) {
        if (sqlite3_prepare_v2(database_, sql, -1, &statement_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database_));
        }
    }

    ~Statement() {
        if (statement_ != nullptr) {
            sqlite3_finalize(statement_);
        }
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    void bindText(int index, const std::string& value) {
        if (sqlite3_bind_text(statement_, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database_));
        }
    }

    void bindInt(int index, int value) {
        if (sqlite3_bind_int(statement_, index, value) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database_));
        }
    }

    bool stepRow() {
        const int result = sqlite3_step(statement_);
        if (result == SQLITE_ROW) {
            return true;
        }
        if (result == SQLITE_DONE) {
            return false;
        }
        throw std::runtime_error(sqlite3_errmsg(database_));
    }

    void stepDone() {
        if (sqlite3_step(statement_) != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(database_));
        }
    }

    int columnInt(int index) const {
        return sqlite3_column_int(statement_, index);
    }

    std::string columnText(int index) const {
        const unsigned char* text = sqlite3_column_text(statement_, index);
        return text != nullptr ? reinterpret_cast<const char*>(text) : "";
    }

private:
    sqlite3* database_ = nullptr;
    sqlite3_stmt* statement_ = nullptr;
};

Task readTask(Statement& statement) {
    Task task;
    task.id = statement.columnInt(0);
    task.title = statement.columnText(1);
    task.priority = priorityFromCode(statement.columnInt(2));
    task.category = statement.columnText(3);
    task.dueDate = statement.columnText(4);
    task.done = statement.columnInt(5) != 0;
    task.createdAt = statement.columnText(6);
    task.completedAt = statement.columnText(7);
    return task;
}

}  // namespace

TaskRepository::TaskRepository(Database& database) : database_(database) {}

std::vector<Task> TaskRepository::findAll() {
    Statement statement(database_.handle(),
                        "SELECT id, title, priority, category, due_date, done, created_at, completed_at "
                        "FROM tasks ORDER BY id");

    std::vector<Task> tasks;
    while (statement.stepRow()) {
        tasks.push_back(readTask(statement));
    }
    return tasks;
}

std::optional<Task> TaskRepository::findById(int id) {
    Statement statement(database_.handle(),
                        "SELECT id, title, priority, category, due_date, done, created_at, completed_at "
                        "FROM tasks WHERE id = ?");
    statement.bindInt(1, id);
    if (!statement.stepRow()) {
        return std::nullopt;
    }
    return readTask(statement);
}

int TaskRepository::insert(const Task& task) {
    Statement statement(database_.handle(),
                        "INSERT INTO tasks "
                        "(title, priority, category, due_date, done, created_at, completed_at) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?)");
    statement.bindText(1, task.title);
    statement.bindInt(2, static_cast<int>(task.priority));
    statement.bindText(3, task.category);
    statement.bindText(4, task.dueDate);
    statement.bindInt(5, task.done ? 1 : 0);
    statement.bindText(6, task.createdAt);
    statement.bindText(7, task.completedAt);
    statement.stepDone();
    return static_cast<int>(sqlite3_last_insert_rowid(database_.handle()));
}

void TaskRepository::update(const Task& task) {
    Statement statement(database_.handle(),
                        "UPDATE tasks SET title = ?, priority = ?, category = ?, "
                        "due_date = ?, done = ?, completed_at = ? WHERE id = ?");
    statement.bindText(1, task.title);
    statement.bindInt(2, static_cast<int>(task.priority));
    statement.bindText(3, task.category);
    statement.bindText(4, task.dueDate);
    statement.bindInt(5, task.done ? 1 : 0);
    statement.bindText(6, task.completedAt);
    statement.bindInt(7, task.id);
    statement.stepDone();
}

bool TaskRepository::remove(int id) {
    Statement statement(database_.handle(), "DELETE FROM tasks WHERE id = ?");
    statement.bindInt(1, id);
    statement.stepDone();
    return sqlite3_changes(database_.handle()) > 0;
}

int TaskRepository::clearCompleted() {
    Statement statement(database_.handle(), "DELETE FROM tasks WHERE done = 1");
    statement.stepDone();
    return sqlite3_changes(database_.handle());
}

}  // namespace taskman
