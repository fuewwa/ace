#ifndef ACE_KEYBINDS_H_
#define ACE_KEYBINDS_H_

#include <X11/Xlib.h>
#include <string>
#include <vector>

namespace keybinds {

enum class Action {
    kExec,
    kClose,
    kQuit,
    kFullscreen,
    kFocusNext,
    kFocusPrevious,
    kSwapNext,
    kSwapPrevious,
    kPromote,
    kMasterShrink,
    kMasterGrow,
    kWorkspace,
    kMoveToWorkspace,
    kReload,
};

struct Binding {
    KeySym keysym = NoSymbol;
    unsigned int modifiers = 0;
    Action action = Action::kExec;
    std::string argument;
    int index = 0;
};

std::vector<Binding> Load();

}

#endif
