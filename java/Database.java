import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.HashSet;
import java.util.Set;

public class Database implements AutoCloseable {
    private final Connection connection;

    public Database(String path) {
        try {
            Class.forName("org.sqlite.JDBC");
        } catch (ClassNotFoundException error) {
            throw new StorageException("SQLite driver was not found.", error);
        }

        Connection opened = null;
        try {
            opened = DriverManager.getConnection("jdbc:sqlite:" + path.replace('\\', '/'));
            try (Statement statement = opened.createStatement()) {
                statement.execute("PRAGMA busy_timeout = 5000");
            }
            initializeSchema(opened);
        } catch (SQLException error) {
            if (opened != null) {
                try {
                    opened.close();
                } catch (SQLException ignored) {
                }
            }
            throw new StorageException("Could not open database: " + error.getMessage(), error);
        }

        this.connection = opened;
    }

    public Connection connection() {
        return connection;
    }

    private static void initializeSchema(Connection connection) throws SQLException {
        try (Statement statement = connection.createStatement()) {
            statement.executeUpdate("""
                    CREATE TABLE IF NOT EXISTS tasks (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        title TEXT NOT NULL,
                        priority INTEGER NOT NULL DEFAULT 2,
                        category TEXT NOT NULL DEFAULT 'General',
                        due_date TEXT NOT NULL DEFAULT '',
                        done INTEGER NOT NULL DEFAULT 0,
                        created_at TEXT NOT NULL DEFAULT '',
                        completed_at TEXT NOT NULL DEFAULT ''
                    )
                    """);
        }

        Set<String> columns = new HashSet<>();
        try (Statement statement = connection.createStatement();
             ResultSet result = statement.executeQuery("PRAGMA table_info(tasks)")) {
            while (result.next()) {
                columns.add(result.getString("name"));
            }
        }

        addColumn(connection, columns, "priority",
                "priority INTEGER NOT NULL DEFAULT 2");
        addColumn(connection, columns, "category",
                "category TEXT NOT NULL DEFAULT 'General'");
        addColumn(connection, columns, "due_date",
                "due_date TEXT NOT NULL DEFAULT ''");
        addColumn(connection, columns, "created_at",
                "created_at TEXT NOT NULL DEFAULT ''");
        addColumn(connection, columns, "completed_at",
                "completed_at TEXT NOT NULL DEFAULT ''");

        try (Statement statement = connection.createStatement()) {
            statement.executeUpdate(
                    "UPDATE tasks SET created_at = strftime('%Y-%m-%dT%H:%M:%SZ', 'now') "
                            + "WHERE created_at = ''");
        }
    }

    private static void addColumn(Connection connection,
                                  Set<String> columns,
                                  String name,
                                  String definition) throws SQLException {
        if (columns.contains(name)) {
            return;
        }
        try (Statement statement = connection.createStatement()) {
            statement.executeUpdate("ALTER TABLE tasks ADD COLUMN " + definition);
        }
    }

    @Override
    public void close() {
        try {
            connection.close();
        } catch (SQLException error) {
            throw new StorageException("Could not close database.", error);
        }
    }
}
