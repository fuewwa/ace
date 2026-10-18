#include "window_manager.h"

#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "config.h"
#include "ewmh.h"
#include "process.h"
#include "workspaces.h"

namespace {

constexpr int kBorderWidth = 2;
constexpr int kGap = 6;
constexpr double kMasterRatioStep = 0.05;
constexpr double kMinMasterRatio = 0.15;
constexpr double kMaxMasterRatio = 0.85;
constexpr const char* kFocusedBorderColor = "#5e81ac";
constexpr const char* kNormalBorderColor = "#3b4252";

unsigned long AllocateColor(Display* display, const char* name, unsigned long fallback) {
    Colormap colormap = DefaultColormap(display, DefaultScreen(display));
    XColor color;
    if (!XParseColor(display, colormap, name, &color) ||
        !XAllocColor(display, colormap, &color)) {
        return fallback;
    }
    return color.pixel;
}

}

int WindowManager::OnXError(Display* display, XErrorEvent* e) {
    char buffer[1024];
    XGetErrorText(display, e->error_code, buffer, sizeof(buffer));
    fprintf(stderr, "X error: %s (request code %d, minor code %d)\n",
            buffer, e->request_code, e->minor_code);
    return 0;
}

WindowManager* WindowManager::Create() {
    Display* display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        return nullptr;
    }
    return new WindowManager(display);
}

WindowManager::WindowManager(Display* display)
    : display_(display),
      root_(DefaultRootWindow(display)) {}

WindowManager::~WindowManager() {
    XCloseDisplay(display_);
}

void WindowManager::Run() {
    XSetErrorHandler(&WindowManager::OnXError);

    XSelectInput(
        display_,
        root_,
        SubstructureRedirectMask | SubstructureNotifyMask);

    XSync(display_, False);

    int screen = DefaultScreen(display_);
    XSetWindowBackground(display_, root_, BlackPixel(display_, screen));
    XClearWindow(display_, root_);
    XFlush(display_);

    Cursor default_cursor = XCreateFontCursor(display_, XC_left_ptr);
    XDefineCursor(display_, root_, default_cursor);
    XFlush(display_);

    LoadBindings();

    focused_border_pixel_ = AllocateColor(display_, kFocusedBorderColor, WhitePixel(display_, screen));
    normal_border_pixel_ = AllocateColor(display_, kNormalBorderColor, BlackPixel(display_, screen));

    ewmh::Initialize(display_, root_);
    workspaces::Initialize(display_, root_);

    for (const std::string& command : GetAutostartCommands()) {
        LaunchCommand(command);
    }

    for (;;) {
        XEvent e;
        XNextEvent(display_, &e);

        switch (e.type) {
            case MapRequest:
                OnMapRequest(e.xmaprequest);
                break;
            case ConfigureRequest:
                OnConfigureRequest(e.xconfigurerequest);
                break;
            case KeyPress:
                OnKeyPress(e.xkey);
                break;
            case DestroyNotify:
                OnDestroyNotify(e.xdestroywindow);
                break;
            case EnterNotify:
                OnEnterNotify(e.xcrossing);
                break;
            case ClientMessage:
                OnClientMessage(e.xclient);
                break;
            default:
                break;
        }
    }
}

void WindowManager::OnMapRequest(const XMapRequestEvent& e) {
    if (managed_windows_.count(e.window) > 0) {
        XMapWindow(display_, e.window);
        return;
    }

    if (!ewmh::ShouldTile(display_, e.window)) {
        XMapWindow(display_, e.window);
        return;
    }

    ExitFullscreenOnCurrentWorkspace();

    XSelectInput(display_, e.window, EnterWindowMask);
    managed_windows_[e.window] = true;
    workspaces::AddWindow(e.window);
    NotifyClientListChanged();

    Retile();
    XMapWindow(display_, e.window);

    FocusWindow(e.window);
    WarpPointerToWindow(e.window);
}

void WindowManager::OnConfigureRequest(const XConfigureRequestEvent& e) {
    if (managed_windows_.count(e.window) > 0) {
        SendConfigureNotify(e.window);
        return;
    }

    XWindowChanges changes;
    changes.x = e.x;
    changes.y = e.y;
    changes.width = e.width;
    changes.height = e.height;
    changes.border_width = e.border_width;
    changes.sibling = e.above;
    changes.stack_mode = e.detail;
    XConfigureWindow(display_, e.window, e.value_mask, &changes);
}

