CXX = g++
CXXFLAGS = -O2 -Wall -std=c++17 -isystem ../../gui/External

SRC = main.cpp  \
      ../../gui/ConfigHelper.cpp \
      ../../gui/DriverHelper.cpp \
      ../../gui/CustomCurve.cpp

OBJ = $(SRC:.cpp=.o)

yeetmousectl: $(OBJ)
	$(CXX) -o yeetmousectl $(OBJ)

default: all
all: yeetmousectl

.PHONY: clean

clean:
	rm -f $(OBJ) yeetmousectl