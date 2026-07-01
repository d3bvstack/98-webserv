#!/usr/bin/python3
# Same as example.com/py/env.py but for the example2.com vhost.
# Run on both vhosts to compare SERVER_NAME, SERVER_PORT, etc.
import os
print("Content-Type: text/plain\n")
for k, v in sorted(os.environ.items()):
    print(f"{k}={v}")
