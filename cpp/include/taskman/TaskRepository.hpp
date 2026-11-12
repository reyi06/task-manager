#pragma once

#include "taskman/Database.hpp"
#include "taskman/Task.hpp"

#include <optional>
#include <vector>

namespace taskman {

class TaskRepository {
public:
    explicit TaskRepository(Database& database);

    std::vector<Task> findAll();
    std::optional<Task> findById(int id);
    int insert(const Task& task);
    void update(const Task& task);
    bool remove(int id);
    int clearCompleted();

private:
    Database& database_;
};

}  // namespace taskman
