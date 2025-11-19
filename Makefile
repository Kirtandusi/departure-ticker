CC = gcc
CFLAGS = -Wall -Wextra -I./src

SRC = src/main.c src/networking.c src/parser.c src/render.c
OBJ = $(SRC:.c=.o)
TARGET = bus_ticker

LIBS = -lcurl

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
