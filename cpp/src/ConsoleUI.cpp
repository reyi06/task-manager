#include "taskman/ConsoleUI.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace taskman {

namespace {

constexpr int kWidth = 88;
constexpr int kInner = kWidth - 2;
constexpr int kMetricWidth = 20;
constexpr int kMenuWidth = 28;

constexpr const char* kBold = "\x1b[1m";
constexpr const char* kDim = "\x1b[2m";
constexpr const char* kRed = "\x1b[31m";

struct InputClosed {};

std::string repeat(char character, int count) {
    return std::string(static_cast<std::size_t>(count), character);
}

std::string twoColumns(const std::string& left,
                       const std::string& right,
                       std::size_t width) {
    const std::size_t visible = Terminal::visibleLength(left) + Terminal::visibleLength(right);
    const std::size_t gap = visible < width ? width - visible : 1;
    return left + repeat(' ', static_cast<int>(gap)) + right;
}

std::string taskCount(int count) {
    return std::to_string(count) + (count == 1 ? " task" : " tasks");
}

}  // namespace

ConsoleUI::ConsoleUI(TaskService& service, std::string edition)
    : service_(service), edition_(std::move(edition)) {}

void ConsoleUI::setStatus(const std::string& message, bool error) {
    status_ = message;
    statusError_ = error;
}

void ConsoleUI::printHeader() {
    const std::string border = "+" + repeat('-', kInner) + "+";
    std::cout << terminal_.dim(border) << "\n";
    const std::string title = "  " + terminal_.bold("TASK MANAGER");
    const std::string edition = terminal_.dim(edition_ + " / SQLite") + "  ";
    std::cout << terminal_.dim("|")
              << terminal_.padRight(twoColumns(title, edition, kInner), kInner)
              << terminal_.dim("|") << "\n";

    const std::string subtitle = "  " + terminal_.dim(
            "A terminal task manager for keeping track of what matters.");
    std::cout << terminal_.dim("|")
              << terminal_.padRight(subtitle, kInner)
              << terminal_.dim("|") << "\n";
    std::cout << terminal_.dim(border) << "\n";
}

void ConsoleUI::printDashboard(const TaskStats& stats) {
    const auto metric = [&](const std::string& label, int value, const char* color) {
        const std::string top = terminal_.dim("+" + repeat('-', kMetricWidth - 2) + "+");
        const std::string labelLine = terminal_.dim("|")
                + " " + terminal_.padRight(terminal_.dim(label), kMetricWidth - 4)
                + " " + terminal_.dim("|");
        const std::string valueLine = terminal_.dim("|")
                + " " + terminal_.padRight(terminal_.paint(std::to_string(value), color),
                                           kMetricWidth - 4)
                + " " + terminal_.dim("|");
        return std::vector<std::string>{top, labelLine, valueLine, top};
    };

    const auto open = metric("OPEN", stats.open, kBold);
    const auto completed = metric("COMPLETED", stats.completed, kBold);
    const auto overdue = metric("OVERDUE", stats.overdue,
                                stats.overdue > 0 ? "\x1b[1;31m" : kDim);
    const auto dueToday = metric("DUE TODAY", stats.dueToday, kBold);

    std::cout << "\n";
    for (std::size_t index = 0; index < open.size(); ++index) {
        std::cout << "  "
                  << open[index] << "  "
                  << completed[index] << "  "
                  << overdue[index] << "  "
                  << dueToday[index] << "\n";
    }
}

void ConsoleUI::printMenu() {
    const auto menuBox = [&](const std::vector<std::pair<std::string, std::string>>& items,
                             bool muted) {
        std::vector<std::string> lines;
        lines.push_back(terminal_.dim("+" + repeat('-', kMenuWidth - 2) + "+"));
        for (const auto& [key, label] : items) {
            const std::string keyText = muted ? terminal_.dim("[" + key + "]")
                                              : terminal_.bold("[" + key + "]");
            const std::string labelText = muted ? terminal_.dim(label) : label;
            lines.push_back(terminal_.dim("|")
                            + " " + terminal_.padRight(keyText + " " + labelText,
                                                       kMenuWidth - 4)
                            + " " + terminal_.dim("|"));
        }
        lines.push_back(terminal_.dim("+" + repeat('-', kMenuWidth - 2) + "+"));
        return lines;
    };

    const auto first = menuBox({{"1", "Add task"}, {"2", "List tasks"},
                                {"3", "View / edit task"}}, false);
    const auto second = menuBox({{"4", "Toggle complete"}, {"5", "Delete task"},
                                 {"6", "Search / filter"}}, false);
    const auto third = menuBox({{"7", "Statistics"}, {"8", "Clear completed"},
                                {"9", "Quit"}}, true);

    std::cout << "\n";
    for (std::size_t index = 0; index < first.size(); ++index) {
        std::cout << first[index] << "  " << second[index] << "  " << third[index] << "\n";
    }
}

