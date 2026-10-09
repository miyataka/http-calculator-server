#!/usr/bin/env bash
# HTTP サーバーの結合テスト．サーバーを起動して curl / nc で叩き，
# ステータスコードと body を確認する．
#
# 実行: make test （unit-test の後にこれが走る）
set -euo pipefail

SERVER_BIN="./bin/server"
HOST=127.0.0.1
PORT=8080
BASE="http://$HOST:$PORT"

cleanup() {
  if [[ -n "${SERVER_PID:-}" ]]; then
    kill "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

# build
make build

# 別のサーバーがポートを使っていると，そちらをテストしてしまうので止める
if nc -z "$HOST" "$PORT" 2>/dev/null; then
  echo "port $PORT is already in use; stop the other server first" >&2
  exit 1
fi

# start server
"$SERVER_BIN" >/dev/null 2>&1 &
SERVER_PID=$!

# 起動待ち
for _ in {1..50}; do
  if nc -z "$HOST" "$PORT" 2>/dev/null; then
    break
  fi
  sleep 0.1
done

failed=0

# expect_http <name> <expected_status> <expected_body> <url>
expect_http() {
  local name=$1 want_status=$2 want_body=$3 url=$4
  local out status body
  # body と status を1回のリクエストで取る．最終行が status
  out=$(curl -s -o - -w '\n%{http_code}' "$url")
  status=${out##*$'\n'}
  body=${out%$'\n'*}

  if [[ "$status" == "$want_status" && "$body" == "$want_body" ]]; then
    printf '  %-44s ok\n' "$name"
  else
    printf '  %-44s FAILED\n' "$name"
    printf '      url:    %s\n' "$url"
    printf '      expect: %s %q\n' "$want_status" "$want_body"
    printf '      got:    %s %q\n' "$status" "$body"
    failed=1
  fi
}

# expect_raw <name> <expected_status_line_prefix> <raw_request>
# curl では送れない壊れたリクエストを nc で生で送る
expect_raw() {
  local name=$1 want=$2 raw=$3
  local first_line
  first_line=$(printf '%b' "$raw" | nc -w 1 "$HOST" "$PORT" | head -n 1 | tr -d '\r')

  if [[ "$first_line" == "$want"* ]]; then
    printf '  %-44s ok\n' "$name"
  else
    printf '  %-44s FAILED\n' "$name"
    printf '      expect: %q\n' "$want"
    printf '      got:    %q\n' "$first_line"
    failed=1
  fi
}

echo "routing"
expect_http "GET / returns fixed body"            200 "hello"      "$BASE/"
expect_http "GET /nope is 404"                    404 "Not Found"  "$BASE/nope"
expect_http "GET /calculator does not match /calc" 404 "Not Found"  "$BASE/calculator"

echo "calc"
expect_http "percent-encoded plus adds"           200 "3"          "$BASE/calc?q=1%2B2"
expect_http "minus subtracts"                     200 "7"          "$BASE/calc?q=10-3"
expect_http "asterisk multiplies"                 200 "20"         "$BASE/calc?q=4*5"
expect_http "slash divides"                       200 "3"          "$BASE/calc?q=9/3"
expect_http "spaces around operator are ignored"  200 "3"          "$BASE/calc?q=1+%2B+2"
expect_http "negative lhs is accepted"            200 "1"          "$BASE/calc?q=-1%2B2"
expect_http "division truncates toward zero"      200 "-3"         "$BASE/calc?q=-7/2"

echo "calc errors"
expect_http "missing q is 400"                    400 "Bad Request" "$BASE/calc"
expect_http "division by zero is 400"             400 "Bad Request" "$BASE/calc?q=1/0"
expect_http "non-numeric operands are 400"        400 "Bad Request" "$BASE/calc?q=abc"
expect_http "number without operator is 400"      400 "Bad Request" "$BASE/calc?q=42"
expect_http "raw plus (= space) is 400"           400 "Bad Request" "$BASE/calc?q=1+2"
expect_http "overflow is 400"                     400 "Bad Request" "$BASE/calc?q=9223372036854775807%2B1"
expect_http "invalid percent escape is 400"       400 "Bad Request" "$BASE/calc?q=%zz"

echo "malformed request"
expect_raw  "request line without version is 400" "HTTP/1.1 400" 'GET /calc\r\n\r\n'
expect_raw  "garbage request line is 400"         "HTTP/1.1 400" 'garbage\r\n\r\n'

if [[ $failed -ne 0 ]]; then
  echo "integration test FAILED" >&2
  exit 1
fi
echo "integration test passed"
