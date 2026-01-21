# RouteCraft - Cross-platform Makefile
# Detects OS and builds accordingly

# Detect OS
ifeq ($(OS),Windows_NT)
    PLATFORM := WINDOWS
    EXE := routecraft.exe
    RM := del /Q
    RMDIR := rmdir /S /Q
    MKDIR := mkdir
    SEP := \\
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM := LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
        PLATFORM := MACOS
    endif
    EXE := routecraft
    RM := rm -f
    RMDIR := rm -rf
    MKDIR := mkdir -p
    SEP := /
endif

# Compiler settings
CC := gcc
CXX := g++
CFLAGS := -Wall -Wextra -std=c11 -O2
CXXFLAGS := -Wall -Wextra -std=c++17 -O2
DEBUG_FLAGS := -g -DDEBUG

# Directories
SRC_DIR := src
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

# Source files
C_SOURCES := $(wildcard $(SRC_DIR)/*.c)
CPP_SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SOURCES))
OBJECTS += $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(CPP_SOURCES))

# Raylib settings
ifeq ($(PLATFORM),WINDOWS)
    RAYLIB_PATH ?= C:/raylib/raylib
    INCLUDE_PATHS := -I$(RAYLIB_PATH)/src -I$(SRC_DIR)
    LDFLAGS := -L$(RAYLIB_PATH)/src
    LDLIBS := -lraylib -lopengl32 -lgdi32 -lwinmm
else ifeq ($(PLATFORM),LINUX)
    INCLUDE_PATHS := -I/usr/local/include -I$(SRC_DIR)
    LDFLAGS := -L/usr/local/lib
    LDLIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
else ifeq ($(PLATFORM),MACOS)
    INCLUDE_PATHS := -I/usr/local/include -I$(SRC_DIR)
    LDFLAGS := -L/usr/local/lib
    LDLIBS := -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
endif

# Default target
all: dirs $(BUILD_DIR)/$(EXE)

# Create directories
dirs:
ifeq ($(PLATFORM),WINDOWS)
	@if not exist $(BUILD_DIR) $(MKDIR) $(BUILD_DIR)
	@if not exist $(OBJ_DIR) $(MKDIR) $(OBJ_DIR)
else
	@$(MKDIR) $(BUILD_DIR) $(OBJ_DIR)
endif

# Link
$(BUILD_DIR)/$(EXE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS)

# Compile C files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) -c $< -o $@

# Compile C++ files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATHS) -c $< -o $@

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: all

# Clean
clean:
ifeq ($(PLATFORM),WINDOWS)
	@if exist $(BUILD_DIR) $(RMDIR) $(BUILD_DIR)
else
	$(RMDIR) $(BUILD_DIR)
endif

# Run
run: all
	$(BUILD_DIR)/$(EXE)

# Phony targets
.PHONY: all dirs clean debug run

