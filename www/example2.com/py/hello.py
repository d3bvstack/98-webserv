#!/usr/bin/python3
# Same as example.com/py/hello.py but uses "(example2.com)" in the message.
# Confirms which vhost handled the request.
print("Content-Type: text/html\n")
print("<html><body><h1>Hello from Python CGI! (example2.com)</h1></body></html>")
