import java.time.Instant;
import java.time.LocalDate;
import java.time.format.DateTimeParseException;
import java.time.temporal.ChronoUnit;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;
import java.util.Optional;

public class TaskService {
    private final TaskRepository repository;

    public TaskService(TaskRepository repository) {
        this.repository = repository;
    }

    public Task add(String title, Priority priority, String category, String dueDate) {
        Task task = new Task(
                0,
                normalizeTitle(title),
                priority,
                normalizeCategory(category),
                normalizeDueDate(dueDate),
                false,
                Instant.now().truncatedTo(ChronoUnit.SECONDS).toString(),
                "");
        int id = repository.insert(task);
        return new Task(id, task.title(), task.priority(), task.category(),
                task.dueDate(), task.done(), task.createdAt(), task.completedAt());
    }

    public Task get(int id) {
        Optional<Task> task = repository.findById(id);
        if (task.isEmpty()) {
            throw new TaskNotFoundException(id);
        }
        return task.get();
    }

    public Task update(int id,
                       String title,
                       Priority priority,
                       String category,
                       String dueDate) {
        Task current = get(id);
        Task updated = new Task(
                current.id(),
                normalizeTitle(title),
                priority,
                normalizeCategory(category),
                normalizeDueDate(dueDate),
                current.done(),
                current.createdAt(),
                current.completedAt());
        repository.update(updated);
        return updated;
    }

    public Task toggleDone(int id) {
        Task current = get(id);
        boolean done = !current.done();
        Task updated = new Task(
                current.id(),
                current.title(),
                current.priority(),
                current.category(),
                current.dueDate(),
                done,
                current.createdAt(),
                done ? Instant.now().truncatedTo(ChronoUnit.SECONDS).toString() : "");
        repository.update(updated);
        return updated;
    }

    public void delete(int id) {
        if (!repository.delete(id)) {
            throw new TaskNotFoundException(id);
        }
    }

    public int clearCompleted() {
        return repository.clearCompleted();
    }

    public List<Task> list(TaskQuery query) {
        String keyword = query.keyword() == null
                ? "" : query.keyword().trim().toLowerCase(Locale.ROOT);
        String category = query.category() == null
                ? "" : query.category().trim();

        Comparator<Task> order = Comparator
                .comparing((Task task) -> task.done())
                .thenComparing(task -> task.dueDate().isEmpty())
                .thenComparing(Task::dueDate)
                .thenComparing(Task::priority, Comparator.reverseOrder())
                .thenComparingInt(Task::id);

        return repository.findAll().stream()
                .filter(task -> query.done() == null || task.done() == query.done())
                .filter(task -> query.priority() == null || task.priority() == query.priority())
                .filter(task -> category.isEmpty() || task.category().equalsIgnoreCase(category))
                .filter(task -> keyword.isEmpty()
                        || task.title().toLowerCase(Locale.ROOT).contains(keyword)
                        || task.category().toLowerCase(Locale.ROOT).contains(keyword))
                .sorted(order)
                .toList();
    }

    public TaskStats stats() {
        List<Task> tasks = repository.findAll();
        String today = LocalDate.now().toString();
        int open = 0;
        int completed = 0;
        int overdue = 0;
        int dueToday = 0;
        int high = 0;
        int normal = 0;
        int low = 0;

        for (Task task : tasks) {
            if (task.done()) {
                completed++;
            } else {
                open++;
            }
            if (task.isOverdue()) {
                overdue++;
            }
            if (!task.done() && task.dueDate().equals(today)) {
                dueToday++;
            }
            switch (task.priority()) {
                case HIGH -> high++;
                case LOW -> low++;
                case NORMAL -> normal++;
            }
        }

        return new TaskStats(tasks.size(), open, completed, overdue, dueToday,
                high, normal, low);
    }

    private static String normalizeTitle(String title) {
        String value = title == null ? "" : title.trim();
        if (value.isEmpty()) {
            throw new IllegalArgumentException("Title cannot be empty.");
        }
        if (value.codePointCount(0, value.length()) > 200) {
            throw new IllegalArgumentException("Title must be 200 characters or fewer.");
        }
        return value;
    }

    private static String normalizeCategory(String category) {
        String value = category == null ? "" : category.trim();
        if (value.isEmpty()) {
            return "General";
        }
        if (value.codePointCount(0, value.length()) > 50) {
            throw new IllegalArgumentException("Category must be 50 characters or fewer.");
        }
        return value;
    }

    private static String normalizeDueDate(String dueDate) {
        String value = dueDate == null ? "" : dueDate.trim();
        if (value.isEmpty()) {
            return "";
        }

        String currentYear = Integer.toString(LocalDate.now().getYear());
        if (value.matches("\\d{2}-\\d{2}")) {
            value = currentYear + "-" + value;
        }

        if (!value.matches("\\d{4}-\\d{2}-\\d{2}")) {
            throw invalidDate(currentYear);
        }
        LocalDate parsed;
        try {
            parsed = LocalDate.parse(value);
        } catch (DateTimeParseException error) {
            throw invalidDate(currentYear);
        }
        // same floor as the C++ version; 0001-01-01 is not a task due date
        if (parsed.getYear() < 1900) {
            throw invalidDate(currentYear);
        }
        return value;
    }

    private static IllegalArgumentException invalidDate(String currentYear) {
        return new IllegalArgumentException(
                "Due date must be a valid date in MM-DD format (year "
                        + currentYear + ") or YYYY-MM-DD.");
    }
}
