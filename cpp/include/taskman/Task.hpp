#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace taskman {

enum class Priority {
    Low = 1,
    Normal = 2,
    High = 3,
};

Priority priorityFromCode(int code);
std::optional<Priority> parsePriority(const std::string& text);
std::string priorityName(Priority priority);
std::string priorityShortName(Priority priority);

std::string trim(const std::string& text);
std::string toLower(const std::string& text);

std::size_t textLength(const std::string& text);
std::string truncateText(const std::string& text, std::size_t width);

bool isValidDate(const std::string& text);
std::string todayString();
std::string nowTimestamp();
bool isOverdue(const std::string& dueDate);

struct Task {
    int id = 0;
    std::string title;
    Priority priority = Priority::Normal;
    std::string category = "General";
    std::string dueDate;
    bool done = false;
    std::string createdAt;
    std::string completedAt;
};

}  // namespace taskman
