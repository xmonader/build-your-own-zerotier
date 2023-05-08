#ifndef __PUNCH_H
#define __PUNCH_H

#include <stdint.h>

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

#define MSGTYPE_REGISTER_REQUEST 0x10000001
#define MSGTYPE_MEMBER_BROADCAST 0x10000002
#define MSGTYPE_HANDSHAKE_REQUEST 0x10000003
#define MSGTYPE_HANDSHAKE_REPLY 0x10000004
#define MSGTYPE_P2P_PACKET 0x10000005

struct PUNCH_MEMBER
{
  struct sockaddr_in addr;
  struct sockaddr_in vaddr;
};

struct PUNCH_HANDSHAKE
{
  struct PUNCH_MEMBER from_member;
  struct PUNCH_MEMBER to_member;
};

struct PUNCH_MESSAGE
{
  uint32_t type;
  int32_t size;
  char data[0];
};

#endif /* __PUNCH_H */