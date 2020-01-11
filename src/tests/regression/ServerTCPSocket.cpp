#include "ServerTCPSocket.h"

#ifdef _WIN32
    // include Windows-specific headers
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    // POSIX-ish headers.
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

network::TCPSocket::TCPSocket()
{
}

network::TCPSocket::~TCPSocket()
{
}

