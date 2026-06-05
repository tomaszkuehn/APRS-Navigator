# APRS Engine v2 — Top-level Makefile
# Builds engine library, tests, and GUI client

CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -O2 -funsigned-char
LDFLAGS := -lm -lpthread

ENGINE_DIR    := engine
ENGINE_INC    := -I$(ENGINE_DIR)/include -I$(ENGINE_DIR)/src
ENGINE_LIB    := $(ENGINE_DIR)/build/libaprs_engine.a

TRANSPORT_DIR := transport
TEST_DIR      := tests
CLIENT_DIR    := clients/gui

.PHONY: all engine test gui http-server ws-server clean run-gui run-http run-ws

all: engine test gui http-server ws-server

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

http-server: engine
	$(CC) -std=gnu99 -Wall -Wextra -O2 -funsigned-char $(ENGINE_INC) \
		$(TRANSPORT_DIR)/http/http_server.c $(TRANSPORT_DIR)/http/main.c \
		$(ENGINE_LIB) $(LDFLAGS) -o $(TRANSPORT_DIR)/http/aprs_http_server

run-http: http-server
	$(TRANSPORT_DIR)/http/aprs_http_server

ws-server: engine
	$(CC) -std=gnu99 -Wall -Wextra -O2 -funsigned-char $(ENGINE_INC) \
		$(TRANSPORT_DIR)/websocket/ws_server.c $(TRANSPORT_DIR)/websocket/ws_main.c \
		$(ENGINE_LIB) $(LDFLAGS) -o $(TRANSPORT_DIR)/websocket/aprs_ws_server

run-ws: ws-server
	$(TRANSPORT_DIR)/websocket/aprs_ws_server

clean:
	cd $(ENGINE_DIR) && $(MAKE) clean
	rm -f $(TEST_DIR)/test_engine_api
	rm -f $(CLIENT_DIR)/aprs_gui
	rm -f $(TRANSPORT_DIR)/http/aprs_http_server
	rm -f $(TRANSPORT_DIR)/websocket/aprs_ws_server
