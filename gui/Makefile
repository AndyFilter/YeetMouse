# Define the compiler and flags
CXX ?= g++
CXXFLAGS = -std=c++17 -I . -isystem External

# ImPlot has few of these
IMGUIFLAGS = -Wformat=0 -Wno-format-security
ifneq (,$(findstring clang,$(CXX)))
    # ImGui produces many of these
    IMGUIFLAGS += -Wno-nontrivial-memcall
endif

WARNFLAGS = -Wall -Wno-sign-compare
OPTIMIZATION_FLAGS = -O2 -flto=auto
DEBUG_FLAGS = -g -O0

PKG_CONFIG ?= pkg-config
GLFW_CFLAGS := $(shell $(PKG_CONFIG) --cflags glfw3 2>/dev/null)
GLFW_LIBS := $(shell $(PKG_CONFIG) --libs glfw3 2>/dev/null)
OPENGL_LIBS := -lGL -ldl

# Define the source files
SOURCES = main.cpp gui.cpp DriverHelper.cpp ImGuiExtensions.cpp FunctionHelper.cpp CustomCurve.cpp ConfigHelper.cpp $(wildcard External/ImGui/*.cpp) $(wildcard gui/lib/*.cpp)

# Define the object files
OBJECTS = $(patsubst %.cpp, %.o, $(SOURCES))

# Define the target
TARGET = YeetMouseGui

default: all

all: check_deps $(TARGET)

check_deps:
	@command -v $(PKG_CONFIG) >/dev/null 2>&1 || { \
		echo "Missing dependency: pkg-config"; \
		echo "Please install pkg-config and the GLFW3 development files, then try again."; \
		exit 1; \
	}
	@$(PKG_CONFIG) --exists glfw3 || { \
		echo "Missing dependency: glfw3 development files"; \
		echo "Please install GLFW3 development files for your distribution, then try again."; \
		exit 1; \
	}

# Rule to build the target
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(GLFW_CFLAGS) $(OPTIMIZATION_FLAGS) $(WARNFLAGS) -o $@ $(OBJECTS) $(GLFW_LIBS) $(OPENGL_LIBS)

# Rule to build the object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(GLFW_CFLAGS) $(OPTIMIZATION_FLAGS) $(WARNFLAGS) -c $< -o $@

# Rule to build the object files for ImGui source files
External/ImGui/%.o: External/ImGui/%.cpp
	$(CXX) $(CXXFLAGS) $(GLFW_CFLAGS) $(OPTIMIZATION_FLAGS) $(IMGUIFLAGS) -c $< -o $@

.PHONY: all check_deps clean clean-all debug

clean-all: clean
	rm -rf External/ImGui/*.o

# Removing the imgui object files is kinda pointless for most cases and it takes a lot of time to rebuild
clean:
	rm -rf *.o
	rm -f $(TARGET)

debug:
	@echo "Building GUI for debugging..."
	$(MAKE) all OPTIMIZATION_FLAGS="$(DEBUG_FLAGS)"
