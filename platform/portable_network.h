#ifndef _PORTABLE_NETWORK_H
#define _PORTABLE_NETWORK_H

#ifdef _WIN32
/* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 /* Windows XP. */
#endif
#include <winsock2.h>
#include <Ws2tcpip.h>
#else
typedef int SOCKET;

/* Assume that any non-Windows platform uses POSIX-style sockets instead. */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h> /* Needed for close() */
#include <fcntl.h>  /* Needed for fcntl() */
#include <errno.h>  /* Needed for errno */
#endif

int portable_init_environment();
int portable_make_socket_nonblocking(SOCKET sock);
int portable_close_socket(SOCKET sock);
int portable_is_socket_valid(SOCKET sock);
int portable_is_EWOULDBLOCK_or_EAGAIN();
const char* portable_socket_api_strerror();

#endif