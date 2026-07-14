#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    const char *method = getenv("REQUEST_METHOD");
    if (!method)
        method = "UNKNOWN";

    printf("Content-Type: text/plain\r\n");
    printf("Status: 200 OK\r\n");
    printf("\r\n");
    printf("Hello from compiled C CGI!\n");
    printf("Request Method: %s\n", method);
    return (0);
}