void ConsoleUI::printFooter(const TaskStats& stats) {
    const std::string left = "  " + terminal_.bold(std::to_string(stats.total)) + " tasks   "
            + terminal_.bold(std::to_string(stats.open)) + " open   "
            + terminal_.bold(std::to_string(stats.completed)) + " completed   "
            + (stats.overdue > 0 ? terminal_.red(std::to_string(stats.overdue))
                                 : terminal_.bold(std::to_string(stats.overdue)))
            + " overdue";
    const std::string right = terminal_.bold(std::to_string(stats.completionRate()) + "%")
            + " complete  ";

    std::cout << "\n" << terminal_.dim(repeat('-', kWidth)) << "\n";
    std::cout << twoColumns(left, right, kWidth) << "\n";
}

void ConsoleUI::printStatus() {
    std::cout << "\n";
    if (status_.empty()) {
        std::cout << terminal_.dim("  Ready. Choose an action above.") << "\n";
        return;
    }

    const std::string text = truncateText(status_, 82);
    std::cout << (statusError_ ? terminal_.red("  ! " + text)
                               : terminal_.bold("  > " + text))
              << "\n";
}

void ConsoleUI::drawScreen() {
    terminal_.clear();
    const TaskStats stats = service_.stats();

    printHeader();
    printDashboard(stats);
    printMenu();
    if (!panel_.empty()) {
        std::cout << "\n" << panel_ << "\n";
    }
    printFooter(stats);
    printStatus();
}

std::string ConsoleUI::buildTable(const std::vector<Task>& tasks,
                                  const std::string& heading) {
    std::ostringstream out;
    out << "  " << terminal_.bold(heading) << "  "
        << terminal_.dim(taskCount(static_cast<int>(tasks.size()))) << "\n";

    if (tasks.empty()) {
        out << "  " << terminal_.dim("No tasks found.") << "\n";
        return out.str();
    }

    const std::string top = "+" + repeat('-', 4) + "+" + repeat('-', 5) + "+"
            + repeat('-', 8) + "+" + repeat('-', 12) + "+" + repeat('-', 13) + "+"
            + repeat('-', 39) + "+";

    std::string header = "|" + terminal_.padRight(" ID", 4)
            + "|" + terminal_.padRight(" ST", 5)
            + "|" + terminal_.padRight(" PRI", 8)
            + "|" + terminal_.padRight(" DUE", 12)
            + "|" + terminal_.padRight(" CATEGORY", 13)
            + "|" + terminal_.padRight(" TITLE", 39) + "|";

    out << terminal_.dim(top) << "\n";
    out << terminal_.bold(header) << "\n";
    out << terminal_.dim(top) << "\n";

    for (const Task& task : tasks) {
        std::string due = displayDue(task.dueDate);
        const bool overdue = !task.done && isOverdue(task.dueDate);
        if (overdue) {
            due = "!" + due;
        }

        const std::string statusText = task.done ? terminal_.dim(" [x]") : " [ ]";
        const std::string priorityText = " " + std::to_string(static_cast<int>(task.priority))
                + " " + priorityShortName(task.priority);
        const std::string dueText = overdue ? terminal_.red(" " + due) : " " + due;
        const std::string categoryText = " " + truncateText(task.category, 12);
        const std::string titleText = task.done
                ? terminal_.dim(" " + truncateText(task.title, 38))
                : " " + truncateText(task.title, 38);

        const std::string idCell = terminal_.padRight(" " + std::to_string(task.id), 4);
        const std::string statusCell = terminal_.padRight(statusText, 5);
        const std::string priorityCell = terminal_.padRight(priorityText, 8);
        const std::string dueCell = terminal_.padRight(dueText, 12);
        const std::string categoryCell = terminal_.padRight(categoryText, 13);
        const std::string titleCell = terminal_.padRight(titleText, 39);

        out << terminal_.dim("|") << idCell
            << terminal_.dim("|") << statusCell
            << terminal_.dim("|") << priorityCell
            << terminal_.dim("|") << dueCell
            << terminal_.dim("|") << categoryCell
            << terminal_.dim("|") << titleCell
            << terminal_.dim("|") << "\n";
    }

    out << terminal_.dim(top) << "\n";
    return out.str();
}

