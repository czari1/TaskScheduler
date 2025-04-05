# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -I./include -I"C:/curl/curl-8.13.0_1-win64-mingw/include" 
LDFLAGS = -L"C:/curl/curl-8.13.0_1-win64-mingw/lib" -lsqlite3 -lcurl -lwinmm -lws2_32 -lwldap32 -pthread

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

debug: CXXFLAGS += -g -O0
debug: clean all

.PHONY: all clean debug

# Include dependencies
-include $(DEPS)