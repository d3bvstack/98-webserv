#!/usr/bin/python3
# Shows all environment variables the server passed to this script.
# Use this to debug what the server sends (request method, headers, paths).
import os
print("Content-Type: text/plain\n")
for k, v in sorted(os.environ.items()):
    print(f"{k}={v}")
