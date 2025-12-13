#pragma once

#include <algorithm>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace utils {

inline std::string trim(const std::string& s) {
    auto begin = s.begin();
    while (begin != s.end() && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }

    if (begin == s.end()) {
        return "";
    }

    auto end = s.end();
    do {
        --end;
    } while (end != begin && std::isspace(static_cast<unsigned char>(*end)));

    return std::string(begin, end + 1);
}

// Split preserving empty fields (including trailing empties).
inline std::vector<std::string> split(const std::string& s, char delim) {
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

inline std::string nowTimestamp() {
    using clock = std::chrono::system_clock;
    auto t = clock::to_time_t(clock::now());

    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

inline void logInfo(const std::string& msg) {
    std::cerr << "[" << nowTimestamp() << "] " << msg << std::endl;
}

} // namespace utils
