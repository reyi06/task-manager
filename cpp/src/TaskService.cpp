#include "taskman/TaskService.hpp"

#include <algorithm>
#include <stdexcept>

namespace taskman {

TaskNotFound::TaskNotFound(int id)
    : std::runtime_error("Task #" + std::to_string(id) + " was not found.") {}

TaskService::TaskService(TaskRepository& repository) : repository_(repository) {}

std::string TaskService::normalizeTitle(const std::string& title) {
    const std::string value = trim(title);
    if (value.empty()) {
        throw std::invalid_argument("Title cannot be empty.");
    }
    if (textLength(value) > 200) {
        throw std::invalid_argument("Title must be 200 characters or fewer.");
    }
    return value;
}

std::string TaskService::normalizeCategory(const std::string& category) {
    const std::string value = trim(category);
    if (value.empty()) {
        return "General";
    }
    if (textLength(value) > 50) {
        throw std::invalid_argument("Category must be 50 characters or fewer.");
    }
    return value;
}

std::string TaskService::normalizeDueDate(const std::string& dueDate) {
    std::string value = trim(dueDate);
    if (value.empty()) {
        return "";
    }

    const std::string currentYear = todayString().substr(0, 4);
    if (value.size() == 5 && value[2] == '-') {
        value = currentYear + "-" + value;
    }

    if (!isValidDate(value)) {
        throw std::invalid_argument(
                "Due date must be a valid date in MM-DD format (year "
                + currentYear + ") or YYYY-MM-DD.");
    }
    return value;
}

Task TaskService::add(const std::string& title,
                      Priority priority,
                      const std::string& category,
                      const std::string& dueDate) {
    Task task;
    task.title = normalizeTitle(title);
    task.priority = priority;
    task.category = normalizeCategory(category);
    task.dueDate = normalizeDueDate(dueDate);
    task.done = false;
    task.createdAt = nowTimestamp();

    task.id = repository_.insert(task);
    return task;
}

Task TaskService::get(int id) {
    const std::optional<Task> task = repository_.findById(id);
    if (!task.has_value()) {
        throw TaskNotFound(id);
    }
    return *task;
}

Task TaskService::update(int id,
                         const std::string& title,
                         Priority priority,
                         const std::string& category,
                         const std::string& dueDate) {
    Task task = get(id);
    task.title = normalizeTitle(title);
    task.priority = priority;
    task.category = normalizeCategory(category);
    task.dueDate = normalizeDueDate(dueDate);
    repository_.update(task);
    return task;
}

Task TaskService::toggleDone(int id) {
    Task task = get(id);
    task.done = !task.done;
    task.completedAt = task.done ? nowTimestamp() : "";
    repository_.update(task);
    return task;
}

bool TaskService::remove(int id) {
    if (!repository_.remove(id)) {
        throw TaskNotFound(id);
    }
    return true;
}

int TaskService::clearCompleted() {
    return repository_.clearCompleted();
}

std::vector<Task> TaskService::list(const TaskQuery& query) {
    std::vector<Task> tasks = repository_.findAll();
    const std::string keyword = toLower(trim(query.keyword));
    const std::string category = toLower(trim(query.category));

    tasks.erase(
        std::remove_if(tasks.begin(), tasks.end(), [&](const Task& task) {
            if (query.done.has_value() && task.done != *query.done) {
                return true;
            }
            if (query.priority.has_value() && task.priority != *query.priority) {
                return true;
            }
            if (!category.empty() && toLower(task.category) != category) {
                return true;
            }
            if (!keyword.empty()
                && toLower(task.title).find(keyword) == std::string::npos
                && toLower(task.category).find(keyword) == std::string::npos) {
                return true;
            }
            return false;
        }),
        tasks.end());

    std::sort(tasks.begin(), tasks.end(), [](const Task& left, const Task& right) {
        if (left.done != right.done) {
            return !left.done;
        }

        const bool leftHasNoDueDate = left.dueDate.empty();
        const bool rightHasNoDueDate = right.dueDate.empty();
        if (leftHasNoDueDate != rightHasNoDueDate) {
            return !leftHasNoDueDate;
        }
        if (!leftHasNoDueDate && left.dueDate != right.dueDate) {
            return left.dueDate < right.dueDate;
        }
        if (left.priority != right.priority) {
            return static_cast<int>(left.priority) > static_cast<int>(right.priority);
        }
        return left.id < right.id;
    });

    return tasks;
}

TaskStats TaskService::stats() {
    TaskStats result;
    const std::string today = todayString();
    const std::vector<Task> tasks = repository_.findAll();

    result.total = static_cast<int>(tasks.size());
    for (const Task& task : tasks) {
        if (task.done) {
            ++result.completed;
        } else {
            ++result.open;
        }

        if (!task.done && isOverdue(task.dueDate)) {
            ++result.overdue;
        }
        if (!task.done && task.dueDate == today) {
            ++result.dueToday;
        }

        switch (task.priority) {
            case Priority::High:
                ++result.high;
                break;
            case Priority::Low:
                ++result.low;
                break;
            case Priority::Normal:
            default:
                ++result.normal;
                break;
        }
    }

    return result;
}

}  // namespace taskman