std::string ConsoleUI::buildStats(const TaskStats& stats) {
    constexpr int cardWidth = 50;
    const std::string border = "+" + repeat('-', cardWidth - 2) + "+";

    const auto row = [&](const std::string& label, const std::string& value,
                         const char* color) {
        const std::string content = " " + terminal_.padRight(terminal_.dim(label), 18)
                + terminal_.padRight(terminal_.paint(value, color), 10);
        return terminal_.dim("|") + terminal_.padRight(content, cardWidth - 2)
                + terminal_.dim("|");
    };

    const std::string title = terminal_.dim("|")
            + terminal_.padRight(" " + terminal_.bold("TASK STATISTICS"), cardWidth - 2)
            + terminal_.dim("|");

    const std::string priorityLine = " " + terminal_.dim("HIGH") + " "
            + terminal_.bold(std::to_string(stats.high)) + "     "
            + terminal_.dim("NORMAL") + " "
            + terminal_.bold(std::to_string(stats.normal)) + "     "
            + terminal_.dim("LOW") + " "
            + terminal_.bold(std::to_string(stats.low));

    std::ostringstream out;
    out << "  " << terminal_.dim(border) << "\n";
    out << "  " << title << "\n";
    out << "  " << terminal_.dim(border) << "\n";
    out << "  " << row("Total tasks", std::to_string(stats.total), kBold) << "\n";
    out << "  " << row("Open", std::to_string(stats.open), kBold) << "\n";
    out << "  " << row("Completed", std::to_string(stats.completed), kBold) << "\n";
    out << "  " << row("Overdue", std::to_string(stats.overdue),
                       stats.overdue > 0 ? "\x1b[1;31m" : kDim) << "\n";
    out << "  " << row("Due today", std::to_string(stats.dueToday), kBold) << "\n";
    out << "  " << row("Completion", std::to_string(stats.completionRate()) + "%",
                       kBold) << "\n";
    out << "  " << terminal_.dim(border) << "\n";
    out << "  " << terminal_.dim("|") << terminal_.padRight(priorityLine, cardWidth - 2)
        << terminal_.dim("|") << "\n";
    out << "  " << terminal_.dim(border) << "\n";
    return out.str();
}

std::string ConsoleUI::buildDetails(const Task& task) {
    const std::string border = "+" + repeat('-', kInner) + "+";

    const auto line = [&](const std::string& content) {
        return terminal_.dim("|") + terminal_.padRight(content, kInner)
                + terminal_.dim("|");
    };

    const auto field = [&](const std::string& label, const std::string& value,
                           const char* color) {
        const std::string content = "  " + terminal_.dim(terminal_.padRight(label, 12))
                + terminal_.paint(value, color);
        return line(content);
    };

    const std::string left = "  " + terminal_.bold("TASK #" + std::to_string(task.id));
    const std::string right = terminal_.dim(task.done ? "[COMPLETE]" : "[OPEN]") + "  ";
    const std::string status = task.done ? "Complete" : "Open";

    std::string due = displayDue(task.dueDate);
    const bool overdue = !task.done && isOverdue(task.dueDate);
    if (overdue) {
        due += " (overdue)";
    }

    std::ostringstream out;
    out << terminal_.dim(border) << "\n";
    out << line(twoColumns(left, right, kInner)) << "\n";
    out << terminal_.dim(border) << "\n";
    out << field("Title:", truncateText(task.title, 68), kBold) << "\n";
    out << field("Status:", status, kBold) << "\n";
    out << field("Priority:", priorityName(task.priority) + " ("
                 + std::to_string(static_cast<int>(task.priority)) + ")", kBold) << "\n";
    out << field("Category:", truncateText(task.category, 68), kBold) << "\n";
    out << field("Due date:", due, overdue ? kRed : kBold) << "\n";
    out << field("Created:", task.createdAt.empty() ? "-" : task.createdAt, kDim) << "\n";
    out << field("Completed:", task.completedAt.empty() ? "-" : task.completedAt, kDim) << "\n";
    out << terminal_.dim(border) << "\n";
    return out.str();
}

std::string ConsoleUI::prompt(const std::string& label) const {
    std::cout << label;
    std::cout.flush();

    std::string line;
    if (!std::getline(std::cin, line)) {
        throw InputClosed{};
    }
    return trim(line);
}

