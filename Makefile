# Compiler and flags
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -Iinclude

# Directories
SRC_DIR  := src
OBJ_DIR  := build
BIN_DIR  := bin

# Target binary
TARGET   := $(BIN_DIR)/map_veto_server

# All .cpp files under src/
SRCS     := $(wildcard $(SRC_DIR)/*.cpp)

# Object files (build/main.o, build/state.o, ...)
OBJS     := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Default rule
all: $(TARGET)

# Link
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

# Compile each .cpp -> build/*.o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ensure dirs exist
$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(BIN_DIR):
	mkdir $(BIN_DIR)

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
