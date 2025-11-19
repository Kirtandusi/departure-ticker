# Compiler
CC = gcc

# Paths
INCLUDE_DIRS = -Iinclude -Iinclude/proto -I/opt/homebrew/include
LIB_DIRS = -L/opt/homebrew/lib

# Libraries
LIBS = -lprotobuf-c -lcurl

# Compiler flags
CFLAGS = -Wall -Wextra $(INCLUDE_DIRS)
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

# Compile rules
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
