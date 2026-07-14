#!/usr/bin/python3
# Tests CGI error handling via query-string arguments.
#
# Usage:
#   /py/error.py?mode=status&code=404
#   /py/error.py?mode=exit&code=1
#
# mode=status  → Sends a non-200 Status header with the given code
# mode=exit    → Prints a response then calls sys.exit(code)
# (default)    → Shows usage help
import os, sys, urllib.parse

STATUS_REASONS = {
    400: "Bad Request",
    401: "Unauthorized",
    403: "Forbidden",
    404: "Not Found",
    405: "Method Not Allowed",
    408: "Request Timeout",
    418: "I'm a Teapot",
    429: "Too Many Requests",
    500: "Internal Server Error",
    502: "Bad Gateway",
    503: "Service Unavailable",
    504: "Gateway Timeout",
}

qs = urllib.parse.parse_qs(os.environ.get("QUERY_STRING", ""))
mode = qs.get("mode", [""])[0]
code_str = qs.get("code", [""])[0]

if mode == "status":
    try:
        code = int(code_str)
    except ValueError:
        code = 500
    reason = STATUS_REASONS.get(code, "Error")
    print(f"Status: {code} {reason}")
    print("Content-Type: text/html\n")
    print(f"<html><body><h1>{code} {reason}</h1>")
    print(f"<p>The server returned HTTP status code {code}.</p></body></html>")

elif mode == "exit":
    try:
        code = int(code_str)
    except ValueError:
        code = 1
    print("Content-Type: text/html\n")
    print("<html><body><h1>CGI Exit Test</h1>")
    print(f"<p>This script is about to call sys.exit({code}).</p>")
    print("<p>If you see this, the server sent the output before the exit.</p>")
    print("</body></html>")
    sys.stdout.flush()
    sys.exit(code)

else:
    print("Content-Type: text/html\n")
    print("<html><body><h1>Error Handling Test Script</h1>")
    print("<h2>Available modes:</h2>")
    print("<h3>status</h3>")
    print("<p>Returns a non-200 HTTP status code.</p>")
    print("<pre>/py/error.py?mode=status&amp;code=404</pre>")
    print("<h3>exit</h3>")
    print("<p>Prints output then calls sys.exit() with the given code.</p>")
    print("<pre>/py/error.py?mode=exit&amp;code=1</pre>")
    print("</body></html>")
