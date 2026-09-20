#!/usr/bin/env bash
set -euo pipefail

SERVER_BIN="./bin/server"
PORT=8080

cleanup() {
  if [[ -n "${SERVER_PID:-}" ]]; then
    kill "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
  fi
}

trap cleanup EXIT INT TERM

# build
make

# start server
"$SERVER_BIN" "$PORT" &
SERVER_PID=$!

# 起動待ち
for _ in {1..50}; do
  if nc -z 127.0.0.1 "$PORT" 2>/dev/null; then
    break
  fi
  sleep 0.1
done

# test
response=$(printf 'hello' | nc 127.0.0.1 "$PORT")

if [[ "$response" != "hello" ]]; then
  echo "test failed: expected 'hello', got '$response'" >&2
  exit 1
fi

echo "test passed"
