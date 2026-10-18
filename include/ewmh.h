#ifndef ACE_EWMH_H_
#define ACE_EWMH_H_

#include <X11/Xlib.h>
#include <vector>

namespace ewmh {

void Initialize(Display* display, Window root);
void UpdateClientList(Display* display, Window root, const std::vector<Window>& windows);
void UpdateActiveWindow(Display* display, Window root, Window window);
void SetFullscreen(Display* display, Window window, bool fullscreen);
bool ShouldTile(Display* display, Window window);
bool ResolveFullscreenRequest(Display* display, const XClientMessageEvent& e,
                               bool currently_fullscreen, bool* want_fullscreen);

}  // namespace ewmh

#endif  // ACE_EWMH_H_
