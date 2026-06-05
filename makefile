# APRS Engine v2 — Top-level Makefile
# Builds engine library, tests, and GUI client

CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -O2 -funsigned-char
LDFLAGS := -lm

ENGINE_DIR    := engine
ENGINE_INC    := -I$(ENGINE_DIR)/include -I$(ENGINE_DIR)/src
ENGINE_LIB    := $(ENGINE_DIR)/build/libaprs_engine.a

TRANSPORT_DIR := transport
TEST_DIR      := tests
CLIENT_DIR    := clients/gui

.PHONY: all engine test gui clean run-gui

all: engine test gui

engine:
	cd $(ENGINE_DIR) && $(MAKE)

test: engine
	$(CC) $(CFLAGS) $(ENGINE_INC) $(TEST_DIR)/test_engine_api.c \
		$(ENGINE_LIB) $(LDFLAGS) -o $(TEST_DIR)/test_engine_api
	@echo "=== Running tests ==="
	$(TEST_DIR)/test_engine_api

gui: engine
	$(CC) -std=gnu99 -Wall -Wextra -O2 -funsigned-char $(ENGINE_INC) \
		$(CLIENT_DIR)/gui_main.c $(CLIENT_DIR)/renderer.c \
		$(ENGINE_LIB) $(LDFLAGS) -o $(CLIENT_DIR)/aprs_gui

run-gui: gui
	$(CLIENT_DIR)/aprs_gui

clean:
	cd $(ENGINE_DIR) && $(MAKE) clean
	rm -f $(TEST_DIR)/test_engine_api
	rm -f $(CLIENT_DIR)/aprs_gui
