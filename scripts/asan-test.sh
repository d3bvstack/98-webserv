#!/usr/bin/env bash
set -eu

LOG_DIR='.logs/asan'
LOG_FILE="$LOG_DIR/asan-$(date +%Y%m%d-%H%M%S).log"

mkdir -p "$LOG_DIR"

echo "Logging ASAN output to $LOG_FILE"
exec > >(tee "$LOG_FILE") 2>&1

# Build with AddressSanitizer and run a short smoke test.
CXXFLAGS="-Wall -Wextra -Werror -std=c++98 -pedantic -fsanitize=address -g"
export ASAN_OPTIONS="detect_leaks=1:exitcode=21"

cleanup() 
{
  echo "Cleaning up ASAN build artifacts..."
  make fclean
}

# Execute cleanup on script exit
trap cleanup EXIT


echo "Building with ASAN..."
make clean
make CXXFLAGS="$CXXFLAGS" re

echo "Running webserv under timeout..."
if command -v timeout >/dev/null; then
  set +e #Changes script behavior to continue running even if a command fails
  timeout --foreground -s INT 3 ./bin/webserv
  STATUS=$?
  set -e

  if grep -q -E "AddressSanitizer:|LEAK SUMMARY:|detected memory leaks" "$LOG_FILE"; then
    echo "[ERROR][MEMORY] ASAN detected memory leaks, check ${LOG_FILE}"
    exit 1
  fi

  if [[ "$STATUS" -eq 0 ]] || [[ "$STATUS" -eq 124 ]] || [[ "$STATUS" -eq 130 ]]; then
    echo "ASAN smoke test finished."
    exit 0
  else
    echo "[ERROR] webserv crashed or failed with an internal error. CODE: $STATUS"
    exit 1
  fi
else
  echo '[ERROR] Command "timeout" does not exist.'
  exit 1
fi
