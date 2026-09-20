#include <cstdio>
#include <cstdlib>
#include <string>

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include "config.h"
#include "window_manager.h"

namespace {

std::string LockPath() {
    const char* runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    if (runtime_dir != nullptr && runtime_dir[0] != '\0') {
        return std::string(runtime_dir) + "/wm.lock";
    }
    return "/tmp/wm-" + std::to_string(getuid()) + ".lock";
}

bool AcquireInstanceLock() {
    int fd = open(LockPath().c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
    if (fd < 0) {
        return false;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        close(fd);
        return false;
    }
    return true;
}

}

int main() {
    if (!AcquireInstanceLock()) {
        std::fputs("some window manager is already running\n", stderr);
        return 1;
    }

    EnsureConfigExists();

    WindowManager* wm = WindowManager::Create();
    if (wm == nullptr) {
        return 1;
    }
    wm->Run();
    delete wm;
    return 0;
}
