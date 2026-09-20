CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -g -O0
TARGET  := bin/server
SRCS    := $(wildcard src/*.c)

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
	docker build -t http-calclator-server .

shell: image
	docker run --rm \
		-it \
		-v $(PWD):${PWD} \
		-w ${PWD} \
		http-calclator-server \
		bash

docker-build: image
	docker run --rm \
		-it \
		-v $(PWD):${PWD} \
		-w ${PWD} \
		http-calclator-server \
		sh -c 'make build'

docker-run: image
	docker run --rm \
		-it -p 8080:8080 \
		-v $(PWD):${PWD} \
		-w ${PWD} \
		http-calclator-server \
		sh -c 'make build && make run'

docker-compdb: image
	docker run --rm \
		-it \
		-v $(PWD):${PWD} \
		-w ${PWD} \
		http-calclator-server \
		sh -c 'make compdb'
