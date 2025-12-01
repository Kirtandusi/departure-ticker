# Detect platform
UNAME_S := $(shell uname -s)

# Compiler
CC = gcc

# Base include paths
INCLUDE_DIRS = -Iinclude -Iinclude/proto

# Base library dirs + libs
LIB_DIRS =
LIBS = -lprotobuf-c -lcurl

# Platform-specific settings
ifeq ($(UNAME_S), Darwin)     # macOS
    INCLUDE_DIRS += -I/opt/homebrew/include
    LIB_DIRS += -L/opt/homebrew/lib
endif

ifeq ($(UNAME_S), Linux)      # Raspberry Pi
    # Linux typically needs no extra paths
endif

# Flags
CFLAGS = -Wall -Wextra -O2 $(INCLUDE_DIRS)
LDFLAGS = $(LIB_DIRS) $(LIBS)

# Source files
SRC = \
    src/main.c \
    src/networking.c \
    src/parser.c \
    src/render.c \
    include/proto/gtfs-realtime.pb-c.c

# Object files
OBJ = $(SRC:.c=.o)

# Output binary
TARGET = ticker

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
