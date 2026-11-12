import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public class TaskRepository {
    private static final String SELECT_COLUMNS =
            "SELECT id, title, priority, category, due_date, done, created_at, completed_at FROM tasks";

    private final Connection connection;

    public TaskRepository(Database database) {
        this.connection = database.connection();
    }

    public List<Task> findAll() {
        List<Task> tasks = new ArrayList<>();
        try (Statement statement = connection.createStatement();
             ResultSet result = statement.executeQuery(SELECT_COLUMNS + " ORDER BY id")) {
            while (result.next()) {
                tasks.add(readTask(result));
            }
        } catch (SQLException error) {
            throw new StorageException("Could not load tasks.", error);
        }
        return tasks;
    }

    public Optional<Task> findById(int id) {
        try (PreparedStatement statement =
                     connection.prepareStatement(SELECT_COLUMNS + " WHERE id = ?")) {
            statement.setInt(1, id);
            try (ResultSet result = statement.executeQuery()) {
                if (result.next()) {
                    return Optional.of(readTask(result));
                }
            }
        } catch (SQLException error) {
            throw new StorageException("Could not load task #" + id + ".", error);
        }
        return Optional.empty();
    }

    public int insert(Task task) {
        try (PreparedStatement statement = connection.prepareStatement(
                "INSERT INTO tasks "
                        + "(title, priority, category, due_date, done, created_at, completed_at) "
                        + "VALUES (?, ?, ?, ?, ?, ?, ?)")) {
            statement.setString(1, task.title());
            statement.setInt(2, task.priority().code());
            statement.setString(3, task.category());
            statement.setString(4, task.dueDate());
            statement.setInt(5, task.done() ? 1 : 0);
            statement.setString(6, task.createdAt());
            statement.setString(7, task.completedAt());
            statement.executeUpdate();
        } catch (SQLException error) {
            throw new StorageException("Could not add task.", error);
        }

        try (Statement statement = connection.createStatement();
             ResultSet result = statement.executeQuery("SELECT last_insert_rowid()")) {
            result.next();
            return result.getInt(1);
        } catch (SQLException error) {
            throw new StorageException("Could not read the new task ID.", error);
        }
    }

    public void update(Task task) {
        try (PreparedStatement statement = connection.prepareStatement(
                "UPDATE tasks SET title = ?, priority = ?, category = ?, "
                        + "due_date = ?, done = ?, completed_at = ? WHERE id = ?")) {
            statement.setString(1, task.title());
            statement.setInt(2, task.priority().code());
            statement.setString(3, task.category());
            statement.setString(4, task.dueDate());
            statement.setInt(5, task.done() ? 1 : 0);
            statement.setString(6, task.completedAt());
            statement.setInt(7, task.id());
            statement.executeUpdate();
        } catch (SQLException error) {
            throw new StorageException("Could not update task #" + task.id() + ".", error);
        }
    }

    public boolean delete(int id) {
        try (PreparedStatement statement =
                     connection.prepareStatement("DELETE FROM tasks WHERE id = ?")) {
            statement.setInt(1, id);
            return statement.executeUpdate() > 0;
        } catch (SQLException error) {
            throw new StorageException("Could not delete task #" + id + ".", error);
        }
    }

    public int clearCompleted() {
        try (Statement statement = connection.createStatement()) {
            return statement.executeUpdate("DELETE FROM tasks WHERE done = 1");
        } catch (SQLException error) {
            throw new StorageException("Could not clear completed tasks.", error);
        }
    }

    private static Task readTask(ResultSet result) throws SQLException {
        return new Task(
                result.getInt("id"),
                result.getString("title"),
                Priority.fromCode(result.getInt("priority")),
                result.getString("category"),
                result.getString("due_date"),
                result.getInt("done") != 0,
                result.getString("created_at"),
                result.getString("completed_at"));
    }
}
