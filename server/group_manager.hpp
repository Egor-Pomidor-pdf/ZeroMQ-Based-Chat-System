#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

class GroupManager {
private:
    std::unordered_map<std::string, std::vector<std::string>> groups;

public:
    bool createGroup(const std::string& name) {
        if (name.empty()) {
            return false;
        }
        if (groups.find(name) != groups.end()) {
            return false;
        }
        groups.emplace(name, std::vector<std::string>{});
        return true;
    }

    bool joinGroup(const std::string& group, const std::string& user) {
        if (group.empty() || user.empty()) {
            return false;
        }

        auto it = groups.find(group);
        if (it == groups.end()) {
            return false;
        }

        auto& members = it->second;
        if (std::find(members.begin(), members.end(), user) != members.end()) {
            return true; // already a member
        }

        members.push_back(user);
        return true;
    }

    bool isMember(const std::string& group, const std::string& user) const {
        auto it = groups.find(group);
        if (it == groups.end()) {
            return false;
        }
        const auto& members = it->second;
        return std::find(members.begin(), members.end(), user) != members.end();
    }

    // std::vector<std::string> getMembers(const std::string& group) const {
    //     auto it = groups.find(group);
    //     if (it == groups.end()) {
    //         return {};
    //     }
    //     return it->second;
    // }

    bool groupExists(const std::string& group) const {
        return groups.find(group) != groups.end();
    }
};
