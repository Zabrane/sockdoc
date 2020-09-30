/* max-accept
 For testing what happens when a max number of file descriptors
 has been exceeded.
 Example:
    max-accept 7777
 All this does is sit endlessly accepting all incoming connections
 until it hits an error.

 One way to trigger this error is running `max-connect` program from
 multiple other systems in order to overload this system.

 Based on `tcp-srv-one`, but with everything after `accept()` removed.
 */
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

//#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
    struct addrinfo *ai = NULL;
    struct addrinfo hints = {0};
    int err;
    int fd = -1;
    int yes = 1;
    char hostaddr[NI_MAXHOST];
    char hostport[NI_MAXSERV];
        
    /* Ignore the send() problem */
    signal(SIGPIPE, SIG_IGN);

    if (argc < 2 || 3 < argc) {
        fprintf(stderr, "[-] usage: max-accept <port> [address]\n");
        return -1;
    }
    
    /* Get an address structure for the port */
    hints.ai_flags = AI_PASSIVE;
    err = getaddrinfo((argc==3)?argv[2]:0,  /* local address*/
                      argv[1],              /* local port number */
                      &hints,               /* hints */
                      &ai);                 /* result */
    if (err) {
        fprintf(stderr, "[-] getaddrinfo(): %s\n", gai_strerror(err));
        return -1;
    }

    /* And retrieve back again which addresses were assigned */
    err = getnameinfo(ai->ai_addr, ai->ai_addrlen,
                        hostaddr, sizeof(hostaddr),
                        hostport, sizeof(hostport),
                        NI_NUMERICHOST | NI_NUMERICSERV);
    if (err) {
        fprintf(stderr, "[-] getnameinfo(): %s\n", gai_strerror(err));
        goto cleanup;
    }
    
    /* Create a file handle for the kernel resources */
    fd = socket(ai->ai_family, SOCK_STREAM, 0);
    if (fd == -1) {
        fprintf(stderr, "[-] socket(): %s\n", strerror(errno));
        goto cleanup;
    }

    /* Allow multiple processes to share this IP address */
    err = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (err) {
        fprintf(stderr, "[-] SO_REUSEADDR([%s]:%s): %s\n", hostaddr, hostport, strerror(errno));
        goto cleanup;
    }
    
#if defined(SO_REUSEPORT)
    /* Allow multiple processes to share this port */
    err = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
    if (err) {
        fprintf(stderr, "[-] SO_REUSEPORT([%s]:%s): %s\n", hostaddr, hostport, strerror(errno));
        goto cleanup;
    }
#endif

    /* Tell it to use the local port number (and optionally, address) */
    err = bind(fd, ai->ai_addr, ai->ai_addrlen);
    if (err) {
        fprintf(stderr, "[-] bind([%s]:%s): %s\n", hostaddr, hostport, strerror(errno));
        goto cleanup;
    }

    /* Configure the socket for listening (i.e. accepting incoming connections) */
    err = listen(fd, 10);
    if (err) {
        fprintf(stderr, "[-] listen([%s]:%s): %s\n", hostaddr, hostport, strerror(errno));
        goto cleanup;
    } else
        fprintf(stderr, "[+] listening on [%s]:%s\n", hostaddr, hostport);
    

    /* Allocate a structure to hold the incoming address */
    sa_max = sizeof(struct sockaddr_in);
    if (sa_max < sizeof(struct sockaddr_in6))
        sa_max = sizeof(struct sockaddr_in6);
    sa = malloc(sa_max);

    /* Loop accepting incoming connections */
    for (;;) {
        int fd2;
        struct sockaddr_storage addr;
        socklen_t addrlen = sizeof(addr);
        char peeraddr[NI_MAXHOST];
        char peerport[NI_MAXSERV];
    
        /* Wait until somebody connects to us */
        fd2 = accept(fd, &addr, &addrlen);
        if (fd2 == -1) {
            fprintf(stderr, "[-] accept([%s]:%s): %s\n", hostaddr, hostport, strerror(errno));
            continue;
        }

        /* Pretty print the incoming address/port */
        err = getnameinfo(&addr, addrlen,
                        peeraddr, sizeof(peeraddr),
                        peerport, sizeof(peerport),
                        NI_NUMERICHOST | NI_NUMERICSERV);
        if (err) {
            fprintf(stderr, "[-] getnameinfo(): %s\n", gai_strerror(err));
            goto cleanup;
        }
        fprintf(stderr, "[+] accept([%s]:%s) from [%s]:%s\n", hostaddr, hostport, peeraddr, peerport);

        /* Loop on this connection receiving/transmitting data */
        for (;;) {
            char buf[512];
            ptrdiff_t bytes_received;
            ptrdiff_t bytes_sent;

            /* Wait until some bytes received or connection closed */
            bytes_received = recv(fd2, buf, sizeof(buf), 0);
            if (bytes_received == 0) {
                fprintf(stderr, "[+] close() from [%s]:%s\n", peeraddr, peerport);
                break;
            } else if (bytes_received == -1) {
                fprintf(stderr, "[-] error from [%s]:%s\n", peeraddr, peerport);
                break;
            } else
                fprintf(stderr, "[+] recv([%s]:%s) %d bytes\n", peeraddr, peerport, (int)bytes_received);
                

            /* Echo back to sender */
            bytes_sent = send(fd2, buf, bytes_received, 0);
            if (bytes_sent == -1) {
                fprintf(stderr, "[-] send([%s]:%s): %s\n", peeraddr, peerport, strerror(errno));
                break;
            } else
                fprintf(stderr, "[+] send([%s]:%s) %d bytes\n", peeraddr, peerport, (int)bytes_sent);
        }
     
        close(fd2);
    }

    
cleanup:
    if (fd > 0)
        close(fd);
    if (ai)
        freeaddrinfo(ai);
    return 0;
}

