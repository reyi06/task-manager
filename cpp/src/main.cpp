#include "taskman/ConsoleUI.hpp"
#include "taskman/Database.hpp"
#include "taskman/TaskRepository.hpp"
#include "taskman/TaskService.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

std::string databasePath() {
    const char* fromEnvironment = std::getenv("TASKMAN_DB");
    if (fromEnvironment != nullptr && *fromEnvironment != '\0') {
        return fromEnvironment;
    }
    return "data/tasks.db";
}

}  // namespace

int main() {
    try {
        const std::string path = databasePath();
        const std::filesystem::path parent = std::filesystem::path(path).parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }

        taskman::Database database(path);
        database.initializeSchema();

        taskman::TaskRepository repository(database);
        taskman::TaskService service(repository);
        taskman::ConsoleUI ui(service, "C++");
        ui.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << "\n";
        return 1;
    }
}
