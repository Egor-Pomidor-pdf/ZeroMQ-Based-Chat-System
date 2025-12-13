#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "utils.hpp"

namespace message {

enum class Type {
    CreateGroup,
    JoinGroup,
    Send,
    Unknown,
};

struct Message {
    Type type = Type::Unknown;
    std::string user;
    std::string group;
    std::string text;
};

inline Type parseType(const std::string& s) {
    if (s == "CREATE_GROUP") {
        return Type::CreateGroup;
    }
    if (s == "JOIN_GROUP") {
        return Type::JoinGroup;
    }
    if (s == "SEND") {
        return Type::Send;
    }
    return Type::Unknown;
}

// Format: TYPE|CLIENT_ID|GROUP|MESSAGE
inline Message parseMessage(const std::string& line) {
    auto parts = utils::split(line, '|');
    if (parts.size() < 3) {
        return {};
    }

    Message m;
    m.type = parseType(utils::trim(parts[0]));
    m.user = utils::trim(parts[1]);
    m.group = utils::trim(parts[2]);

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

inline std::string makeReplyOk() {
    return "OK";
}

inline std::string makeReplyError(const std::string& reason) {
    return std::string("ERROR|") + reason;
}

} // namespace message
