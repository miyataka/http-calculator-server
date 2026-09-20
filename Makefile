CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -g -O0
TARGET  := bin/server
SRCS    := $(wildcard src/*.c)

COMPOSE := docker compose
RUN     := $(COMPOSE) run --rm --service-ports dev

.PHONY: build run clean compdb image shell docker-build docker-run docker-compdb

# ---- コンテナ内 (または Linux ホスト) で実行するターゲット ----
build: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^

run: build
	./$(TARGET)

# clangd 用の compile_commands.json を生成
compdb: clean
	bear -- $(MAKE) build

clean:
	rm -rf bin/* compile_commands.json

# ---- ホスト (macOS) から Docker 経由で実行するターゲット ----
image:
	$(COMPOSE) build

shell:
	$(RUN)

docker-build:
	$(RUN) make build

docker-run:
	$(RUN) make run

docker-compdb:
	$(RUN) make compdb
