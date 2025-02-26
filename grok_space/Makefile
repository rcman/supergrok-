# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -Iinclude -std=c11 `sdl2-config --cflags`
LDFLAGS := -lm -lstdc++ -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2_gfx

# Directories
SRC_DIR := src
INCLUDE_DIR := include
RELEASE_DIR := release

# Output executable
TARGET := $(RELEASE_DIR)/game

# Find all source files in src/
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(RELEASE_DIR)/%.o)

# Default target
all: $(TARGET)

# Compile the source files into object files
$(RELEASE_DIR)/%.o: $(SRC_DIR)/%.cpp | $(RELEASE_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link the object files to create the final executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Ensure the release directory exists
$(RELEASE_DIR):
	mkdir -p $(RELEASE_DIR)

# Clean up build files
clean:
	rm -rf $(RELEASE_DIR)/*.o $(TARGET)

# Run the game
run: all
	./$(TARGET)

.PHONY: all clean run

