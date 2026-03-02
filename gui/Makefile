# Define the compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++17 -I gui/External
OPTIMIZATION_FLAGS = -O2 -flto=auto
DEBUG_FLAGS = -g -O0
LIBS = -lglfw -ldl # this might have to be lglfw3 instead
#LDFLAGS = -flto

ifneq ("$(wildcard $(/usr/lib/x86_64-linux-gnu/libglfw3.so))","") # Check if glfw3 exists
	LIBS = -lglfw3 # and use it instead
endif

# glfw and OpenGL are required to compile!
#sudo apt install libglfw3-dev

# If I even decide to switch to libusb:
#sudo apt-get install libusb-1.0-0-dev

# Define the source files
SOURCES = main.cpp gui.cpp DriverHelper.cpp ImGuiExtensions.cpp FunctionHelper.cpp CustomCurve.cpp ConfigHelper.cpp $(wildcard External/ImGui/*.cpp) $(wildcard gui/lib/*.cpp)

# Define the object files
OBJECTS = $(patsubst %.cpp, %.o, $(SOURCES))

# Define the target
TARGET = YeetMouseGui

default: all

all: $(TARGET)

# Rule to build the target
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION_FLAGS) -o $@ $(OBJECTS) -L/usr/lib/x86_64-linux-gnu -lGL $(LIBS)

# Rule to build the object files with LTO
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION_FLAGS) -c $< -o $@

# Rule to build the object files with LTO (only for ImGui source files)
External/ImGui/%.o: External/ImGui/%.cpp
	$(CXX) $(CXXFLAGS) $(OPTIMIZATION_FLAGS) -c $< -o $@


.PHONY: clean clean-all debug

clean-all: clean
	rm -rf External/ImGui/*.o

# Removing the imgui object files is kinda pointless for most cases and it takes a lot of time to rebuild
clean:
	rm -rf *.o
	rm $(TARGET)

debug:
	@echo "Building GUI for debugging..."
	$(MAKE) all OPTIMIZATION_FLAGS="$(DEBUG_FLAGS)"
