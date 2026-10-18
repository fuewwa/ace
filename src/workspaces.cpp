#include "workspaces.h"

#include <algorithm>
#include <array>

namespace workspaces {

namespace {

Display* g_display = nullptr;
Window g_root = None;
int g_current_index = 0;
std::array<std::vector<Window>, kWorkspaceCount> g_windows;

int FindWorkspace(Window window) {
    for (int i = 0; i < kWorkspaceCount; ++i) {
        const std::vector<Window>& list = g_windows[i];
        if (std::find(list.begin(), list.end(), window) != list.end()) {
            return i;
        }
    }
    return -1;
}

}  // namespace

void Initialize(Display* display, Window root) {
    g_display = display;
    g_root = root;
    g_current_index = 0;
    for (std::vector<Window>& list : g_windows) {
        list.clear();
    }
}

void AddWindow(Window window) {
    if (FindWorkspace(window) >= 0) {
        return;
    }
    g_windows[g_current_index].push_back(window);
}

void RemoveWindow(Window window) {
    int index = FindWorkspace(window);
    if (index < 0) {
        return;
    }
    std::vector<Window>& list = g_windows[index];
    list.erase(std::remove(list.begin(), list.end(), window), list.end());
}

void SwitchTo(int index) {
    if (index < 0 || index >= kWorkspaceCount || index == g_current_index) {
        return;
    }

    for (Window window : g_windows[index]) {
        XMapWindow(g_display, window);
    }
    for (Window window : g_windows[g_current_index]) {
        XUnmapWindow(g_display, window);
    }

    g_current_index = index;
}

int GetCurrentIndex() {
    return g_current_index;
}

const std::vector<Window>& GetWindows() {
    return g_windows[g_current_index];
}

void MoveWindow(Window window, int index) {
    if (index < 0 || index >= kWorkspaceCount) {
        return;
    }

    int from = FindWorkspace(window);
    if (from < 0 || from == index) {
        return;
    }

    RemoveWindow(window);
    g_windows[index].push_back(window);

    if (index == g_current_index) {
        XMapWindow(g_display, window);
    } else if (from == g_current_index) {
        XUnmapWindow(g_display, window);
    }
}

void Swap(Window first, Window second) {
    std::vector<Window>& list = g_windows[g_current_index];
    auto first_it = std::find(list.begin(), list.end(), first);
    auto second_it = std::find(list.begin(), list.end(), second);
    if (first_it == list.end() || second_it == list.end()) {
        return;
    }
    std::iter_swap(first_it, second_it);
}

void Promote(Window window) {
    std::vector<Window>& list = g_windows[g_current_index];
    auto it = std::find(list.begin(), list.end(), window);
    if (it == list.end() || it == list.begin()) {
        return;
    }
    std::rotate(list.begin(), it, it + 1);
}

}  // namespace workspaces