void WindowManager::SendConfigureNotify(Window window) {
    XWindowAttributes attrs;
    if (!XGetWindowAttributes(display_, window, &attrs)) {
        return;
    }

    XConfigureEvent event = {};
    event.type = ConfigureNotify;
    event.display = display_;
    event.event = window;
    event.window = window;
    event.x = attrs.x;
    event.y = attrs.y;
    event.width = attrs.width;
    event.height = attrs.height;
    event.border_width = attrs.border_width;
    event.above = None;
    event.override_redirect = False;

    XSendEvent(display_, window, False, StructureNotifyMask,
               reinterpret_cast<XEvent*>(&event));
}

void WindowManager::OnKeyPress(const XKeyEvent& e) {
    unsigned int state = e.state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask);

    for (const keybinds::Binding& candidate : bindings_) {
        if (candidate.modifiers != state ||
            XKeysymToKeycode(display_, candidate.keysym) != e.keycode) {
            continue;
        }

        keybinds::Binding binding = candidate;
        RunAction(binding);
        return;
    }
}

void WindowManager::LoadBindings() {
    XUngrabKey(display_, AnyKey, AnyModifier, root_);

    bindings_ = keybinds::Load();
    for (const keybinds::Binding& binding : bindings_) {
        if (XKeysymToKeycode(display_, binding.keysym) == 0) {
            const char* name = XKeysymToString(binding.keysym);
            fprintf(stderr, "ace: config: key %s is not available on this keyboard\n",
                    name != nullptr ? name : "?");
            continue;
        }
        GrabKeyWithLockVariants(binding.keysym, binding.modifiers);
    }
}

void WindowManager::RunAction(const keybinds::Binding& binding) {
    switch (binding.action) {
        case keybinds::Action::kExec:
            LaunchCommand(binding.argument);
            break;
        case keybinds::Action::kClose:
            CloseFocusedWindow();
            break;
        case keybinds::Action::kQuit:
            exit(0);
        case keybinds::Action::kFullscreen:
            ToggleFocusedFullscreen();
            break;
        case keybinds::Action::kFocusNext:
            FocusRelative(1);
            break;
        case keybinds::Action::kFocusPrevious:
            FocusRelative(-1);
            break;
        case keybinds::Action::kSwapNext:
            SwapFocusedRelative(1);
            break;
        case keybinds::Action::kSwapPrevious:
            SwapFocusedRelative(-1);
            break;
        case keybinds::Action::kPromote:
            PromoteFocused();
            break;
        case keybinds::Action::kMasterShrink:
            ChangeMasterRatio(-kMasterRatioStep);
            break;
        case keybinds::Action::kMasterGrow:
            ChangeMasterRatio(kMasterRatioStep);
            break;
        case keybinds::Action::kWorkspace:
            SwitchWorkspace(binding.index);
            break;
        case keybinds::Action::kMoveToWorkspace:
            MoveFocusedToWorkspace(binding.index);
            break;
        case keybinds::Action::kReload:
            LoadBindings();
            break;
    }
}

void WindowManager::GrabKeyWithLockVariants(KeySym keysym, unsigned int modifiers) {
    KeyCode keycode = XKeysymToKeycode(display_, keysym);

    const unsigned int lock_variants[] = {
        0,
        LockMask,
        Mod2Mask,
        LockMask | Mod2Mask,
    };

    for (unsigned int lock_mask : lock_variants) {
        XGrabKey(
            display_,
            keycode,
            modifiers | lock_mask,
            root_,
            False,
            GrabModeAsync,
            GrabModeAsync);
    }
}

void WindowManager::OnEnterNotify(const XCrossingEvent& e) {
    if (managed_windows_.count(e.window) == 0 || e.window == focused_window_) {
        return;
    }

    XWindowAttributes attrs;
    if (!XGetWindowAttributes(display_, e.window, &attrs) || attrs.map_state != IsViewable) {
        return;
    }

    FocusWindow(e.window);
}

void WindowManager::FocusWindow(Window window) {
    if (window == None) {
        XSetInputFocus(display_, PointerRoot, RevertToPointerRoot, CurrentTime);
    } else {
        XSetInputFocus(display_, window, RevertToPointerRoot, CurrentTime);
    }
    focused_window_ = window;
    ewmh::UpdateActiveWindow(display_, root_, window);
    UpdateBorders();
}

