# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -I./include
LDFLAGS = -lsqlite3

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = .

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

# Target executable
TARGET = $(BIN_DIR)/task_scheduler.exe

# Create directories
$(OBJ_DIR):
	@if not exist "$(OBJ_DIR)" mkdir "$(OBJ_DIR)"

# Main target
all: $(OBJ_DIR) $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CXX)	$(OBJS) -o $@	$(LDFLAGS)

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX)	$(CXXFLAGS) -c $< -o $@

# Clean
clean:
	@if exist "$(OBJ_DIR)" rd /s /q "$(OBJ_DIR)"
	@if exist "$(TARGET)" del /q "$(TARGET)"

.PHONY: all clean

# Include dependencies
-include $(DEPS)