CC ?= cc
PKG_CONFIG ?= pkg-config
MATRIX ?= 0
RPI_RGB ?= /opt/rpi-rgb-led-matrix
MODE := $(if $(filter 1,$(MATRIX)),matrix,console)
BUILD_DIR ?= build/$(MODE)
TARGET := $(BUILD_DIR)/ticker

CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude $(shell $(PKG_CONFIG) --cflags libcurl libcjson)
LDLIBS += $(shell $(PKG_CONFIG) --libs libcurl libcjson) -lm
SRC := src/main.c src/config.c src/networking.c src/parser.c src/render.c
OBJ := $(SRC:src/%.c=$(BUILD_DIR)/%.o)

ifeq ($(MATRIX),1)
CPPFLAGS += -DENABLE_MATRIX -I$(RPI_RGB)/include
LDLIBS += $(RPI_RGB)/lib/librgbmatrix.a -lstdc++ -lrt -lpthread
MATRIX_TEST := $(BUILD_DIR)/test_matrix
endif

.PHONY: all clean test lint sanitize check
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/test_core: tests/test_core.c src/config.c src/parser.c src/networking.c src/render.c $(wildcard include/*.h)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

$(BUILD_DIR)/test_network: tests/test_network.c src/networking.c include/networking.h include/parser.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DREQUEST_TIMEOUT_SECONDS=1L $(LDFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

$(BUILD_DIR)/test_matrix: tests/test_matrix.c src/render.c include/render.h include/parser.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

test: $(TARGET) $(BUILD_DIR)/test_core $(BUILD_DIR)/test_network $(MATRIX_TEST)
	$(BUILD_DIR)/test_core
	python3 tests/test_integration.py $(BUILD_DIR)/ticker $(BUILD_DIR)/test_network
ifeq ($(MATRIX),1)
	$(MATRIX_TEST)
endif

lint:
	$(CC) $(CPPFLAGS) $(CFLAGS) -Werror -fsyntax-only $(SRC) tests/test_core.c tests/test_network.c

check: lint test

sanitize:
	$(MAKE) MATRIX=0 BUILD_DIR=build/sanitize CFLAGS='-std=c11 -Wall -Wextra -Wpedantic -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' LDFLAGS='-fsanitize=address,undefined' check

clean:
	rm -rf build

-include $(OBJ:.o=.d)