void WindowManager::RefocusUnderPointer() {
    Window root_return;
    Window child_return;
    int root_x;
    int root_y;
    int win_x;
    int win_y;
    unsigned int mask_return;

    bool has_child = XQueryPointer(
        display_, root_, &root_return, &child_return,
        &root_x, &root_y, &win_x, &win_y, &mask_return) &&
        child_return != None &&
        managed_windows_.count(child_return) > 0;

    if (has_child) {
        XWindowAttributes attrs;
        has_child = XGetWindowAttributes(display_, child_return, &attrs) &&
                    attrs.map_state == IsViewable;
    }

    if (has_child) {
        FocusWindow(child_return);
        return;
    }

    const std::vector<Window>& windows = workspaces::GetWindows();
    FocusWindow(windows.empty() ? None : windows.back());
}

void WindowManager::WarpPointerToWindow(Window window) {
    if (window == None) {
        return;
    }

    XWindowAttributes attrs;
    if (!XGetWindowAttributes(display_, window, &attrs)) {
        return;
    }

    XWarpPointer(display_, None, window, 0, 0, 0, 0, attrs.width / 2, attrs.height / 2);
}

void WindowManager::ApplyGeometry(Window window, const Geometry& geometry) {
    int width = std::max(1, geometry.width - 2 * kBorderWidth);
    int height = std::max(1, geometry.height - 2 * kBorderWidth);

    XSetWindowBorderWidth(display_, window, kBorderWidth);
    XMoveResizeWindow(display_, window, geometry.x, geometry.y,
                       static_cast<unsigned int>(width),
                       static_cast<unsigned int>(height));
}

void WindowManager::PlaceFullscreen(Window window) {
    int screen = DefaultScreen(display_);
    int screen_width = DisplayWidth(display_, screen);
    int screen_height = DisplayHeight(display_, screen);

    XSetWindowBorderWidth(display_, window, 0);
    XMoveResizeWindow(display_, window, 0, 0,
                       static_cast<unsigned int>(screen_width),
                       static_cast<unsigned int>(screen_height));
    XRaiseWindow(display_, window);
}

void WindowManager::Retile() {
    int screen = DefaultScreen(display_);
    int screen_width = DisplayWidth(display_, screen);
    int screen_height = DisplayHeight(display_, screen);

    std::vector<Window> tiled;
    for (Window window : workspaces::GetWindows()) {
        if (fullscreen_windows_.count(window) > 0) {
            PlaceFullscreen(window);
        } else {
            tiled.push_back(window);
        }
    }

    if (!tiled.empty()) {
        Geometry area{kGap, kGap, screen_width - 2 * kGap, screen_height - 2 * kGap};

        if (tiled.size() == 1) {
            ApplyGeometry(tiled[0], area);
        } else {
            int master_width = static_cast<int>((area.width - kGap) * master_ratio_);
            ApplyGeometry(tiled[0], Geometry{area.x, area.y, master_width, area.height});

            int stack_x = area.x + master_width + kGap;
            int stack_width = area.width - master_width - kGap;
            int stack_count = static_cast<int>(tiled.size()) - 1;
            int cell_height = (area.height - kGap * (stack_count - 1)) / stack_count;

            int y = area.y;
            for (int i = 0; i < stack_count; ++i) {
                int height = (i == stack_count - 1) ? (area.y + area.height - y) : cell_height;
                ApplyGeometry(tiled[i + 1], Geometry{stack_x, y, stack_width, height});
                y += height + kGap;
            }
        }
    }

    UpdateBorders();
}

void WindowManager::UpdateBorders() {
    for (Window window : workspaces::GetWindows()) {
        if (fullscreen_windows_.count(window) > 0) {
            continue;
        }
        unsigned long pixel =
            window == focused_window_ ? focused_border_pixel_ : normal_border_pixel_;
        XSetWindowBorder(display_, window, pixel);
    }
}

void WindowManager::FocusRelative(int direction) {
    const std::vector<Window>& windows = workspaces::GetWindows();
    if (windows.empty() || fullscreen_windows_.count(focused_window_) > 0) {
        return;
    }

    int count = static_cast<int>(windows.size());
    auto it = std::find(windows.begin(), windows.end(), focused_window_);
    int index = 0;
    if (it != windows.end()) {
        index = (static_cast<int>(it - windows.begin()) + direction + count) % count;
    }

    Window target = windows[index];
    FocusWindow(target);
    WarpPointerToWindow(target);
}

void WindowManager::SwapFocusedRelative(int direction) {
    const std::vector<Window>& windows = workspaces::GetWindows();
    if (windows.size() < 2 || focused_window_ == None ||
        fullscreen_windows_.count(focused_window_) > 0) {
        return;
    }

    auto it = std::find(windows.begin(), windows.end(), focused_window_);
    if (it == windows.end()) {
        return;
    }

    int count = static_cast<int>(windows.size());
    int index = static_cast<int>(it - windows.begin());
    int other = (index + direction + count) % count;

    Window first = windows[index];
    Window second = windows[other];
    workspaces::Swap(first, second);

    Retile();
    WarpPointerToWindow(focused_window_);
}

