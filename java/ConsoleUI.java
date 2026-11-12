import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.List;
import java.util.Locale;

public class ConsoleUI {
    private static final int WIDTH = 88;
    private static final int INNER = WIDTH - 2;
    private static final int METRIC_WIDTH = 20;
    private static final int MENU_WIDTH = 28;

    private static final String BOLD = "\u001b[1m";
    private static final String DIM = "\u001b[2m";
    private static final String RED = "\u001b[31m";

    private final TaskService service;
    private final Terminal terminal;
    private final BufferedReader reader;
    private final String edition;

    private String panel = "";
    private String status = "";
    private boolean statusError = false;

    public ConsoleUI(TaskService service, String edition) {
        this.service = service;
        this.terminal = new Terminal();
        this.reader = new BufferedReader(new InputStreamReader(System.in));
        this.edition = edition;
    }

    public void run() {
        while (true) {
            try {
                drawScreen();
            } catch (RuntimeException error) {
                System.err.println("Fatal error: " + error.getMessage());
                return;
            }

            try {
                String choice = prompt("  Select an option [1-9]: ");
                panel = "";
                status = "";
                statusError = false;
                if (!choice.equals("9") && terminal.ansi()) {
                    terminal.clear();
                    printHeader();
                }

                switch (choice) {
                    case "1" -> addTask();
                    case "2" -> listTasks();
                    case "3" -> viewEditTask();
                    case "4" -> toggleTask();
                    case "5" -> deleteTask();
                    case "6" -> searchTasks();
                    case "7" -> showStats();
                    case "8" -> clearCompleted();
                    case "9" -> {
                        terminal.clear();
                        System.out.println();
                        System.out.println("  Goodbye.");
                        System.out.println();
                        return;
                    }
                    default -> setStatus("Invalid option. Choose 1-9.", true);
                }
            } catch (InputClosed error) {
                System.out.println();
                return;
            } catch (RuntimeException error) {
                setStatus(error.getMessage(), true);
            }
        }
    }

    private void drawScreen() {
        terminal.clear();
        TaskStats stats = service.stats();

        printHeader();
        printDashboard(stats);
        printMenu();
        if (!panel.isEmpty()) {
            System.out.println();
            System.out.println(panel);
        }
        printFooter(stats);
        printStatus();
    }

    private void printHeader() {
        String border = "+" + repeat('-', INNER) + "+";
        System.out.println(terminal.dim(border));

        String title = "  " + terminal.bold("TASK MANAGER");
        String right = terminal.dim(edition + " / SQLite") + "  ";
        System.out.println(terminal.dim("|")
                + terminal.padRight(twoColumns(title, right, INNER), INNER)
                + terminal.dim("|"));

        String subtitle = "  " + terminal.dim(
                "A terminal task manager for keeping track of what matters.");
        System.out.println(terminal.dim("|")
                + terminal.padRight(subtitle, INNER)
                + terminal.dim("|"));
        System.out.println(terminal.dim(border));
    }

    private void printDashboard(TaskStats stats) {
        String[][] metrics = {
                {"OPEN", Integer.toString(stats.open()), BOLD},
                {"COMPLETED", Integer.toString(stats.completed()), BOLD},
                {"OVERDUE", Integer.toString(stats.overdue()),
                        stats.overdue() > 0 ? "\u001b[1;31m" : DIM},
                {"DUE TODAY", Integer.toString(stats.dueToday()), BOLD}
        };

        System.out.println();
        String[][] boxes = new String[4][];
        for (int index = 0; index < metrics.length; index++) {
            boxes[index] = metricBox(metrics[index][0], metrics[index][1], metrics[index][2]);
        }
        for (int line = 0; line < 4; line++) {
            System.out.println("  "
                    + boxes[0][line] + "  "
                    + boxes[1][line] + "  "
                    + boxes[2][line] + "  "
                    + boxes[3][line]);
        }
    }

    private String[] metricBox(String label, String value, String color) {
        String top = terminal.dim("+" + repeat('-', METRIC_WIDTH - 2) + "+");
        String labelLine = terminal.dim("|") + " "
                + terminal.padRight(terminal.dim(label), METRIC_WIDTH - 4)
                + " " + terminal.dim("|");
        String valueLine = terminal.dim("|") + " "
                + terminal.padRight(terminal.paint(value, color), METRIC_WIDTH - 4)
                + " " + terminal.dim("|");
        return new String[]{top, labelLine, valueLine, top};
    }

