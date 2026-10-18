#include "keybinds.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

#include "config.h"
#include "workspaces.h"

namespace keybinds {

namespace {

struct ActionName {
    const char* name;
    Action action;
};

const ActionName kActionNames[] = {
    {"exec", Action::kExec},
    {"close", Action::kClose},
    {"quit", Action::kQuit},
    {"fullscreen", Action::kFullscreen},
    {"focus_next", Action::kFocusNext},
    {"focus_prev", Action::kFocusPrevious},
    {"swap_next", Action::kSwapNext},
    {"swap_prev", Action::kSwapPrevious},
    {"promote", Action::kPromote},
    {"master_shrink", Action::kMasterShrink},
    {"master_grow", Action::kMasterGrow},
    {"workspace", Action::kWorkspace},
    {"move_to_workspace", Action::kMoveToWorkspace},
    {"reload", Action::kReload},
};

std::string Trim(const std::string& text) {
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return std::string();
    }
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

std::string ToLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

std::vector<std::string> Split(const std::string& text, char delimiter) {
    std::vector<std::string> parts;
    size_t start = 0;
    for (;;) {
        size_t end = text.find(delimiter, start);
        if (end == std::string::npos) {
            parts.push_back(text.substr(start));
            break;
        }
        parts.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    return parts;
}

void ReportError(const std::string& line, const std::string& reason) {
    fprintf(stderr, "ace: config: bind '%s': %s\n", line.c_str(), reason.c_str());
}

bool ParseModifierList(const std::string& text, const unsigned int* mod_mask, unsigned int* result) {
    unsigned int mask = 0;
    for (const std::string& part : Split(text, '+')) {
        std::string name = ToLower(Trim(part));
        if (name == "mod" && mod_mask != nullptr) {
            mask |= *mod_mask;
        } else if (name == "super" || name == "win") {
            mask |= Mod4Mask;
        } else if (name == "alt") {
            mask |= Mod1Mask;
        } else if (name == "ctrl" || name == "control") {
            mask |= ControlMask;
        } else if (name == "shift") {
            mask |= ShiftMask;
        } else {
            return false;
        }
    }
    *result = mask;
    return true;
}

KeySym ParseKeysym(const std::string& name) {
    if (name.empty()) {
        return NoSymbol;
    }

    if (name.size() == 1) {
        return XStringToKeysym(ToLower(name).c_str());
    }

    KeySym keysym = XStringToKeysym(name.c_str());
    if (keysym != NoSymbol) {
        return keysym;
    }

    std::string lower = ToLower(name);
    keysym = XStringToKeysym(lower.c_str());
    if (keysym != NoSymbol) {
        return keysym;
    }

    lower[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(lower[0])));
    return XStringToKeysym(lower.c_str());
}

bool ParseActionName(const std::string& name, Action* action) {
    std::string lower = ToLower(name);
    for (const ActionName& entry : kActionNames) {
        if (lower == entry.name) {
            *action = entry.action;
            return true;
        }
    }
    return false;
}

bool IsDigitRange(const std::string& key) {
    return key.size() == 4 && std::isdigit(static_cast<unsigned char>(key[0])) &&
           key[1] == '.' && key[2] == '.' && std::isdigit(static_cast<unsigned char>(key[3]));
}

bool IsWorkspaceAction(Action action) {
    return action == Action::kWorkspace || action == Action::kMoveToWorkspace;
}

void AddBinding(std::vector<Binding>* bindings, const Binding& binding) {
    for (Binding& existing : *bindings) {
        if (existing.keysym == binding.keysym && existing.modifiers == binding.modifiers) {
            const char* name = XKeysymToString(binding.keysym);
            fprintf(stderr, "ace: config: duplicate binding for key %s, the later one is used\n",
                    name != nullptr ? name : "?");
            existing = binding;
            return;
        }
    }
    bindings->push_back(binding);
}

void ParseBindLine(const std::string& line, unsigned int mod_mask, std::vector<Binding>* bindings) {
    size_t keys_end = line.find_first_of(" \t");
    if (keys_end == std::string::npos) {
        ReportError(line, "expected '<keys> <action>'");
        return;
    }

    std::string keys = line.substr(0, keys_end);
    std::string rest = Trim(line.substr(keys_end));

    size_t action_end = rest.find_first_of(" \t");
    std::string action_name = rest.substr(0, action_end);
    std::string argument =
        action_end == std::string::npos ? std::string() : Trim(rest.substr(action_end));

    Action action;
    if (!ParseActionName(action_name, &action)) {
        ReportError(line, "unknown action '" + action_name + "'");
        return;
    }

    std::vector<std::string> parts = Split(keys, '+');
    for (const std::string& part : parts) {
        if (part.empty()) {
            ReportError(line, "malformed key combination");
            return;
        }
    }

    std::string key_name = parts.back();
    parts.pop_back();

    unsigned int modifiers = 0;
    if (!parts.empty()) {
        std::string modifier_text;
        for (size_t i = 0; i < parts.size(); ++i) {
            modifier_text += (i == 0 ? "" : "+") + parts[i];
        }
        if (!ParseModifierList(modifier_text, &mod_mask, &modifiers)) {
            ReportError(line, "unknown modifier in '" + keys + "'");
            return;
        }
    }

    Binding binding;
    binding.modifiers = modifiers;
    binding.action = action;

    if (IsDigitRange(key_name)) {
        if (!IsWorkspaceAction(action)) {
            ReportError(line, "key ranges are only supported for workspace actions");
            return;
        }
        if (!argument.empty()) {
            ReportError(line, "a key range does not take an argument");
            return;
        }

        int first = key_name[0] - '0';
        int last = key_name[3] - '0';
        if (first > last) {
            ReportError(line, "invalid key range");
            return;
        }

        for (int digit = first; digit <= last; ++digit) {
            Binding entry = binding;
            entry.keysym = ParseKeysym(std::string(1, static_cast<char>('0' + digit)));
            entry.index = digit == 0 ? workspaces::kWorkspaceCount - 1 : digit - 1;
            AddBinding(bindings, entry);
        }
        return;
    }

    binding.keysym = ParseKeysym(key_name);
    if (binding.keysym == NoSymbol) {
        ReportError(line, "unknown key '" + key_name + "'");
        return;
    }

    if (IsWorkspaceAction(action)) {
        char* end = nullptr;
        long number = std::strtol(argument.c_str(), &end, 10);
        if (argument.empty() || *end != '\0' || number < 1 || number > workspaces::kWorkspaceCount) {
            ReportError(line, "expected a workspace number from 1 to " +
                                  std::to_string(workspaces::kWorkspaceCount));
            return;
        }
        binding.index = static_cast<int>(number) - 1;
    } else if (action == Action::kExec) {
        if (argument.empty()) {
            ReportError(line, "exec needs a command");
            return;
        }
        binding.argument = argument;
    } else if (!argument.empty()) {
        ReportError(line, "action '" + action_name + "' does not take an argument");
        return;
    }

    AddBinding(bindings, binding);
}

}

std::vector<Binding> Load() {
    unsigned int mod_mask = Mod4Mask;
    std::string mod_value = GetModifierValue();
    unsigned int parsed_mask = 0;
    if (ParseModifierList(mod_value, nullptr, &parsed_mask)) {
        mod_mask = parsed_mask;
    } else {
        fprintf(stderr, "ace: config: invalid mod value '%s', using Super\n", mod_value.c_str());
    }

    std::vector<std::string> lines = GetBindLines();
    if (lines.empty()) {
        fputs("ace: config: no bind entries found, using the defaults\n", stderr);
        lines = GetDefaultBindLines();
    }

    std::vector<Binding> bindings;
    for (const std::string& line : lines) {
        ParseBindLine(line, mod_mask, &bindings);
    }
    return bindings;
}

}
