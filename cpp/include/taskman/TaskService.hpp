#pragma once

#include "taskman/Task.hpp"
#include "taskman/TaskRepository.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace taskman {

class TaskNotFound : public std::runtime_error {
public:
    explicit TaskNotFound(int id);
};

struct TaskQuery {
    std::string keyword;
    std::optional<bool> done;
    std::optional<Priority> priority;
    std::string category;
};

struct TaskStats {
    int total = 0;
    int open = 0;
    int completed = 0;
    int overdue = 0;
    int dueToday = 0;
    int high = 0;
    int normal = 0;
    int low = 0;

    int completionRate() const {
        return total == 0 ? 0 : completed * 100 / total;
    }
};

class TaskService {
public:
    explicit TaskService(TaskRepository& repository);

    Task add(const std::string& title,
             Priority priority,
             const std::string& category,
             const std::string& dueDate);
    Task get(int id);
    Task update(int id,
                const std::string& title,
                Priority priority,
                const std::string& category,
                const std::string& dueDate);
    Task toggleDone(int id);
    bool remove(int id);
    int clearCompleted();

    std::vector<Task> list(const TaskQuery& query = {});
    TaskStats stats();

private:
    TaskRepository& repository_;

    static std::string normalizeTitle(const std::string& title);
    static std::string normalizeCategory(const std::string& category);
    static std::string normalizeDueDate(const std::string& dueDate);
};

}  // namespace taskman
