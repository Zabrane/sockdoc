/* tcp-srv-one
 Simple example of recieving a TCP connection.
 This is an 'echo' server that echoes back whatever it receives.
 Example usage:
    tcp-srv-one 7777
 This will listen on port 7777, accept one connection at a time,
 and echo back whatever it receives on the connection.
 */
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static const char *my_http_response = 
    "HTTP/1.0 200 OK\r\n"
    "Server: a-tcp-srv/1.0\r\n"
    "Content-type: text/html\r\n"
    "Content-Length: 41\r\n"
    "\r\n"
    "<title>a-tcp-srv</title>\r\n"
    "Hello, world.\r\n";

void
print_string(const unsigned char *buf, ssize_t count) {
    ssize_t i;
    if (count <= 0)
        return;
    for (i = 0; i < count; i++) {
            unsigned char c = buf[i];
            if (isprint(c) || isspace(c))
                printf("%c", c);
            else
                printf("."); /* non-printable characters */
        }
}

int main(int argc, char *argv[])
{
    struct addrinfo *local = NULL;
    struct addrinfo hints = {0};
    int err;
    int fd = -1;
    const char *portname = argv[1];
    const char *hostname = (argc>=3)?argv[2]:0;
    unsigned char buf[65536];
    ssize_t count;
        
    if (argc < 2 || 3 < argc) {
        fprintf(stderr, "[-] usage: a-tcp-srv <port> [address]\n");
        return -1;
    }
    
    hints.ai_flags = AI_PASSIVE;
    err = getaddrinfo(hostname, portname, &hints, &local);
    fd = socket(local->ai_family, SOCK_STREAM, 0);
    err = bind(fd, local->ai_addr, local->ai_addrlen);
    err = listen(fd, 10);
    sleep(3600);
    while (err == 0) {
        int fd2 = accept(fd, 0, 0);
        count = recv(fd2, buf, sizeof(buf), 0);
        print_string(buf, count);    
        count = send(fd2, my_http_response, strlen(my_http_response), 0);
        close(fd2);
    }
    close(fd);
    return 0;
}

