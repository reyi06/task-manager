# Task Manager: C++ and Java

Two terminal task managers that provide the same features and use the same
SQLite database:

```text
data/tasks.db
```

Tasks added in the C++ version can be viewed, edited, completed, or deleted in
the Java version, and vice versa.

Both programs are required to behave the same: same menu, same layout, same
messages, same task order, same validation. That requirement is written down in
[`docs/architecture.md`](docs/architecture.md) and checked by the test suite,
which compares the screens of the two programs line by line.

## Documentation

| Document | Contents |
|---|---|
| [`docs/architecture.md`](docs/architecture.md) | Layers, class map for both languages, data model, flows, the parity contract, decisions, known limits |
| [`docs/testing.md`](docs/testing.md) | Test cases and the last recorded run |
| [`docs/MTE Project Report - Midterm.pdf`](docs/) | Course report |

## Features

- Add, view, edit, and delete tasks
- Mark tasks complete or reopen them
- Set a priority: low, normal, or high
- Set an optional due date and see overdue tasks
- Group tasks by category
- Search by keyword and filter by status, priority, or category
- Show task statistics
- Clear all completed tasks
- Validate input and handle database errors without crashing

## ASCII interface

```text
+--------------------------------------------------------------------------------------+
|  TASK MANAGER                                                          C++ / SQLite  |
|  A terminal task manager for keeping track of what matters.                          |
+--------------------------------------------------------------------------------------+

  +------------------+  +------------------+  +------------------+  +------------------+
  | OPEN             |  | COMPLETED        |  | OVERDUE          |  | DUE TODAY        |
  | 3                |  | 1                |  | 1                |  | 0                |
  +------------------+  +------------------+  +------------------+  +------------------+

+--------------------------+  +--------------------------+  +--------------------------+
| [1] Add task             |  | [4] Toggle complete      |  | [7] Statistics           |
| [2] List tasks           |  | [5] Delete task          |  | [8] Clear completed      |
| [3] View / edit task     |  | [6] Search / filter      |  | [9] Quit                 |
+--------------------------+  +--------------------------+  +--------------------------+

  TASKS  3 tasks
+----+-----+--------+------------+-------------+---------------------------------------+
| ID | ST  | PRI    | DUE        | CATEGORY    | TITLE                                 |
+----+-----+--------+------------+-------------+---------------------------------------+
| 3  | [ ] | 2 NORM | !10-20     | School      | Read chapter                          |
| 1  | [x] | 3 HIGH | 11-01      | School      | Finish project                        |
| 2  | [ ] | 1 LOW  | -          | Personal    | Buy milk                              |
+----+-----+--------+------------+-------------+---------------------------------------+

  --------------------------------------------------------------------------------------
  3 tasks   2 open   1 completed   1 overdue                              33% complete
```

`[ ]` means open, `[x]` means complete, and `!` marks an overdue open task.

Priorities use `1 = Low`, `2 = Normal`, and `3 = High`. Higher priorities sort
first. When you choose edit, toggle, or delete, the task list is shown before
the ID prompt.

Due dates are entered as `MM-DD`; the current year is assumed. A full
`YYYY-MM-DD` date is still accepted when a task needs a different year.

The interface uses only bold and dim text, plus red for overdue tasks and error
messages. Color and screen clearing are disabled automatically when output is
piped, or when `NO_COLOR` is set.

## Database

Both programs use the same schema:

```sql
CREATE TABLE IF NOT EXISTS tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    priority INTEGER NOT NULL DEFAULT 2,
    category TEXT NOT NULL DEFAULT 'General',
    due_date TEXT NOT NULL DEFAULT '',
    done INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL DEFAULT '',
    completed_at TEXT NOT NULL DEFAULT ''
);
```

Priority values are `1 = low`, `2 = normal`, and `3 = high`. Dates use
`YYYY-MM-DD` in storage. The form accepts `MM-DD` and assumes the current year;
full dates are still supported. Timestamps use UTC ISO 8601.

The C++ and Java programs both check for missing columns on startup and migrate
older versions of the table automatically.

## Project structure

```text
task-manager/
  cpp/
    include/taskman/
      Task.hpp
      Database.hpp
      TaskRepository.hpp
      TaskService.hpp
      ConsoleUI.hpp
      Terminal.hpp
    src/
      Task.cpp
      Database.cpp
      TaskRepository.cpp
      TaskService.cpp
      ConsoleUI.cpp
      Terminal.cpp
      main.cpp
    third_party/sqlite/
  java/
    Main.java
    ConsoleUI.java
    Terminal.java
    Database.java
    Task.java
    Priority.java
    TaskQuery.java
    TaskRepository.java
    TaskService.java
    TaskStats.java
    TaskNotFoundException.java
    StorageException.java
    lib/
  scripts/
  data/tasks.db
```

The code is split into four layers:

- **Model:** `Task` and `Priority`
- **Storage:** `Database` and `TaskRepository`
- **Logic:** `TaskService`
- **Interface:** `ConsoleUI`, `Terminal`, and `main`

The database layer uses prepared statements for every insert, update, delete,
and lookup.

## Build and run

From VS Code:

- `Ctrl+Shift+B` - build both programs
- `Tasks: Run Task` -> `Run C++`
- `Tasks: Run Task` -> `Run Java`
- `Tasks: Run Task` -> `Test System`
- `F5` -> choose `C++: Debug` or `Java: Debug`

From PowerShell:

```powershell
.\scripts\build-cpp.ps1
.\scripts\run-cpp.ps1

.\scripts\build-java.ps1
.\scripts\run-java.ps1
```

Both run scripts set `TASKMAN_DB` to the absolute path of `data/tasks.db`, so
they use the same file even when launched from a different directory. If
`TASKMAN_DB` is not set, the programs fall back to `data/tasks.db`.

## Resetting the data

Close both programs, then delete:

```text
data/tasks.db
```

The database and the table are recreated on the next run.

## Tests

Run the automated cross-language test suite:

```powershell
.\scripts\test-system.ps1
```

The test cases and recorded results are in
[`docs/testing.md`](docs/testing.md).

The suite ends with a full-session comparison: the same read-only session is
fed to both programs against the same database, and the two screens must match
line for line. Only the edition label in the header is masked.

## Releases

Prebuilt packages are attached to the GitHub releases. The download contains
the C++ binary, the compiled Java classes, the SQLite driver, a demo database,
two launchers, and the user documentation; the source stays in this repository.

## Toolchain

- C++: WinLibs GCC 16.2.0 / GDB 18.1
- Java: Temurin JDK 21
- C++ SQLite: amalgamation 3.54.1 in `cpp/third_party/sqlite`
- Java SQLite: `sqlite-jdbc` 3.53.4.0 in `java/lib`
