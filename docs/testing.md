# Testing the Task Manager

## Test environment

- Windows 11
- C++: WinLibs GCC 16.2.0
- Java: Temurin JDK 21
- C++ SQLite: amalgamation 3.54.1
- Java SQLite: sqlite-jdbc 3.53.4.0

## Automated test suite

Run:

```powershell
.\scripts\test-system.ps1
```

The script builds both programs, uses a temporary database
(`data/test-suite.db`), runs the same scenarios through both versions, and
deletes the test database afterward. The real `data/tasks.db` is not touched.

The last check runs both programs over the same database and compares their
screens line for line. The only accepted difference is the edition label in the
header (`C++ / SQLite` versus `Java / SQLite`), which the script masks.

## Test cases and results

Last automated run: **2026-11-12** — all checks passed.

| # | Test case | Expected result | C++ | Java |
|---:|---|---|:---:|:---:|
| 1 | Build both programs | Both compile without errors | Pass | Pass |
| 2 | First run | Database and `tasks` table are created | Pass | Pass |
| 3 | Add task | Task appears with the next ID | Pass | Pass |
| 4 | Add empty title | `Title cannot be empty.` | Pass | Pass |
| 5 | Add MM-DD due date | `11-01` is accepted and stored with the current year | Pass | Pass |
| 6 | Add full due date | `2028-02-29` is accepted | Pass | Pass |
| 7 | Add invalid due date | `02-30` and `02-29` are rejected | Pass | Pass |
| 8 | List tasks | Open tasks appear before completed tasks; due dates then priority then ID decide the order | Pass | Pass |
| 9 | Edit task | Title, priority, category, and due date can be changed | Pass | Pass |
| 10 | Clear a due date | Entering `none` during edit removes the due date | Pass | Pass |
| 11 | Mark complete | Status becomes `[x]` and `completed_at` is set | Pass | Pass |
| 12 | Reopen task | Status returns to `[ ]` and `completed_at` is cleared | Pass | Pass |
| 13 | Delete and confirm | Task is removed | Pass | Pass |
| 14 | Delete and cancel | Task remains in the database | Pass | Pass |
| 15 | Search by keyword | Matching titles/categories are shown | Pass | Pass |
| 16 | Filter by status | Open-only or completed-only lists are correct | Pass | Pass |
| 17 | Filter by priority and category | Only matching tasks are shown | Pass | Pass |
| 18 | Statistics | Total, open, completed, overdue, due today, and priority counts are correct | Pass | Pass |
| 19 | Clear completed | Only completed tasks are deleted | Pass | Pass |
| 20 | Invalid task ID | `Task #999 was not found.` | Pass | Pass |
| 21 | Invalid menu option | Input is rejected without crashing | Pass | Pass |
| 22 | Shared database, C++ to Java | A task added in C++ is visible in Java | Pass | Pass |
| 23 | Shared database, Java to C++ | A task added or completed in Java is visible in C++ | Pass | Pass |
| 24 | Older database schema | Missing columns are added automatically | Pass | Pass |
| 25 | Priority codes | `1 = Low`, `2 = Normal`, `3 = High`; high sorts first | Pass | Pass |
| 26 | Task picker | Edit, toggle, and delete show the task list before asking for an ID | Pass | Pass |
| 27 | Identical screen | Both versions print the same screen for the same database, edition label aside | Pass | Pass |
| 28 | Accented text | A title with accents is stored and printed unchanged in both versions | Pass | Pass |

## Manual checks

These checks were also performed against temporary database files:

- An overdue task using a past due date shows `!` in the list and
  `(overdue)` in the detail view.
- The statistics screen correctly reports overdue tasks and the completion
  percentage.
- Closing and reopening each program keeps the data in SQLite.
- The `MM-DD` due-date entry stores the current year, while a full
  `YYYY-MM-DD` date is still accepted for another year.
- Test databases were removed after testing, and `data/tasks.db` was left empty.
