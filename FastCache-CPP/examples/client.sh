#!/usr/bin/env bash
set -euo pipefail

HOST="${1:-127.0.0.1}"
PORT="${2:-8080}"

if ! command -v nc >/dev/null 2>&1; then
  echo "netcat (nc) is required"
  exit 1
fi

{
  printf 'PING\n'
  printf 'SET user:1001 Anant\n'
  printf 'GET user:1001\n'
  printf 'SET session active EX 5\n'
  printf 'GET session\n'
  printf 'STATS\n'
  printf 'QUIT\n'
} | nc "$HOST" "$PORT"
