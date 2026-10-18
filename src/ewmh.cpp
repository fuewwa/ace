#include "ewmh.h"

#include <X11/Xatom.h>

namespace ewmh {

namespace {

Window g_check_window = None;

}  // namespace

void Initialize(Display* display, Window root) {
    g_check_window = XCreateSimpleWindow(display, root, -1, -1, 1, 1, 0, 0, 0);

    Atom net_supporting_wm_check = XInternAtom(display, "_NET_SUPPORTING_WM_CHECK", False);
    Atom net_wm_name = XInternAtom(display, "_NET_WM_NAME", False);
    Atom utf8_string = XInternAtom(display, "UTF8_STRING", False);
    Atom net_supported = XInternAtom(display, "_NET_SUPPORTED", False);
    Atom net_client_list = XInternAtom(display, "_NET_CLIENT_LIST", False);
    Atom net_active_window = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    Atom net_wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom net_wm_state_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

    XChangeProperty(
        display, root, net_supporting_wm_check, XA_WINDOW, 32,
        PropModeReplace, reinterpret_cast<unsigned char*>(&g_check_window), 1);
    XChangeProperty(
        display, g_check_window, net_supporting_wm_check, XA_WINDOW, 32,
        PropModeReplace, reinterpret_cast<unsigned char*>(&g_check_window), 1);

    const char* wm_name = "ace";
    XChangeProperty(
        display, g_check_window, net_wm_name, utf8_string, 8,
        PropModeReplace, reinterpret_cast<const unsigned char*>(wm_name), 3);

    Atom supported[] = {
        net_supported,
        net_supporting_wm_check,
        net_client_list,
        net_active_window,
        net_wm_name,
        net_wm_state,
        net_wm_state_fullscreen,
    };
    XChangeProperty(
        display, root, net_supported, XA_ATOM, 32,
        PropModeReplace, reinterpret_cast<unsigned char*>(supported),
        sizeof(supported) / sizeof(supported[0]));

    UpdateClientList(display, root, {});
    UpdateActiveWindow(display, root, None);
}

void UpdateClientList(Display* display, Window root, const std::vector<Window>& windows) {
    Atom net_client_list = XInternAtom(display, "_NET_CLIENT_LIST", False);

    XChangeProperty(
        display, root, net_client_list, XA_WINDOW, 32,
        PropModeReplace,
        reinterpret_cast<unsigned char*>(const_cast<Window*>(windows.data())),
        static_cast<int>(windows.size()));
}

void UpdateActiveWindow(Display* display, Window root, Window window) {
    Atom net_active_window = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);

    XChangeProperty(
        display, root, net_active_window, XA_WINDOW, 32,
        PropModeReplace, reinterpret_cast<unsigned char*>(&window), 1);
}

void SetFullscreen(Display* display, Window window, bool fullscreen) {
    Atom net_wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom net_wm_state_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

    if (fullscreen) {
        XChangeProperty(
            display, window, net_wm_state, XA_ATOM, 32,
            PropModeReplace, reinterpret_cast<unsigned char*>(&net_wm_state_fullscreen), 1);
    } else {
        XChangeProperty(
            display, window, net_wm_state, XA_ATOM, 32,
            PropModeReplace, nullptr, 0);
    }
}

bool ShouldTile(Display* display, Window window) {
    Atom net_wm_window_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);

    const char* skipped_names[] = {
        "_NET_WM_WINDOW_TYPE_DOCK",
        "_NET_WM_WINDOW_TYPE_DESKTOP",
        "_NET_WM_WINDOW_TYPE_SPLASH",
        "_NET_WM_WINDOW_TYPE_MENU",
        "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
        "_NET_WM_WINDOW_TYPE_POPUP_MENU",
        "_NET_WM_WINDOW_TYPE_TOOLTIP",
        "_NET_WM_WINDOW_TYPE_NOTIFICATION",
        "_NET_WM_WINDOW_TYPE_COMBO",
        "_NET_WM_WINDOW_TYPE_DND",
    };

    Atom actual_type = None;
    int actual_format = 0;
    unsigned long item_count = 0;
    unsigned long bytes_after = 0;
    unsigned char* data = nullptr;

    int status = XGetWindowProperty(
        display, window, net_wm_window_type, 0, 32, False, XA_ATOM,
        &actual_type, &actual_format, &item_count, &bytes_after, &data);

    bool tile = true;
    if (status == Success && data != nullptr) {
        if (item_count > 0) {
            Atom first_type = reinterpret_cast<Atom*>(data)[0];
            for (const char* name : skipped_names) {
                if (first_type == XInternAtom(display, name, False)) {
                    tile = false;
                    break;
                }
            }
        }
        XFree(data);
    }

    return tile;
}

bool ResolveFullscreenRequest(Display* display, const XClientMessageEvent& e,
                               bool currently_fullscreen, bool* want_fullscreen) {
    Atom net_wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom net_wm_state_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

    if (static_cast<Atom>(e.message_type) != net_wm_state) {
        return false;
    }

    Atom first_property = static_cast<Atom>(e.data.l[1]);
    Atom second_property = static_cast<Atom>(e.data.l[2]);
    if (first_property != net_wm_state_fullscreen && second_property != net_wm_state_fullscreen) {
        return false;
    }

    const long kNetWmStateRemove = 0;
    const long kNetWmStateAdd = 1;
    const long kNetWmStateToggle = 2;

    long action = e.data.l[0];
    if (action == kNetWmStateAdd) {
        *want_fullscreen = true;
    } else if (action == kNetWmStateRemove) {
        *want_fullscreen = false;
    } else if (action == kNetWmStateToggle) {
        *want_fullscreen = !currently_fullscreen;
    } else {
        return false;
    }

    return true;
}

}  // namespace ewmh
