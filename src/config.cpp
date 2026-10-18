#include "config.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <pwd.h>
#include <unistd.h>

namespace {

std::string GetHomeDirectory() {
    const char* home = std::getenv("HOME");
    if (home != nullptr && home[0] != '\0') {
        return std::string(home);
    }

    struct passwd* pw = getpwuid(getuid());
    if (pw != nullptr && pw->pw_dir != nullptr) {
        return std::string(pw->pw_dir);
    }

    return std::string();
}

void Trim(std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) {
        s.clear();
        return;
    }
    s = s.substr(start, end - start + 1);
}

bool ParseConfigLine(const std::string& line, std::string& key, std::string& value) {
    size_t separator = line.find('=');
    if (separator == std::string::npos) {
        return false;
    }

    key = line.substr(0, separator);
    value = line.substr(separator + 1);

    Trim(key);
    Trim(value);

    return !key.empty() && !value.empty();
}

std::string GetValueForKey(const std::string& target_key, const std::string& fallback) {
    std::string config_path = GetConfigPath();
    if (config_path.empty()) {
        return fallback;
    }

    std::ifstream in(config_path);
    if (!in.is_open()) {
        return fallback;
    }

    std::string line;
    std::string key;
    std::string value;
    while (std::getline(in, line)) {
        if (!ParseConfigLine(line, key, value)) {
            continue;
        }

        if (key == target_key) {
            return value;
        }
    }

    return fallback;
}

}  // namespace

std::string GetConfigPath() {
    std::string home = GetHomeDirectory();
    if (home.empty()) {
        return std::string();
    }
    return home + "/.config/ace/config";
}

void EnsureConfigExists() {
    std::string config_path = GetConfigPath();
    if (config_path.empty()) {
        return;
    }

    if (std::filesystem::exists(config_path)) {
        return;
    }

    std::filesystem::path path(config_path);
    std::filesystem::create_directories(path.parent_path());

    std::ofstream out(config_path);
    if (!out.is_open()) {
        return;
    }

    out << "terminal = kitty\n";
    out << "apps = rofi -show drun\n";
}

std::string GetTerminalCommand() {
    return GetValueForKey("terminal", "kitty");
}

std::string GetLauncherCommand() {
    return GetValueForKey("apps", "rofi");
}

std::vector<std::string> GetAutostartCommands() {
    std::vector<std::string> commands;

    std::string config_path = GetConfigPath();
    if (config_path.empty()) {
        return commands;
    }

    std::ifstream in(config_path);
    if (!in.is_open()) {
        return commands;
    }

    std::string line;
    std::string key;
    std::string value;
    while (std::getline(in, line)) {
        if (!ParseConfigLine(line, key, value)) {
            continue;
        }

        if (key == "execute") {
            commands.push_back(value);
        }
    }

    return commands;
}
