# Detect platform
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Compiler
CC = gcc

# Base include paths
INCLUDE_DIRS = -Iinclude -Iinclude/proto

# Base library dirs + libs
LIB_DIRS =
LIBS = -lprotobuf-c -lcurl -lcjson

# Platform-specific settings
ifeq ($(UNAME_S),Darwin)     # macOS
    INCLUDE_DIRS += -I/opt/homebrew/include
    LIB_DIRS += -L/opt/homebrew/lib
endif

ifeq ($(UNAME_S),Linux)      # Linux (e.g., Raspberry Pi)
    ifeq ($(UNAME_M),aarch64)  # Raspberry Pi 5 (64-bit)
        # Optional: if libraries are installed in custom locations, set here
        # INCLUDE_DIRS += -I/opt/vc/include
        # LIB_DIRS += -L/opt/vc/lib
        CFLAGS += -march=armv8-a
    endif
endif

# Compiler flags
CFLAGS = -Wall -Wextra -O2 -g $(INCLUDE_DIRS)
LDFLAGS = $(LIB_DIRS) $(LIBS)

# Source files
SRC = \
    src/main.c \
    src/networking.c \
    src/parser.c \
    src/render.c

# Object files
OBJ = $(SRC:.c=.o)

# Output binary
TARGET = ticker

# Default target
all: $(TARGET)

# Link objects into executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