void WindowManager::PromoteFocused() {
    if (focused_window_ == None || fullscreen_windows_.count(focused_window_) > 0) {
        return;
    }

    workspaces::Promote(focused_window_);
    Retile();
    WarpPointerToWindow(focused_window_);
}

void WindowManager::ChangeMasterRatio(double delta) {
    master_ratio_ = std::min(kMaxMasterRatio, std::max(kMinMasterRatio, master_ratio_ + delta));
    Retile();
    WarpPointerToWindow(focused_window_);
}

void WindowManager::SwitchWorkspace(int index) {
    workspaces::SwitchTo(index);
    Retile();
    RefocusUnderPointer();
}

void WindowManager::MoveFocusedToWorkspace(int index) {
    if (focused_window_ == None || index == workspaces::GetCurrentIndex()) {
        return;
    }

    workspaces::MoveWindow(focused_window_, index);
    Retile();
    RefocusUnderPointer();
}

void WindowManager::ExitFullscreenOnCurrentWorkspace() {
    std::vector<Window> windows = workspaces::GetWindows();
    for (Window window : windows) {
        if (fullscreen_windows_.count(window) > 0) {
            SetFullscreen(window, false);
        }
    }
}

void WindowManager::SetFullscreen(Window window, bool fullscreen) {
    if (managed_windows_.count(window) == 0) {
        return;
    }

    bool currently_fullscreen = fullscreen_windows_.count(window) > 0;
    if (fullscreen == currently_fullscreen) {
        return;
    }

    if (fullscreen) {
        fullscreen_windows_.insert(window);
    } else {
        fullscreen_windows_.erase(window);
    }

    ewmh::SetFullscreen(display_, window, fullscreen);
    Retile();

    if (!fullscreen && window == focused_window_) {
        WarpPointerToWindow(window);
    }
}

void WindowManager::ToggleFocusedFullscreen() {
    if (focused_window_ == None) {
        return;
    }

    bool currently_fullscreen = fullscreen_windows_.count(focused_window_) > 0;
    SetFullscreen(focused_window_, !currently_fullscreen);
}

void WindowManager::OnClientMessage(const XClientMessageEvent& e) {
    bool currently_fullscreen = fullscreen_windows_.count(e.window) > 0;
    bool want_fullscreen = false;

    if (ewmh::ResolveFullscreenRequest(display_, e, currently_fullscreen, &want_fullscreen)) {
        SetFullscreen(e.window, want_fullscreen);
    }
}

void WindowManager::CloseFocusedWindow() {
    if (focused_window_ == None) {
        return;
    }

    Atom wm_protocols = XInternAtom(display_, "WM_PROTOCOLS", False);
    Atom wm_delete_window = XInternAtom(display_, "WM_DELETE_WINDOW", False);

    Atom* supported_protocols = nullptr;
    int protocol_count = 0;
    bool supports_delete = false;

    if (XGetWMProtocols(display_, focused_window_, &supported_protocols, &protocol_count)) {
        for (int i = 0; i < protocol_count; ++i) {
            if (supported_protocols[i] == wm_delete_window) {
                supports_delete = true;
                break;
            }
        }
        XFree(supported_protocols);
    }

    if (supports_delete) {
        XClientMessageEvent message = {};
        message.type = ClientMessage;
        message.window = focused_window_;
        message.message_type = wm_protocols;
        message.format = 32;
        message.data.l[0] = static_cast<long>(wm_delete_window);
        message.data.l[1] = CurrentTime;

        XSendEvent(display_, focused_window_, False, NoEventMask,
                   reinterpret_cast<XEvent*>(&message));
    } else {
        XKillClient(display_, focused_window_);
    }
}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& e) {
    bool was_managed = managed_windows_.erase(e.window) > 0;
    fullscreen_windows_.erase(e.window);
    workspaces::RemoveWindow(e.window);
    NotifyClientListChanged();

    if (!was_managed) {
        return;
    }

    Retile();

    if (focused_window_ == e.window) {
        focused_window_ = None;
        RefocusUnderPointer();
    }
}

void WindowManager::NotifyClientListChanged() {
    std::vector<Window> windows;
    windows.reserve(managed_windows_.size());
    for (const auto& entry : managed_windows_) {
        windows.push_back(entry.first);
    }
    ewmh::UpdateClientList(display_, root_, windows);
}
