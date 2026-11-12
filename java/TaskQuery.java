public record TaskQuery(String keyword, Boolean done, Priority priority, String category) {
    public static TaskQuery all() {
        return new TaskQuery("", null, null, "");
    }
}
