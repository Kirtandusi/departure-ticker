# Detect platform
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Compiler
CC = gcc

# rpi-rgb-led-matrix location
RPI_RGB = /home/kirtandusi/rpi-rgb-led-matrix

# Base include paths
INCLUDE_DIRS = -Iinclude -I$(RPI_RGB)/include

# Base library dirs + libs
LIB_DIRS = -L$(RPI_RGB)/lib
LIBS = -lcurl -lcjson -lrgbmatrix -lstdc++ -lrt -lm -lpthread

# Platform-specific settings
ifeq ($(UNAME_S),Darwin)     # macOS
INCLUDE_DIRS += -I/opt/homebrew/include
LIB_DIRS += -L/opt/homebrew/lib
endif

ifeq ($(UNAME_S),Linux)      # Linux (e.g., Raspberry Pi)
ifeq ($(UNAME_M),aarch64)    # Raspberry Pi 5 (64-bit)
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
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean