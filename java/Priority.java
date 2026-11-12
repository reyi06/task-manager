import java.util.Locale;

public enum Priority {
    LOW(1, "Low", "LOW"),
    NORMAL(2, "Normal", "NORM"),
    HIGH(3, "High", "HIGH");

    private final int code;
    private final String displayName;
    private final String shortName;

    Priority(int code, String displayName, String shortName) {
        this.code = code;
        this.displayName = displayName;
        this.shortName = shortName;
    }

    public int code() {
        return code;
    }

    public String displayName() {
        return displayName;
    }

    public String shortName() {
        return shortName;
    }

    public static Priority fromCode(int code) {
        return switch (code) {
            case 1 -> LOW;
            case 3 -> HIGH;
            default -> NORMAL;
        };
    }

    public static Priority parse(String text) {
        return switch (text.trim().toLowerCase(Locale.ROOT)) {
            case "low", "l", "1" -> LOW;
            case "normal", "n", "2" -> NORMAL;
            case "high", "h", "3" -> HIGH;
            default -> throw new IllegalArgumentException(
                    "Priority must be 1 (Low), 2 (Normal), or 3 (High).");
        };
    }
}
