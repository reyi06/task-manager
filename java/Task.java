import java.time.LocalDate;
import java.time.format.DateTimeParseException;

public record Task(
        int id,
        String title,
        Priority priority,
        String category,
        String dueDate,
        boolean done,
        String createdAt,
        String completedAt) {

    public boolean isOverdue() {
        if (done || dueDate.isEmpty()) {
            return false;
        }
        try {
            return LocalDate.parse(dueDate).isBefore(LocalDate.now());
        } catch (DateTimeParseException error) {
            // a date stored by something else is shown as-is, never crashes the screen
            return false;
        }
    }

    public String statusLabel() {
        return done ? "[x]" : "[ ]";
    }

    public String displayDue() {
        if (dueDate.isEmpty()) {
            return "-";
        }
        String currentYear = Integer.toString(LocalDate.now().getYear());
        if (dueDate.startsWith(currentYear + "-")) {
            return dueDate.substring(5);
        }
        return dueDate;
    }

    public String dueLabel() {
        if (dueDate.isEmpty()) {
            return "-";
        }
        return isOverdue() ? "!" + displayDue() : displayDue();
    }
}
