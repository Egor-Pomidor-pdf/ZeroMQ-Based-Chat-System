#pragma once

#include <cctype>
#include <iostream>
#include <sstream>
#include <string>

namespace ui {

enum class CommandType {
    CreateGroup,
    JoinGroup,
    Send,
    Help,
    Exit,
    Unknown,
};

struct Command {
    CommandType type = CommandType::Unknown;
    std::string group;
    std::string text;
};

inline std::string ltrimSpaces(std::string s) {
    std::size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
        ++i;
    }
    s.erase(0, i);
    return s;
}

inline void printHelp() {
    std::cout
        << "Commands:\n"
        << "  create_group <group>\n"
        << "  join_group <group>\n"
        << "  send <group> <text>\n"
        << "  help\n"
        << "  exit\n";
}

inline void printPrompt() {
    std::cout << "> " << std::flush;
}

inline Command parseCommandLine(const std::string& line) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd == "help" || cmd == "?") {
        return {CommandType::Help, "", ""};
    }

    if (cmd == "exit" || cmd == "quit") {
        return {CommandType::Exit, "", ""};
    }

    if (cmd == "create_group") {
        std::string group;
        iss >> group;
        if (group.empty()) {
            return {CommandType::Unknown, "", ""};
        }
        return {CommandType::CreateGroup, group, ""};
    }

    if (cmd == "join_group") {
        std::string group;
        iss >> group;
        if (group.empty()) {
            return {CommandType::Unknown, "", ""};
        }
        return {CommandType::JoinGroup, group, ""};
    }

    if (cmd == "send") {
        std::string group;
        iss >> group;
        std::string text;
        std::getline(iss, text);
        text = ltrimSpaces(text);

        if (group.empty()) {
            return {CommandType::Unknown, "", ""};
        }

        return {CommandType::Send, group, text};
    }

    return {CommandType::Unknown, "", ""};
}

} // namespace ui
