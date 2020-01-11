
#ifdef WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <errno.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tests/regression/socket.h>
#include <lib/libplctag.h>


int socket_open(const char *host, const char *port)
{
	//int status;
	struct addrinfo addr_hints;
    struct addrinfo *addr_info = NULL;
	int sock;
    int sock_opt = 0;
    int rc;

#ifdef _WIN32
	/* Windows needs special initialization. */
	WSADATA winsock_data;
	rc = WSAStartup(MAKEWORD(2, 2), &winsock_data);

	if (rc != NO_ERROR) {
		printf("WSAStartup failed with error: %d\n", rc);
		return PLCTAG_ERR_CREATE;
	}
#endif

    /*
     * Set up the hints for the type of socket that we want.
     */

    /* make sure the whole struct is set to 0 bytes */
	memset(&addr_hints, 0, sizeof addr_hints);

    /*
     * From the man page (node == host name here):
     *
     * "If the AI_PASSIVE flag is specified in hints.ai_flags, and node is NULL, then
     * the returned socket addresses will be suitable for bind(2)ing a socket that
     * will  accept(2) connections.   The returned  socket  address  will  contain
     * the  "wildcard  address" (INADDR_ANY for IPv4 addresses, IN6ADDR_ANY_INIT for
     * IPv6 address).  The wildcard address is used by applications (typically servers)
     * that intend to accept connections on any of the host's network addresses.  If
     * node is not NULL, then the AI_PASSIVE flag is ignored.
     *
     * If the AI_PASSIVE flag is not set in hints.ai_flags, then the returned socket
     * addresses will be suitable for use with connect(2), sendto(2), or sendmsg(2).
     * If node is NULL, then the  network address  will  be  set  to the loopback
     * interface address (INADDR_LOOPBACK for IPv4 addresses, IN6ADDR_LOOPBACK_INIT for
     * IPv6 address); this is used by applications that intend to communicate with
     * peers running on the same host."
     *
     * So we can get away with just setting AI_PASSIVE.
     */
    addr_hints.ai_flags = AI_PASSIVE;

    /* this allows for both IPv4 and IPv6 */
    addr_hints.ai_family = AF_UNSPEC;

    /* we want a TCP socket.  And we want it now! */
    addr_hints.ai_socktype = SOCK_STREAM;

    /* Get the address info about the local system, to be used later. */
    rc = getaddrinfo(host, port, &addr_hints, &addr_info);
    if (rc != 0) {
        printf("ERROR: getaddrinfo() failed: %s\n", gai_strerror(rc));
        exit(PLCTAG_ERR_CREATE);
    }

    /* finally, finally, finally, we get to open a socket! */
    sock = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);

    if (sock < 0) {
        printf("ERROR: socket() failed: %s\n", gai_strerror(sock));
        exit(PLCTAG_ERR_CREATE);
    }

    /* if this is going to be a server socket, bind it. */
    if(!host) {
        rc = bind(sock, addr_info->ai_addr, addr_info->ai_addrlen);
        if (rc < 0)	{
            printf("ERROR: Unable to bind() socket: %s\n", gai_strerror(rc));
            exit(PLCTAG_ERR_CREATE);
        }

        rc = listen(sock, 10);
        if(rc < 0) {
            printf("ERROR: Unable to call listen() on socket: %s\n", gai_strerror(rc));
            exit(PLCTAG_ERR_CREATE);
        }

        /* set up our socket to allow reuse if we crash suddenly. */
        sock_opt = 1;
        rc = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&sock_opt, sizeof(sock_opt));
        if(rc) {
            socket_close(sock);
            printf("ERROR: Setting SO_REUSEADDR on socket failed: %s\n", gai_strerror(rc));
            exit(PLCTAG_ERR_CREATE);
        }
    } else {
        struct timeval timeout; /* used for timing out connections etc. */
        struct linger so_linger; /* used to set up short/no lingering after connections are close()ed. */

#ifdef SO_NOSIGPIPE
        /* On *BSD and macOS, set the socket option to prevent SIGPIPE. */
        rc = setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (char*)&sock_opt, sizeof(sock_opt));
        if(rc) {
            socket_close(sock);
            printf("ERROR: Setting SO_REUSEADDR on socket failed: %s\n", gai_strerror(rc));
            exit(PLCTAG_ERR_CREATE);
        }
#endif

        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        rc = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        if(rc) {
            socket_close(sock);
            printf("ERROR: Setting SO_RCVTIMEO on socket failed: %s\n", gai_strerror(rc));
            exit(PLCTAG_ERR_CREATE);
        }

        rc = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
        if(rc) {
            socket_close(sock);
            printf("ERROR: Setting SO_SNDTIMEO on socket failed: %s\n", gai_strerror(rc));
            exit(PLCTAG_ERR_CREATE);
        }

        /* abort the connection on close. */
        so_linger.l_onoff = 1;
        so_linger.l_linger = 0;

        rc = setsockopt(sock, SOL_SOCKET, SO_LINGER,(char*)&so_linger,sizeof(so_linger));
        if(rc) {
            socket_close(sock);
            printf("ERROR: Setting SO_LINGER on socket failed: %s\n", gai_strerror(rc));
            exit(PLCTAG_ERR_CREATE);
        }
    }

    /* free the memory for the address info struct. */
    freeaddrinfo(addr_info);

    return sock;
}



void socket_close(int sock)
{
#ifdef WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}


slice_s socket_read(int sock, slice_s in_buf)
{
    int rc = (int)recv(sock, (char *)in_buf.data, (size_t)in_buf.len, 0);

    if(rc < 0) {
#ifdef WIN32
        rc = WSAGetLastError();
        if(rc == WSAEWOULDBLOCK) {
#else
        rc = errno;
        if(rc == EAGAIN || rc == EWOULDBLOCK) {
#endif
            rc = 0;
        } else {
            printf("Socket read error rc=%d.\n", rc);
            rc = PLCTAG_ERR_READ;
        }
    }

    return ((rc>=0) ? slice_trunc(in_buf, rc) : slice_make_err(rc));
}