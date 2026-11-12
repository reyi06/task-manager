public class TaskNotFoundException extends RuntimeException {
    public TaskNotFoundException(int id) {
        super("Task #" + id + " was not found.");
    }
}
