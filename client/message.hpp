#pragma once

#include <cstddef>
#include <cctype>
#include <string>
#include <vector>

#include "utils.hpp"

namespace message {

struct Message {
    std::string type;
    std::string user;
    std::string group;
    std::string text;
};

// Format: TYPE|CLIENT_ID|GROUP|MESSAGE
inline std::string toString(const Message& m) {
    return m.type + "|" + m.user + "|" + m.group + "|" + m.text;
}

inline Message parse(const std::string& line) {
    auto parts =  utils::split(line, '|');

    Message m;
    if (parts.size() < 3) {
        return m;
    }

    m.type = parts[0];
    m.user = parts[1];
    m.group = parts[2];

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
