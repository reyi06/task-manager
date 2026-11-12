public record TaskStats(
        int total,
        int open,
        int completed,
        int overdue,
        int dueToday,
        int high,
        int normal,
        int low) {

    public int completionRate() {
        return total == 0 ? 0 : completed * 100 / total;
    }
}