    private void printMenu() {
        String[] first = menuBox(new String[][]{
                {"1", "Add task"}, {"2", "List tasks"}, {"3", "View / edit task"}
        }, false);
        String[] second = menuBox(new String[][]{
                {"4", "Toggle complete"}, {"5", "Delete task"}, {"6", "Search / filter"}
        }, false);
        String[] third = menuBox(new String[][]{
                {"7", "Statistics"}, {"8", "Clear completed"}, {"9", "Quit"}
        }, true);

        System.out.println();
        for (int line = 0; line < 5; line++) {
            System.out.println(first[line] + "  " + second[line] + "  " + third[line]);
        }
    }

    private String[] menuBox(String[][] items, boolean muted) {
        String[] lines = new String[5];
        String border = terminal.dim("+" + repeat('-', MENU_WIDTH - 2) + "+");
        lines[0] = border;
        lines[4] = border;

        for (int index = 0; index < items.length; index++) {
            String key = muted ? terminal.dim("[" + items[index][0] + "]")
                    : terminal.bold("[" + items[index][0] + "]");
            String label = muted ? terminal.dim(items[index][1]) : items[index][1];
            String text = key + " " + label;
            lines[index + 1] = terminal.dim("|") + " "
                    + terminal.padRight(text, MENU_WIDTH - 4)
                    + " " + terminal.dim("|");
        }
        return lines;
    }

    private void printFooter(TaskStats stats) {
        String left = "  " + terminal.bold(Integer.toString(stats.total())) + " tasks   "
                + terminal.bold(Integer.toString(stats.open())) + " open   "
                + terminal.bold(Integer.toString(stats.completed())) + " completed   "
                + (stats.overdue() > 0 ? terminal.red(Integer.toString(stats.overdue()))
                        : terminal.bold(Integer.toString(stats.overdue())))
                + " overdue";
        String right = terminal.bold(stats.completionRate() + "%") + " complete  ";

        System.out.println();
        System.out.println(terminal.dim(repeat('-', WIDTH)));
        System.out.println(twoColumns(left, right, WIDTH));
    }

    private void printStatus() {
        System.out.println();
        if (status.isEmpty()) {
            System.out.println(terminal.dim("  Ready. Choose an action above."));
            return;
        }

        String text = truncate(status, 82);
        System.out.println(statusError
                ? terminal.red("  ! " + text)
                : terminal.bold("  > " + text));
    }

    private String buildTable(List<Task> tasks, String heading) {
        StringBuilder out = new StringBuilder();
        out.append("  ").append(terminal.bold(heading)).append("  ")
                .append(terminal.dim(taskCount(tasks.size()))).append("\n");

        if (tasks.isEmpty()) {
            out.append("  ").append(terminal.dim("No tasks found.")).append("\n");
            return out.toString();
        }

        String top = "+" + repeat('-', 4) + "+" + repeat('-', 5) + "+"
                + repeat('-', 8) + "+" + repeat('-', 12) + "+"
                + repeat('-', 13) + "+" + repeat('-', 39) + "+";

        String header = "|" + terminal.padRight(" ID", 4)
                + "|" + terminal.padRight(" ST", 5)
                + "|" + terminal.padRight(" PRI", 8)
                + "|" + terminal.padRight(" DUE", 12)
                + "|" + terminal.padRight(" CATEGORY", 13)
                + "|" + terminal.padRight(" TITLE", 39) + "|";

        out.append(terminal.dim(top)).append("\n");
        out.append(terminal.bold(header)).append("\n");
        out.append(terminal.dim(top)).append("\n");

        for (Task task : tasks) {
            String statusText = task.done() ? terminal.dim(" [x]") : " [ ]";
            String priorityText = " " + task.priority().code() + " "
                    + task.priority().shortName();
            String dueText = task.isOverdue()
                    ? terminal.red(" " + task.dueLabel())
                    : " " + task.dueLabel();
            String categoryText = " " + truncate(task.category(), 12);
            String titleText = task.done()
                    ? terminal.dim(" " + truncate(task.title(), 38))
                    : " " + truncate(task.title(), 38);

            String idCell = terminal.padRight(" " + task.id(), 4);
            String statusCell = terminal.padRight(statusText, 5);
            String priorityCell = terminal.padRight(priorityText, 8);
            String dueCell = terminal.padRight(dueText, 12);
            String categoryCell = terminal.padRight(categoryText, 13);
            String titleCell = terminal.padRight(titleText, 39);

            out.append(terminal.dim("|")).append(idCell)
                    .append(terminal.dim("|")).append(statusCell)
                    .append(terminal.dim("|")).append(priorityCell)
                    .append(terminal.dim("|")).append(dueCell)
                    .append(terminal.dim("|")).append(categoryCell)
                    .append(terminal.dim("|")).append(titleCell)
                    .append(terminal.dim("|")).append("\n");
        }

        out.append(terminal.dim(top)).append("\n");
        return out.toString();
    }

