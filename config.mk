SOURCES := src/main.cpp src/window_manager.cpp src/config.cpp src/ewmh.cpp src/process.cpp src/workspaces.cpp
OBJECTS := $(SOURCES:.cpp=.o)

TARGET := ace

BIN := /usr/bin
XSESSIONS := /usr/share/xsessions
DESKTOP := assets/ace.desktop
