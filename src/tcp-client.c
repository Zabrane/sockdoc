/* tcp-client
 Simple example of writing a client program using TCP.
 Example usage:
    tcp-client www.google.com 80
 This will send an HTTP request, then dump the response it gets
 back from the server.
 */
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

static const char *my_http_request = "HEAD / HTTP/1.0\r\n"
                                     "User-Agent: tcp_client/0.0\r\n"
                                     "\r\n";

int
main(int argc, char *argv[])
{
    struct addrinfo *addresses = NULL;
    struct addrinfo *ai;
    int err;
    int fd = -1;
    ptrdiff_t count;
    const char *hostname;
    const char *portname;

    /* Ignore the send() problem */
    signal(SIGPIPE, SIG_IGN);

    if (argc != 3) {
        fprintf(stderr, "[-] usage: tcp-client <host> <port>\n");
        return -1;
    }
    hostname = argv[1];
    portname = argv[2];

    /* Do a DNS lookup on the name */
    err = getaddrinfo(hostname, portname, 0, &addresses);
    if (err) {
        fprintf(stderr, "[-] getaddrinfo(): %s\n", gai_strerror(err));
        return -1;
    } else {
        count = 0;
        for (ai = addresses; ai; ai = ai->ai_next)
            count++;
        if (count == 0) {
            fprintf(stderr, "[-] getaddrinfo(): returned zero addresses\n");
            goto cleanup;
        } else {
            fprintf(stderr, "[+] getaddrinfo(): returned %d addresses\n", (int)count);
        }
    }

    /* Of the several DNS results, keep trying to connect until  we get one that
     * works */
    for (ai = addresses; ai; ai = ai->ai_next) {
        char addrname[64];
        char portname[8];

        /* Print the address/port to strings for logging/debugging  */
        err = getnameinfo(ai->ai_addr, ai->ai_addrlen, addrname,
            sizeof(addrname), portname, sizeof(portname),
            NI_NUMERICHOST | NI_NUMERICSERV);
        if (err) {
            fprintf(stderr, "[-] getnameinfo(): %s\n", gai_strerror(err));
            goto cleanup;
        }

        /* Create a socket */
        fd = socket(ai->ai_family, SOCK_STREAM, 0);
        if (fd == -1) {
            fprintf(stderr, "[-] socket(): %s\n", strerror(errno));
            goto cleanup;
        }

        /* Try to connect */
        fprintf(stderr, "[ ] connecting to [%s]:%s\n", addrname, portname);
        err = connect(fd, ai->ai_addr, ai->ai_addrlen);
        if (err) {
            fprintf(stderr, "[-] connect([%s]:%s): %s\n", addrname, portname,
                strerror(errno));
            close(fd);
            fd = -1;
            continue;
        } else {
            fprintf(stderr, "[+] connect([%s]:%s): %s\n", addrname, portname,
                "succeeded");
            /* got one that works, so break out of loop */
            break;
        }
    }
    if (fd == -1) {
        fprintf(stderr, "[-] error: no successful connection\n");
        goto cleanup;
    }

    /* The 'fd' socket now has a valid connection to the server, so send the
     * HTTP request */
    count = send(fd, my_http_request, strlen(my_http_request), 0);
    if (count == -1) {
        fprintf(stderr, "[-] send(): %s\n", strerror(errno));
        goto cleanup;
    }
    fprintf(stderr, "[+] send(): sent %d bytes\n", (int)count);

    /* Now dump all the bytes in response */
    for (;;) {
        unsigned char buf[512];
        ssize_t i;
        count = recv(fd, &buf, 512, 0);
        if (count == 0)
            break; /* opposite side closed connection */
        else if (count < 0) {
            fprintf(stderr, "recv(): %s\n", strerror(errno));
            break;
        }

        for (i = 0; i < count; i++) {
            unsigned char c = buf[i];
            if (isprint(c) || isspace(c))
                printf("%c", c);
            else
                printf(".");
        }
    }

cleanup:
    freeaddrinfo(addresses);
    if (fd != -1)
        close(fd);
}
