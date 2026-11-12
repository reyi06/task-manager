public class Terminal {
    private static final String RESET = "\u001b[0m";

    private final boolean interactive;
    private final boolean ansi;

    public Terminal() {
        String noColor = System.getenv("NO_COLOR");
        String term = System.getenv("TERM");

        // Colour only when both ends are a console, the same test the C++ version
        // makes with isatty. Piped output or piped input stays plain text.
        interactive = System.console() != null;
        boolean dumbTerminal = term != null && term.equalsIgnoreCase("dumb");
        ansi = interactive && noColor == null && !dumbTerminal;
    }

    public boolean interactive() {
        return interactive;
    }

    public boolean ansi() {
        return ansi;
    }

    public void clear() {
        if (ansi) {
            System.out.print("\u001b[2J\u001b[H");
            System.out.flush();
        }
    }

    public String paint(String text, String code) {
        if (!ansi) {
            return text;
        }
        return code + text + RESET;
    }

    public String bold(String text) {
        return paint(text, "\u001b[1m");
    }

    public String dim(String text) {
        return paint(text, "\u001b[2m");
    }

    public String red(String text) {
        return paint(text, "\u001b[31m");
    }

    public String green(String text) {
        return paint(text, "\u001b[32m");
    }

    public String yellow(String text) {
        return paint(text, "\u001b[33m");
    }

    public String cyan(String text) {
        return paint(text, "\u001b[36m");
    }

    public String magenta(String text) {
        return paint(text, "\u001b[35m");
    }

    public String white(String text) {
        return paint(text, "\u001b[97m");
    }

    public String padRight(String text, int width) {
        int length = visibleLength(text);
        if (length >= width) {
            return text;
        }
        return text + " ".repeat(width - length);
    }

    public static int visibleLength(String text) {
        int length = 0;
        int index = 0;

        while (index < text.length()) {
            if (text.charAt(index) == '\u001b'
                    && index + 1 < text.length()
                    && text.charAt(index + 1) == '[') {
                index += 2;
                while (index < text.length() && text.charAt(index) != 'm') {
                    index++;
                }
                if (index < text.length()) {
                    index++;
                }
                continue;
            }
            index += Character.charCount(text.codePointAt(index));
            length++;
        }

        return length;
    }
}
