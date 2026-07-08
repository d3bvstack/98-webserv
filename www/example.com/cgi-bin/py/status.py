#!/usr/bin/python3
# Returns a 418 "I'm a Teapot" status code instead of 200 OK.
# Tests whether the server respects the CGI Status header.
print("Status: 418 I'm a Teapot")
print("Content-Type: text/html\n")
print("<html><body><h1>418 I'm a Teapot</h1><p>I'm a little teapot, short and stout.</p></body></html>")
