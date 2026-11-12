#pragma once

#include "taskman/TaskService.hpp"
#include "taskman/Terminal.hpp"

#include <optional>
#include <string>
#include <vector>

namespace taskman {

class ConsoleUI {
public:
    ConsoleUI(TaskService& service, std::string edition);

    void run();

private:
    TaskService& service_;
    Terminal terminal_;
    std::string edition_;
    std::string panel_;
    std::string status_;
    bool statusError_ = false;

    void drawScreen();
    void printHeader();
    void printDashboard(const TaskStats& stats);
    void printMenu();
    void printFooter(const TaskStats& stats);
    void printStatus();

    void addTask();
    void listTasks();
    void viewEditTask();
    void toggleTask();
    void deleteTask();
    void searchTasks();
    void showStats();
    void clearCompleted();

    std::string prompt(const std::string& label) const;
    int promptInt(const std::string& label) const;
    std::optional<int> promptTaskId(const std::string& action);
    bool confirm(const std::string& question) const;
    Priority promptPriority(Priority current) const;

    std::string buildTable(const std::vector<Task>& tasks, const std::string& heading);
    std::string buildStats(const TaskStats& stats);
    std::string buildDetails(const Task& task);

    void setStatus(const std::string& message, bool error = false);
    static std::string displayDue(const std::string& dueDate);
};

}  // namespace taskman