    private String buildStats(TaskStats stats) {
        int cardWidth = 50;
        String border = "+" + repeat('-', cardWidth - 2) + "+";

        StringBuilder out = new StringBuilder();
        out.append("  ").append(terminal.dim(border)).append("\n");
        out.append("  ").append(terminal.dim("|"))
                .append(terminal.padRight(" " + terminal.bold("TASK STATISTICS"),
                        cardWidth - 2))
                .append(terminal.dim("|")).append("\n");
        out.append("  ").append(terminal.dim(border)).append("\n");
        out.append("  ").append(statRow("Total tasks", Integer.toString(stats.total()),
                BOLD, cardWidth)).append("\n");
        out.append("  ").append(statRow("Open", Integer.toString(stats.open()),
                BOLD, cardWidth)).append("\n");
        out.append("  ").append(statRow("Completed", Integer.toString(stats.completed()),
                BOLD, cardWidth)).append("\n");
        out.append("  ").append(statRow("Overdue", Integer.toString(stats.overdue()),
                stats.overdue() > 0 ? "\u001b[1;31m" : DIM, cardWidth)).append("\n");
        out.append("  ").append(statRow("Due today", Integer.toString(stats.dueToday()),
                BOLD, cardWidth)).append("\n");
        out.append("  ").append(statRow("Completion", stats.completionRate() + "%",
                BOLD, cardWidth)).append("\n");
        out.append("  ").append(terminal.dim(border)).append("\n");

        String priorityLine = " " + terminal.dim("HIGH") + " "
                + terminal.bold(Integer.toString(stats.high())) + "     "
                + terminal.dim("NORMAL") + " "
                + terminal.bold(Integer.toString(stats.normal())) + "     "
                + terminal.dim("LOW") + " "
                + terminal.bold(Integer.toString(stats.low()));
        out.append("  ").append(terminal.dim("|"))
                .append(terminal.padRight(priorityLine, cardWidth - 2))
                .append(terminal.dim("|")).append("\n");
        out.append("  ").append(terminal.dim(border)).append("\n");
        return out.toString();
    }

    private String statRow(String label, String value, String color, int cardWidth) {
        String content = " " + terminal.padRight(terminal.dim(label), 18)
                + terminal.padRight(terminal.paint(value, color), 10);
        return terminal.dim("|") + terminal.padRight(content, cardWidth - 2)
                + terminal.dim("|");
    }

    private String buildDetails(Task task) {
        String border = "+" + repeat('-', INNER) + "+";

        String left = "  " + terminal.bold("TASK #" + task.id());
        String right = terminal.dim(task.done() ? "[COMPLETE]" : "[OPEN]") + "  ";
        String due = task.displayDue();
        if (task.isOverdue()) {
            due += " (overdue)";
        }

        StringBuilder out = new StringBuilder();
        out.append(terminal.dim(border)).append("\n");
        out.append(detailLine(twoColumns(left, right, INNER))).append("\n");
        out.append(terminal.dim(border)).append("\n");
        out.append(field("Title:", truncate(task.title(), 68), BOLD)).append("\n");
        out.append(field("Status:", task.done() ? "Complete" : "Open", BOLD)).append("\n");
        out.append(field("Priority:", task.priority().displayName()
                + " (" + task.priority().code() + ")", BOLD)).append("\n");
        out.append(field("Category:", truncate(task.category(), 68), BOLD)).append("\n");
        out.append(field("Due date:", due, task.isOverdue() ? RED : BOLD)).append("\n");
        out.append(field("Created:", task.createdAt().isEmpty() ? "-" : task.createdAt(),
                DIM)).append("\n");
        out.append(field("Completed:",
                task.completedAt().isEmpty() ? "-" : task.completedAt(), DIM)).append("\n");
        out.append(terminal.dim(border)).append("\n");
        return out.toString();
    }

