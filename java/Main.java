import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class Main {
    public static void main(String[] args) {
        // The C++ version writes UTF-8 bytes; print the same bytes, so an accented
        // title looks identical in both programs.
        System.setOut(new PrintStream(
                new FileOutputStream(FileDescriptor.out), true, StandardCharsets.UTF_8));

        String path = System.getenv().getOrDefault("TASKMAN_DB", "data/tasks.db");
        Path databasePath = Path.of(path);

        try {
            if (databasePath.getParent() != null) {
                Files.createDirectories(databasePath.getParent());
            }

            try (Database database = new Database(path)) {
                TaskRepository repository = new TaskRepository(database);
                TaskService service = new TaskService(repository);
                new ConsoleUI(service, "Java").run();
            }
        } catch (Exception error) {
            System.err.println("Fatal error: " + error.getMessage());
            System.exit(1);
        }
    }
}
