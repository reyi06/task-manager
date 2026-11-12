#include "taskman/Task.hpp"

#include <cctype>
#include <ctime>
#include <stdexcept>

namespace taskman {

namespace {

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int year, int month) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return days[month - 1];
}

bool allDigits(const std::string& text) {
    for (char character : text) {
        if (!std::isdigit(static_cast<unsigned char>(character))) {
            return false;
        }
    }
    return true;
}

}  // namespace

std::string trim(const std::string& text) {
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string toLower(const std::string& text) {
    std::string result = text;
    for (char& character : result) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return result;
}

std::size_t textLength(const std::string& text) {
    std::size_t length = 0;
    for (char character : text) {
        if ((static_cast<unsigned char>(character) & 0xC0) != 0x80) {
            ++length;
        }
    }
    return length;
}

std::string truncateText(const std::string& text, std::size_t width) {
    if (textLength(text) <= width) {
        return text;
    }
    const std::string suffix = width <= 3 ? "" : "...";
    const std::size_t keep = width - suffix.size();

    std::size_t index = 0;
    std::size_t kept = 0;
    while (index < text.size() && kept < keep) {
        ++index;
        while (index < text.size() && (static_cast<unsigned char>(text[index]) & 0xC0) == 0x80) {
            ++index;
        }
        ++kept;
    }
    return text.substr(0, index) + suffix;
}

Priority priorityFromCode(int code) {
    switch (code) {
        case 1:
            return Priority::Low;
        case 3:
            return Priority::High;
        default:
            return Priority::Normal;
    }
}

std::optional<Priority> parsePriority(const std::string& text) {
    const std::string value = toLower(trim(text));
    if (value == "high" || value == "h" || value == "3") {
        return Priority::High;
    }
    if (value == "normal" || value == "n" || value == "2") {
        return Priority::Normal;
    }
    if (value == "low" || value == "l" || value == "1") {
        return Priority::Low;
    }
    return std::nullopt;
}

std::string priorityName(Priority priority) {
    switch (priority) {
        case Priority::High:
            return "High";
        case Priority::Low:
            return "Low";
        case Priority::Normal:
        default:
            return "Normal";
    }
}

std::string priorityShortName(Priority priority) {
    switch (priority) {
        case Priority::High:
            return "HIGH";
        case Priority::Low:
            return "LOW";
        case Priority::Normal:
        default:
            return "NORM";
    }
}

bool isValidDate(const std::string& text) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
        return false;
    }

    const std::string yearText = text.substr(0, 4);
    const std::string monthText = text.substr(5, 2);
    const std::string dayText = text.substr(8, 2);
    if (!allDigits(yearText) || !allDigits(monthText) || !allDigits(dayText)) {
        return false;
    }

    const int year = std::stoi(yearText);
    const int month = std::stoi(monthText);
    const int day = std::stoi(dayText);
    if (year < 1900 || year > 9999 || month < 1 || month > 12 || day < 1) {
        return false;
    }

    return day <= daysInMonth(year, month);
}

std::string todayString() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif

    char buffer[11]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &local);
    return buffer;
}

std::string nowTimestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif

    char buffer[21]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc);
    return buffer;
}

bool isOverdue(const std::string& dueDate) {
    return !dueDate.empty() && dueDate < todayString();
}

}  // namespace taskman
