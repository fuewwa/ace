#ifndef ACE_WORKSPACES_H_
#define ACE_WORKSPACES_H_

#include <X11/Xlib.h>
#include <vector>

namespace workspaces {

constexpr int kWorkspaceCount = 10;

void Initialize(Display* display, Window root);
void AddWindow(Window window);
void RemoveWindow(Window window);
void SwitchTo(int index);
int GetCurrentIndex();
const std::vector<Window>& GetWindows();
void MoveWindow(Window window, int index);
void Swap(Window first, Window second);
void Promote(Window window);

}  // namespace workspaces

#endif  // ACE_WORKSPACES_H_
