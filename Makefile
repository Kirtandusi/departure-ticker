CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lcurl

SRC = main.c networking.c parser.c render.c
OBJ = $(SRC:.c=.o)
TARGET = bus_board

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
