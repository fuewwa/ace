SOURCES := src/main.cpp src/window_manager.cpp src/config.cpp src/ewmh.cpp src/process.cpp src/workspaces.cpp
OBJECTS := $(SOURCES:.cpp=.o)

TARGET := ace

BIN := /usr/bin
XSESSIONS := /usr/share/xsessions
DESKTOP := assets/ace.desktop

$(TARGET): $(OBJECTS)
	g++ -std=c++17 -Wall -Wextra -O2 -Iinclude -o $(TARGET) $(OBJECTS) -lX11

src/%.o: src/%.cpp
	g++ -std=c++17 -Wall -Wextra -O2 -Iinclude -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

ifdef DISABLED
run: $(TARGET)
	@command -v Xephyr >/dev/null 2>&1 || { echo "Xephyr not found. Install it first (e.g. sudo pacman -S xorg-server-xephyr)."; exit 1; }
	-pkill Xephyr 2>/dev/null
	-pkill -f "DISPLAY=:1 ./$(TARGET)" 2>/dev/null
	sleep 0.5
	Xephyr :1 -screen 1280x800 & \
	sleep 1; \
	DISPLAY=:1 ./$(TARGET) & \
	echo "Xephyr and ace are running on DISPLAY=:1"
endif

install: $(TARGET)
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo "Please run this as root (sudo make install)."; \
		exit 1; \
	fi
	install -m 755 $(TARGET) $(BIN)/$(TARGET)
	@echo "Installed $(BIN)/$(TARGET)"
	@if [ ! -f $(DESKTOP) ]; then \
		echo "Desktop file not found at $(DESKTOP)."; \
		exit 1; \
	fi
	mkdir -p $(XSESSIONS)
	install -m 644 $(DESKTOP) $(XSESSIONS)/$(TARGET).desktop
	@echo "Installed $(XSESSIONS)/$(TARGET).desktop"
	@echo "Done. Select ace from your display manager session list."

uninstall:
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo "Please run this as root (sudo make uninstall)."; \
		exit 1; \
	fi
	@if [ -f $(BIN)/$(TARGET) ]; then \
		rm -f $(BIN)/$(TARGET); \
		echo "Removed $(BIN)/$(TARGET)"; \
	else \
		echo "$(BIN)/$(TARGET) not found, skipping"; \
	fi
	@if [ -f $(XSESSIONS)/$(TARGET).desktop ]; then \
		rm -f $(XSESSIONS)/$(TARGET).desktop; \
		echo "Removed $(XSESSIONS)/$(TARGET).desktop"; \
	else \
		echo "$(XSESSIONS)/$(TARGET).desktop not found, skipping"; \
	fi
	@echo "Done."

help:
	@echo "Usage:"
	@echo "  make            - Build ace"
	@echo "  make clean      - Remove object files"
	@echo "  sudo make install   - Install ace and the xsession entry"
	@echo "  sudo make uninstall - Remove ace and the xsession entry"

.PHONY: clean help run install uninstall
