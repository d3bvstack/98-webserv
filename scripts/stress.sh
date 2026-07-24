#!/usr/bin/env bash
#
# stress.sh - Procedural HTTP stress tester matching C structures
#

# --------------------------------------------------------------------------
# GLOBAL CONSTANTS & CONFIGURATION (Global Variables)
# --------------------------------------------------------------------------
HOST="127.0.0.1"
PORT="9898"
TOTAL_REQUESTS=1000
CONCURRENCY=100
SOCKET_TIMEOUT=5

# Request Payloads
GET_PATH="/"
POST_PATH="/files"
POST_BODY="hello"
CGI_PATH="/cgi-bin/py/hello.py"

# Global pointer to the shared output file descriptor equivalent
RESULTS_FILE=""

# --------------------------------------------------------------------------
# FUNCTIONS
# --------------------------------------------------------------------------

# Equivalent to: char* send_raw_request(char* method, char* path, char* body)
# Returns HTTP status code (e.g. "200") or "ERR" via standard output.
send_raw_request() {
    method="$1"
    path="$2"
    body="$3"
    body_len=${#body}

    # 1. Format the HTTP request header and body (similar to writing to a socket descriptor)
    # 2. Pipe into netcat (raw TCP socket)
    # 3. Read the first response line (similar to recv())
    response=$({
        printf '%s %s HTTP/1.1\r\n' "$method" "$path"
        printf 'Host: %s\r\n' "$HOST"
        if [[ "$body_len" -gt 0 ]]; then
            printf 'Content-Length: %d\r\n' "$body_len"
        fi
        printf 'Connection: close\r\n\r\n'
        if [[ "$body_len" -gt 0 ]]; then
            printf '%s' "$body"
        fi
    } | nc -w "$SOCKET_TIMEOUT" "$HOST" "$PORT" 2>/dev/null | head -n 1)

    # Extract 3 consecutive digits (HTTP status code)
    status=$(echo "$response" | grep -oE '[0-9]{3}' | head -n 1)

    if [[ -z "$status" ]]; then
        echo "ERR"  # Socket read timeout or connection refused
    else
        echo "$status"
    fi
}

# Equivalent to: void dispatch_request(int index)
# Selects and executes the correct request using basic modulo division
dispatch_request() {
    index="$1"
    modulo=$((index % 3))

    if [[ "$modulo" -eq 0 ]]; then
        send_raw_request "GET" "$GET_PATH" ""
    elif [[ "$modulo" -eq 1 ]]; then
        send_raw_request "POST" "$POST_PATH" "$POST_BODY"
    else
        send_raw_request "GET" "$CGI_PATH" ""
    fi
}

# Equivalent to worker process logic: void run_worker(int requests_to_send)
run_worker() {
    requests_to_send="$1"
    i=0
    
    while [[ "$i" -lt "$requests_to_send" ]]; do
        # Appends output to the shared file descriptor.
        # UNIX guarantees atomic appends for small payloads (under 4KB).
        dispatch_request "$i" >> "$RESULTS_FILE"
        i=$((i + 1))
    done
}

# Equivalent to: void cleanup()
cleanup() {
    if [[ -f "$RESULTS_FILE" ]]; then
        rm -f "$RESULTS_FILE"
    fi
}

# Equivalent to: int main(int argc, char** argv)
main() {
    # Allocate temporary resources
    RESULTS_FILE="$(mktemp)"
    
    # Register signal handlers for cleanup (like sigaction() / atexit())
    trap cleanup EXIT INT TERM

    printf "Starting stress test on http://%s:%s\n" "$HOST" "$PORT"
    printf "Total requests: %d | Concurrency: %d\n" "$TOTAL_REQUESTS" "$CONCURRENCY"
    printf "%s\n" "------------------------------------------------------------"

    start_time="$(date +%s)"

    # Divide loop iterations evenly among workers
    req_per_worker=$((TOTAL_REQUESTS / CONCURRENCY))
    remainder=$((TOTAL_REQUESTS % CONCURRENCY))

    # Fork worker processes (equivalent to a loop of fork() / pthread_create())
    w=0
    while [[ "$w" -lt "$CONCURRENCY" ]]; do
        n="$req_per_worker"
        if [[ "$w" -lt "$remainder" ]]; then
            n=$((n + 1))
        fi
        
        # The '&' operator forks the run_worker function into the background
        run_worker "$n" &
        w=$((w + 1))
    done

    # Block parent thread until all child processes complete (equivalent to wait(NULL) loop)
    wait

    end_time="$(date +%s)"
    elapsed=$((end_time - start_time))
    if [[ "$elapsed" -eq 0 ]]; then
        elapsed=1
    fi

    # Read and aggregate counts from RESULTS_FILE safely
    total_sent=$(wc -l < "$RESULTS_FILE" | tr -d ' ')
    success_count=$(grep -cE '^[23][0-9][0-9]$' "$RESULTS_FILE" || true)
    error_count=$(grep -cE '^[45][0-9][0-9]$' "$RESULTS_FILE" || true)
    failed_count=$(grep -c '^ERR$' "$RESULTS_FILE" || true)
    
    # Calculate average throughput rate
    rps=$((total_sent / elapsed))

    # Output report
    printf "Results:\n"
    printf "  Total Sent          : %d\n" "$total_sent"
    printf "  Successes (2xx/3xx) : %d\n" "$success_count"
    printf "  Errors (4xx/5xx)    : %d\n" "$error_count"
    printf "  Connection Failures : %d\n" "$failed_count"
    printf "  Elapsed Time        : %ds\n" "$elapsed"
    printf "  Average Throughput  : %d req/s\n" "$rps"
    printf "%s\n" "------------------------------------------------------------"
}

# Trigger entry point execution
main "$@"