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

# Windows-compatible directory paths
ifeq ($(PLATFORM),WINDOWS)
    BUILD_DIR_WIN := $(subst /,\,$(BUILD_DIR))
    OBJ_DIR_WIN := $(subst /,\,$(OBJ_DIR))
endif

# Source files (exclude unity_build.c from regular build)
C_SOURCES := $(filter-out $(SRC_DIR)/unity_build.c,$(wildcard $(SRC_DIR)/*.c))
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
	@if not exist "$(BUILD_DIR_WIN)" mkdir "$(BUILD_DIR_WIN)"
	@if not exist "$(OBJ_DIR_WIN)" mkdir "$(OBJ_DIR_WIN)"
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
	@if exist "$(BUILD_DIR_WIN)" rmdir /S /Q "$(BUILD_DIR_WIN)"
else
	$(RMDIR) $(BUILD_DIR)
endif

# Run
run: all
	$(BUILD_DIR)/$(EXE)

# ============================================================================
# Unity Build - Compile all sources as a single translation unit
# Benefits: Faster compilation, better optimization, smaller binary
# ============================================================================

UNITY_FILE := $(SRC_DIR)/unity_build.c

unity: dirs
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) $(UNITY_FILE) -o $(BUILD_DIR)/$(EXE) $(LDFLAGS) $(LDLIBS)
	@echo Unity build complete: $(BUILD_DIR)/$(EXE)

# ============================================================================
# Raylib Installation
# Automatically downloads and installs Raylib if not present
# ============================================================================

RAYLIB_VERSION := 5.0
RAYLIB_REPO := https://github.com/raysan5/raylib.git

check-raylib:
ifeq ($(PLATFORM),WINDOWS)
	@where raylib.h >nul 2>&1 || echo Raylib not found in PATH. Run 'make install-raylib' to install.
else
	@pkg-config --exists raylib 2>/dev/null || echo "Raylib not found. Run 'make install-raylib' to install."
endif

install-raylib:
ifeq ($(PLATFORM),WINDOWS)
	@echo ============================================
	@echo Installing Raylib for Windows...
	@echo ============================================
	@echo.
	@echo Option 1: Using winget (recommended)
	@echo   winget install raysan5.raylib
	@echo.
	@echo Option 2: Manual installation
	@echo   1. Download from https://github.com/raysan5/raylib/releases
	@echo   2. Extract to C:\raylib\raylib
	@echo   3. Set RAYLIB_PATH environment variable
	@echo.
	@echo Option 3: Build from source (requires git and mingw)
	@powershell -Command "if (!(Test-Path 'C:\raylib')) { New-Item -ItemType Directory -Path 'C:\raylib' }"
	@cd /d C:\raylib && git clone --depth 1 --branch $(RAYLIB_VERSION) $(RAYLIB_REPO) 2>nul || echo Raylib already cloned
	@cd /d C:\raylib\raylib\src && mingw32-make PLATFORM=PLATFORM_DESKTOP
	@echo Raylib installed to C:\raylib\raylib
else ifeq ($(PLATFORM),LINUX)
	@echo ============================================
	@echo Installing Raylib for Linux...
	@echo ============================================
	@echo Installing dependencies...
	sudo apt-get update
	sudo apt-get install -y build-essential git cmake
	sudo apt-get install -y libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
	@echo Cloning and building Raylib...
	@if [ ! -d "/tmp/raylib" ]; then git clone --depth 1 --branch $(RAYLIB_VERSION) $(RAYLIB_REPO) /tmp/raylib; fi
	cd /tmp/raylib/src && sudo make install PLATFORM=PLATFORM_DESKTOP
	@echo Raylib installed successfully!
else ifeq ($(PLATFORM),MACOS)
	@echo ============================================
	@echo Installing Raylib for macOS...
	@echo ============================================
	@echo Using Homebrew (recommended)...
	brew install raylib
	@echo.
	@echo If Homebrew is not installed, run:
	@echo   /bin/bash -c "$$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
	@echo Then run 'make install-raylib' again.
endif
	@echo.
	@echo ============================================
	@echo Raylib installation complete!
	@echo Run 'make' to build RouteCraft.
	@echo ============================================

# ============================================================================
# Help
# ============================================================================

help:
	@echo ============================================
	@echo RouteCraft Build System
	@echo ============================================
	@echo.
	@echo Available targets:
	@echo   make              - Build the application (standard build)
	@echo   make unity        - Unity build (faster, single compilation unit)
	@echo   make debug        - Build with debug symbols
	@echo   make run          - Build and run the application
	@echo   make clean        - Remove build artifacts
	@echo   make unity-clean  - Remove unity build file
	@echo.
	@echo Raylib:
	@echo   make check-raylib   - Check if Raylib is installed
	@echo   make install-raylib - Download and install Raylib
	@echo.
	@echo Platform: $(PLATFORM)
	@echo Compiler: $(CC)
	@echo.

# Phony targets
.PHONY: all dirs clean debug run unity check-raylib install-raylib help

