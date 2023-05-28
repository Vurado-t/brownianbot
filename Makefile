BUILD_DIR := ./out
SERVER_EXE_PATH := "$(BUILD_DIR)/brownianbotd"
CLIENT_EXE_PATH := "$(BUILD_DIR)/brownianbot"


.SILENT: all build build-server build-client create-out-dir

all: build

build: build-server build-client

build-server: create-out-dir
	gcc ./src/server_daemon/server_daemon.c \
		./src/sdk/error/error.c \
		./src/sdk/log/log.c \
		./src/sdk/config/config.c \
		./src/sdk/file/file_utils.c \
		./src/sdk/socket/socket_utils.c \
		./src/sdk/server/server.c \
		-o $(SERVER_EXE_PATH) -std=gnu11

build-client: create-out-dir
	gcc ./src/client_cli/client_cli.c \
		./src/sdk/error/error.c \
		./src/sdk/log/log.c \
		./src/sdk/config/config.c \
		./src/sdk/file/file_utils.c \
		./src/sdk/socket/socket_utils.c \
		-o $(CLIENT_EXE_PATH) -std=gnu11

create-out-dir:
	mkdir -p $(BUILD_DIR)