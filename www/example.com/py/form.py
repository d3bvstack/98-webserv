#!/usr/bin/python3
# Shows form data sent by the client.
# GET: reads from the query string in the URL.
# POST: reads from the request body.
# Test with:
#   curl "http://host:port/py/form.py?name=alice"
#   curl -d "name=alice" http://host:port/py/form.py
import os, sys, urllib.parse
print("Content-Type: text/html\n")
print("<html><body>")
print("<h1>Form Data</h1>")
method = os.environ.get("REQUEST_METHOD", "GET")
params = {}
if method == "POST":
    cl = int(os.environ.get("CONTENT_LENGTH", 0))
    body = sys.stdin.read(cl) if cl > 0 else ""
    params = urllib.parse.parse_qs(body)
else:
    qs = os.environ.get("QUERY_STRING", "")
    params = urllib.parse.parse_qs(qs)
print(f"<p>Method: {method}</p>")
print("<h2>Parameters:</h2><ul>")
for k, v in params.items():
    for val in v:
        print(f"<li><b>{k}</b>: {val}</li>")
print("</ul></body></html>")
