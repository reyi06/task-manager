#pragma once

#include <cstddef>
#include <string>

namespace taskman {

class Terminal {
public:
    Terminal();

    bool interactive() const { return interactive_; }
    bool ansi() const { return ansi_; }

    void clear() const;

    std::string paint(const std::string& text, const std::string& code) const;
    std::string bold(const std::string& text) const;
    std::string dim(const std::string& text) const;
    std::string red(const std::string& text) const;
    std::string green(const std::string& text) const;
    std::string yellow(const std::string& text) const;
    std::string cyan(const std::string& text) const;
    std::string magenta(const std::string& text) const;
    std::string white(const std::string& text) const;

    std::string padRight(const std::string& text, std::size_t width) const;
    static std::size_t visibleLength(const std::string& text);

private:
    bool interactive_ = false;
    bool ansi_ = false;
};

}  // namespace taskman
