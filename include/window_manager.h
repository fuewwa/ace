#ifndef ACE_WINDOW_MANAGER_H_
#define ACE_WINDOW_MANAGER_H_

#include <X11/Xlib.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "keybinds.h"

class WindowManager {
public:
    static WindowManager* Create();
    ~WindowManager();

    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;

    void Run();

private:
    struct Geometry {
        int x;
        int y;
        int width;
        int height;
    };

    explicit WindowManager(Display* display);

    void OnMapRequest(const XMapRequestEvent& e);
    void OnConfigureRequest(const XConfigureRequestEvent& e);
    void OnKeyPress(const XKeyEvent& e);
    void OnDestroyNotify(const XDestroyWindowEvent& e);
    void OnEnterNotify(const XCrossingEvent& e);
    void OnClientMessage(const XClientMessageEvent& e);

    static int OnXError(Display* display, XErrorEvent* e);

    void CloseFocusedWindow();
    void GrabKeyWithLockVariants(KeySym keysym, unsigned int modifiers);
    void LoadBindings();
    void RunAction(const keybinds::Binding& binding);
    void NotifyClientListChanged();
    void RefocusUnderPointer();
    void ToggleFocusedFullscreen();
    void SetFullscreen(Window window, bool fullscreen);
    void ExitFullscreenOnCurrentWorkspace();

    void Retile();
    void ApplyGeometry(Window window, const Geometry& geometry);
    void PlaceFullscreen(Window window);
    void UpdateBorders();
    void FocusWindow(Window window);
    void FocusRelative(int direction);
    void SwapFocusedRelative(int direction);
    void PromoteFocused();
    void ChangeMasterRatio(double delta);
    void SwitchWorkspace(int index);
    void MoveFocusedToWorkspace(int index);
    void SendConfigureNotify(Window window);
    void WarpPointerToWindow(Window window);

    Display* display_;
    Window root_;

    Window focused_window_ = None;
    double master_ratio_ = 0.55;
    unsigned long focused_border_pixel_ = 0;
    unsigned long normal_border_pixel_ = 0;

    std::unordered_map<Window, bool> managed_windows_;
    std::unordered_set<Window> fullscreen_windows_;
    std::vector<keybinds::Binding> bindings_;
};

#endif  // ACE_WINDOW_MANAGER_H_