    private String detailLine(String content) {
        return terminal.dim("|") + terminal.padRight(content, INNER)
                + terminal.dim("|");
    }

    private String field(String label, String value, String color) {
        String content = "  " + terminal.dim(terminal.padRight(label, 12))
                + terminal.paint(value, color);
        return detailLine(content);
    }

    private String prompt(String label) {
        System.out.print(label);
        System.out.flush();
        try {
            String line = reader.readLine();
            if (line == null) {
                throw new InputClosed();
            }
            return line.trim();
        } catch (IOException error) {
            throw new StorageException("Could not read input.", error);
        }
    }

    private int promptInt(String label) {
        String text = prompt(label);
        try {
            return Integer.parseInt(text);
        } catch (NumberFormatException error) {
            throw new IllegalArgumentException("Please enter a valid task ID.");
        }
    }

    private Integer promptTaskId(String action) {
        terminal.clear();
        printHeader();

        List<Task> tasks = service.list(TaskQuery.all());
        System.out.println();
        System.out.println(buildTable(tasks, "SELECT A TASK"));

        if (tasks.isEmpty()) {
            setStatus("No tasks available.");
            return null;
        }

        int id = promptInt("  Task ID to " + action + " (0 to cancel): ");
        if (id == 0) {
            setStatus("Cancelled.");
            return null;
        }
        return id;
    }

    private boolean confirm(String question) {
        String answer = prompt(question).toLowerCase(Locale.ROOT);
        return answer.equals("y") || answer.equals("yes");
    }

    private Priority promptPriority(Priority current) {
        String text = prompt("  Priority [1=Low, 2=Normal, 3=High] ("
                + current.displayName() + "): ");
        if (text.isEmpty()) {
            return current;
        }
        return Priority.parse(text);
    }

    private void addTask() {
        String title = prompt("  Title: ");
        if (title.isEmpty()) {
            throw new IllegalArgumentException("Title cannot be empty.");
        }

        Priority priority = promptPriority(Priority.NORMAL);
        String category = prompt("  Category (General): ");
        if (category.isEmpty()) {
            category = "General";
        }
        String dueDate = prompt("  Due date (MM-DD, blank for none): ");

        Task task = service.add(title, priority, category, dueDate);
        panel = buildDetails(task);
        setStatus("Added task #" + task.id() + ".");
    }

    private void listTasks() {
        List<Task> tasks = service.list(TaskQuery.all());
        panel = buildTable(tasks, "TASKS");
        setStatus("Showing " + taskCount(tasks.size()) + ".");
    }

    private void viewEditTask() {
        Integer selected = promptTaskId("edit");
        if (selected == null) {
            return;
        }
        int id = selected;
        Task task = service.get(id);

        terminal.clear();
        printHeader();
        System.out.println();
        System.out.println(buildDetails(task));

        if (!confirm("  Edit this task? [y/N]: ")) {
            panel = buildDetails(task);
            setStatus("No changes made.");
            return;
        }

        String title = prompt("  New title (blank to keep) [" + task.title() + "]: ");
        if (title.isEmpty()) {
            title = task.title();
        }

        Priority priority = promptPriority(task.priority());

        String category = prompt("  New category (blank to keep) ["
                + task.category() + "]: ");
        if (category.isEmpty()) {
            category = task.category();
        }

        String dueDefault = task.dueDate().isEmpty() ? "none" : task.displayDue();
        String dueDate = prompt("  New due date (MM-DD, blank to keep, 'none' to clear) ["
                + dueDefault + "]: ");
        if (dueDate.isEmpty()) {
            dueDate = task.dueDate();
        } else if (dueDate.equalsIgnoreCase("none")) {
            dueDate = "";
        }

        task = service.update(id, title, priority, category, dueDate);
        panel = buildDetails(task);
        setStatus("Updated task #" + task.id() + ".");
    }

