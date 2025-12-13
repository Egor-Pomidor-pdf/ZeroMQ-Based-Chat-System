#pragma once

#include <cstddef>
#include <cctype>
#include <string>
#include <vector>

namespace message {

struct Message {
    std::string type;
    std::string user;
    std::string group;
    std::string text;
};

inline std::string trim(const std::string& s) {
    std::size_t begin = 0;
    while (begin < s.size() && std::isspace(static_cast<unsigned char>(s[begin]))) {
        ++begin;
    }

    std::size_t end = s.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }

    return s.substr(begin, end - begin);
}

inline std::vector<std::string> splitPreserveEmpty(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::size_t start = 0;

    for (std::size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == delim) {
            out.emplace_back(s.substr(start, i - start));
            start = i + 1;
        }
    }

    return out;
}

// Format: TYPE|CLIENT_ID|GROUP|MESSAGE
inline std::string toString(const Message& m) {
    return m.type + "|" + m.user + "|" + m.group + "|" + m.text;
}

inline Message parse(const std::string& line) {
    auto parts = splitPreserveEmpty(line, '|');

    Message m;
    if (parts.size() < 3) {
        return m;
    }

    m.type = trim(parts[0]);
    m.user = trim(parts[1]);
    m.group = trim(parts[2]);

    if (parts.size() >= 4) {
        std::string text = parts[3];
        for (std::size_t i = 4; i < parts.size(); ++i) {
            text += "|";
            text += parts[i];
        }
        m.text = text;
    }

    return m;
}

} // namespace message
