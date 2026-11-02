CXX ?= clang++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Isrc

SRCS = src/main.cpp \
       src/art/AsciiArt.cpp \
       src/core/Commands.cpp \
       src/core/Process.cpp \
       src/core/Scheduler.cpp \
       src/core/SystemConfig.cpp \
       src/ui/Console.cpp

TARGET = process-scheduler

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