    private void toggleTask() {
        Integer selected = promptTaskId("toggle");
        if (selected == null) {
            return;
        }
        int id = selected;
        Task task = service.toggleDone(id);
        panel = buildDetails(task);
        setStatus("Task #" + task.id() + " is now "
                + (task.done() ? "complete." : "open."));
    }

    private void deleteTask() {
        Integer selected = promptTaskId("delete");
        if (selected == null) {
            return;
        }
        int id = selected;
        Task task = service.get(id);

        terminal.clear();
        printHeader();
        System.out.println();
        System.out.println(buildDetails(task));

        if (!confirm("  Delete this task? [y/N]: ")) {
            panel = buildDetails(task);
            setStatus("Delete cancelled.");
            return;
        }

        service.delete(id);
        panel = "";
        setStatus("Deleted task #" + id + ".");
    }

    private void searchTasks() {
        String keyword = prompt("  Keyword (blank for any): ");

        String statusText = prompt("  Status [all/open/done] (all): ")
                .toLowerCase(Locale.ROOT);
        Boolean done = switch (statusText) {
            case "", "all" -> null;
            case "open" -> false;
            case "done" -> true;
            default -> throw new IllegalArgumentException(
                    "Status must be all, open, or done.");
        };

        String priorityText = prompt("  Priority [all/high/normal/low] (all): ")
                .toLowerCase(Locale.ROOT);
        Priority priority;
        if (priorityText.isEmpty() || priorityText.equals("all")) {
            priority = null;
        } else {
            try {
                priority = Priority.parse(priorityText);
            } catch (IllegalArgumentException error) {
                // the search screen names its own options, same text as the C++ version
                throw new IllegalArgumentException(
                        "Priority must be all, high, normal, or low.");
            }
        }

        String category = prompt("  Category (blank for any): ");

        List<Task> tasks = service.list(new TaskQuery(keyword, done, priority, category));

        String statusLabel = done == null ? "All" : done ? "Done" : "Open";
        String priorityLabel = priority == null ? "Any" : priority.displayName();
        String criteria = "  " + terminal.dim("Keyword: ")
                + (keyword.isEmpty() ? "any" : keyword)
                + "   " + terminal.dim("Status: ") + statusLabel
                + "   " + terminal.dim("Priority: ") + priorityLabel
                + "   " + terminal.dim("Category: ")
                + (category.isEmpty() ? "any" : category);

        panel = criteria + "\n" + buildTable(tasks, "SEARCH RESULTS");
        setStatus("Search returned " + taskCount(tasks.size()) + ".");
    }

    private void showStats() {
        panel = buildStats(service.stats());
        setStatus("Statistics updated.");
    }

    private void clearCompleted() {
        List<Task> completed = service.list(new TaskQuery("", true, null, ""));
        if (completed.isEmpty()) {
            setStatus("No completed tasks to clear.");
            return;
        }

        if (!confirm("  Clear " + completed.size() + " completed task(s)? [y/N]: ")) {
            setStatus("Clear cancelled.");
            return;
        }

        int removed = service.clearCompleted();
        panel = "";
        setStatus("Cleared " + removed + " completed task(s).");
    }

    private void setStatus(String message, boolean error) {
        status = message;
        statusError = error;
    }

    private void setStatus(String message) {
        setStatus(message, false);
    }

    private static String taskCount(int count) {
        return count + (count == 1 ? " task" : " tasks");
    }

    private static String repeat(char character, int count) {
        return String.valueOf(character).repeat(count);
    }

    private static String twoColumns(String left, String right, int width) {
        int visible = Terminal.visibleLength(left) + Terminal.visibleLength(right);
        int gap = visible < width ? width - visible : 1;
        return left + " ".repeat(gap) + right;
    }

    private static String truncate(String text, int width) {
        if (text.codePointCount(0, text.length()) <= width) {
            return text;
        }
        if (width <= 3) {
            return text.substring(0, text.offsetByCodePoints(0, width));
        }
        return text.substring(0, text.offsetByCodePoints(0, width - 3)) + "...";
    }

    private static final class InputClosed extends RuntimeException {
    }
}
