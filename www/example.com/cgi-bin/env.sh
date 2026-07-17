#!/bin/sh
# Shows all environment variables the server passed to this script.
# Use this to debug what the server sends (request method, headers, paths).

printf "Content-Type: text/plain\r\n\r\n"

env | sort
