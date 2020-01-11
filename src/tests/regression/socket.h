#pragma once

#include <tests/regression/slice.h>



extern int socket_open(const char *host, const char *port);
extern void socket_close(int sock);
extern slice_s socket_read(int sock, slice_s in_buf);

