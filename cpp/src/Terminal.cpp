#include "taskman/Terminal.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace taskman {

namespace {

constexpr const char* kReset = "\x1b[0m";

bool isInteractive() {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

// Colour only when both ends are a console, the same test the Java version makes
// with System.console(). Piped output or piped input stays plain text.
bool isInteractiveInput() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(fileno(stdin)) != 0;
#endif
}

bool enableAnsi() {
#ifdef _WIN32
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD mode = 0;
    if (!GetConsoleMode(handle, &mode)) {
        return false;
    }
    return SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
    return true;
#endif
}

}  // namespace

Terminal::Terminal() {
    interactive_ = isInteractive() && isInteractiveInput();
    const char* noColor = std::getenv("NO_COLOR");
    const char* term = std::getenv("TERM");
    const bool termIsDumb = term != nullptr && std::string(term) == "dumb";

    if (interactive_ && noColor == nullptr && !termIsDumb) {
        ansi_ = enableAnsi();
    }
}

void Terminal::clear() const {
    if (ansi_) {
        std::printf("\x1b[2J\x1b[H");
        std::fflush(stdout);
    }
}

std::string Terminal::paint(const std::string& text, const std::string& code) const {
    if (!ansi_) {
        return text;
    }
    return code + text + kReset;
}

std::string Terminal::bold(const std::string& text) const {
    return paint(text, "\x1b[1m");
}

std::string Terminal::dim(const std::string& text) const {
    return paint(text, "\x1b[2m");
}

std::string Terminal::red(const std::string& text) const {
    return paint(text, "\x1b[31m");
}

std::string Terminal::green(const std::string& text) const {
    return paint(text, "\x1b[32m");
}

std::string Terminal::yellow(const std::string& text) const {
    return paint(text, "\x1b[33m");
}

std::string Terminal::cyan(const std::string& text) const {
    return paint(text, "\x1b[36m");
}

std::string Terminal::magenta(const std::string& text) const {
    return paint(text, "\x1b[35m");
}

std::string Terminal::white(const std::string& text) const {
    return paint(text, "\x1b[97m");
}

std::size_t Terminal::visibleLength(const std::string& text) {
    std::size_t length = 0;
    std::size_t index = 0;

    while (index < text.size()) {
        if (text[index] == '\x1b' && index + 1 < text.size() && text[index + 1] == '[') {
            index += 2;
            while (index < text.size() && text[index] != 'm') {
                ++index;
            }
            if (index < text.size()) {
                ++index;
            }
            continue;
        }
        // count characters, not bytes, so accented text pads like the Java version
        if ((static_cast<unsigned char>(text[index]) & 0xC0) != 0x80) {
            ++length;
        }
        ++index;
    }

    return length;
}

std::string Terminal::padRight(const std::string& text, std::size_t width) const {
    const std::size_t length = visibleLength(text);
    if (length >= width) {
        return text;
    }
    return text + std::string(width - length, ' ');
}

}  // namespace taskman