int ConsoleUI::promptInt(const std::string& label) const {
    const std::string text = prompt(label);
    try {
        std::size_t used = 0;
        const int value = std::stoi(text, &used);
        if (used != text.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return value;
    } catch (const std::exception&) {
        throw std::invalid_argument("Please enter a valid task ID.");
    }
}

std::optional<int> ConsoleUI::promptTaskId(const std::string& action) {
    terminal_.clear();
    printHeader();

    const std::vector<Task> tasks = service_.list();
    std::cout << "\n" << buildTable(tasks, "SELECT A TASK") << "\n";

    if (tasks.empty()) {
        setStatus("No tasks available.");
        return std::nullopt;
    }

    const int id = promptInt("  Task ID to " + action + " (0 to cancel): ");
    if (id == 0) {
        setStatus("Cancelled.");
        return std::nullopt;
    }
    return id;
}

bool ConsoleUI::confirm(const std::string& question) const {
    const std::string answer = toLower(prompt(question));
    return answer == "y" || answer == "yes";
}

Priority ConsoleUI::promptPriority(Priority current) const {
    const std::string text = prompt(
        "  Priority [1=Low, 2=Normal, 3=High] (" + priorityName(current) + "): ");
    if (text.empty()) {
        return current;
    }

    const std::optional<Priority> priority = parsePriority(text);
    if (!priority.has_value()) {
        throw std::invalid_argument("Priority must be 1 (Low), 2 (Normal), or 3 (High).");
    }
    return *priority;
}

void ConsoleUI::addTask() {
    const std::string title = prompt("  Title: ");
    if (title.empty()) {
        throw std::invalid_argument("Title cannot be empty.");
    }

    const Priority priority = promptPriority(Priority::Normal);
    std::string category = prompt("  Category (General): ");
    if (category.empty()) {
        category = "General";
    }
    const std::string dueDate = prompt("  Due date (MM-DD, blank for none): ");

    const Task task = service_.add(title, priority, category, dueDate);
    panel_ = buildDetails(task);
    setStatus("Added task #" + std::to_string(task.id) + ".");
}

void ConsoleUI::listTasks() {
    const std::vector<Task> tasks = service_.list();
    panel_ = buildTable(tasks, "TASKS");
    setStatus("Showing " + taskCount(static_cast<int>(tasks.size())) + ".");
}

void ConsoleUI::viewEditTask() {
    const std::optional<int> selected = promptTaskId("edit");
    if (!selected.has_value()) {
        return;
    }
    const int id = *selected;
    Task task = service_.get(id);

    terminal_.clear();
    printHeader();
    std::cout << "\n" << buildDetails(task) << "\n";

    if (!confirm("  Edit this task? [y/N]: ")) {
        panel_ = buildDetails(task);
        setStatus("No changes made.");
        return;
    }

    std::string title = prompt("  New title (blank to keep) [" + task.title + "]: ");
    if (title.empty()) {
        title = task.title;
    }

    const Priority priority = promptPriority(task.priority);

    std::string category = prompt(
            "  New category (blank to keep) [" + task.category + "]: ");
    if (category.empty()) {
        category = task.category;
    }

    const std::string dueDefault = task.dueDate.empty() ? "none" : displayDue(task.dueDate);
    std::string dueDate = prompt(
            "  New due date (MM-DD, blank to keep, 'none' to clear) ["
            + dueDefault + "]: ");
    if (dueDate.empty()) {
        dueDate = task.dueDate;
    } else if (toLower(dueDate) == "none") {
        dueDate = "";
    }

    task = service_.update(id, title, priority, category, dueDate);
    panel_ = buildDetails(task);
    setStatus("Updated task #" + std::to_string(task.id) + ".");
}

void ConsoleUI::toggleTask() {
    const std::optional<int> selected = promptTaskId("toggle");
    if (!selected.has_value()) {
        return;
    }
    const int id = *selected;
    const Task task = service_.toggleDone(id);
    panel_ = buildDetails(task);
    setStatus("Task #" + std::to_string(task.id) + " is now "
              + (task.done ? "complete." : "open."));
}

void ConsoleUI::deleteTask() {
    const std::optional<int> selected = promptTaskId("delete");
    if (!selected.has_value()) {
        return;
    }
    const int id = *selected;
    const Task task = service_.get(id);

    terminal_.clear();
    printHeader();
    std::cout << "\n" << buildDetails(task) << "\n";

    if (!confirm("  Delete this task? [y/N]: ")) {
        panel_ = buildDetails(task);
        setStatus("Delete cancelled.");
        return;
    }

    service_.remove(id);
    panel_.clear();
    setStatus("Deleted task #" + std::to_string(id) + ".");
}

void ConsoleUI::searchTasks() {
    const std::string keyword = prompt("  Keyword (blank for any): ");

    const std::string status = toLower(prompt("  Status [all/open/done] (all): "));
    std::optional<bool> done;
    if (status.empty() || status == "all") {
        done = std::nullopt;
    } else if (status == "open") {
        done = false;
    } else if (status == "done") {
        done = true;
    } else {
        throw std::invalid_argument("Status must be all, open, or done.");
    }

    const std::string priorityText = toLower(prompt(
            "  Priority [all/high/normal/low] (all): "));
    std::optional<Priority> priority;
    if (priorityText.empty() || priorityText == "all") {
        priority = std::nullopt;
    } else {
        priority = parsePriority(priorityText);
        if (!priority.has_value()) {
            throw std::invalid_argument("Priority must be all, high, normal, or low.");
        }
    }

    const std::string category = prompt("  Category (blank for any): ");

    TaskQuery query;
    query.keyword = keyword;
    query.done = done;
    query.priority = priority;
    query.category = category;

    const std::vector<Task> tasks = service_.list(query);

    std::string statusText = "All";
    if (done.has_value()) {
        statusText = *done ? "Done" : "Open";
    }
    const std::string priorityLabel = priority.has_value()
            ? priorityName(*priority) : "Any";
    const std::string criteria = "  " + terminal_.dim("Keyword: ")
            + (keyword.empty() ? "any" : keyword)
            + "   " + terminal_.dim("Status: ") + statusText
            + "   " + terminal_.dim("Priority: ") + priorityLabel
            + "   " + terminal_.dim("Category: ")
            + (category.empty() ? "any" : category);

    panel_ = criteria + "\n" + buildTable(tasks, "SEARCH RESULTS");
    setStatus("Search returned " + taskCount(static_cast<int>(tasks.size())) + ".");
}

void ConsoleUI::showStats() {
    panel_ = buildStats(service_.stats());
    setStatus("Statistics updated.");
}

void ConsoleUI::clearCompleted() {
    TaskQuery query;
    query.done = true;
    const std::vector<Task> completed = service_.list(query);

    if (completed.empty()) {
        setStatus("No completed tasks to clear.");
        return;
    }

    if (!confirm("  Clear " + std::to_string(completed.size())
                 + " completed task(s)? [y/N]: ")) {
        setStatus("Clear cancelled.");
        return;
    }

    const int removed = service_.clearCompleted();
    panel_.clear();
    setStatus("Cleared " + std::to_string(removed) + " completed task(s).");
}

std::string ConsoleUI::displayDue(const std::string& dueDate) {
    if (dueDate.empty()) {
        return "-";
    }
    const std::string currentYear = todayString().substr(0, 4);
    if (dueDate.size() == 10 && dueDate.substr(0, 4) == currentYear) {
        return dueDate.substr(5);
    }
    return dueDate;
}

void ConsoleUI::run() {
    while (true) {
        try {
            drawScreen();
        } catch (const std::exception& error) {
            std::cerr << "Fatal error: " << error.what() << "\n";
            return;
        }

        try {
            const std::string choice = prompt("  Select an option [1-9]: ");
            panel_.clear();
            status_.clear();
            statusError_ = false;
            if (choice != "9" && terminal_.ansi()) {
                terminal_.clear();
                printHeader();
            }

            if (choice == "1") {
                addTask();
            } else if (choice == "2") {
                listTasks();
            } else if (choice == "3") {
                viewEditTask();
            } else if (choice == "4") {
                toggleTask();
            } else if (choice == "5") {
                deleteTask();
            } else if (choice == "6") {
                searchTasks();
            } else if (choice == "7") {
                showStats();
            } else if (choice == "8") {
                clearCompleted();
            } else if (choice == "9") {
                terminal_.clear();
                std::cout << "\n  Goodbye.\n\n";
                return;
            } else {
                setStatus("Invalid option. Choose 1-9.", true);
            }
        } catch (const InputClosed&) {
            std::cout << "\n";
            return;
        } catch (const std::exception& error) {
            setStatus(error.what(), true);
        }
    }
}

}  // namespace taskman
