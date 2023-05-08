#include "portable_network.h"
#include "misc/logger.h"

int portable_make_socket_nonblocking(SOCKET sock) {
#ifdef _WIN32
  u_long mode = 1;
  int ret;
  if ((ret = ioctlsocket(sock, FIONBIO, &mode)) != 0) {
    LOG_DEBUG("fail to ioctlsocket, return code %d", ret);
    return -1;
  }
#else
  int flags;
  if ((flags = fcntl(sock, F_GETFL, 0)) == -1) {
    LOG_DEBUG("fail to fcntl, %s", strerror(errno));
    return -1;
  }
  flags |= O_NONBLOCK;
  if ((flags = fcntl(sock, F_SETFL, flags)) == -1) {
    LOG_DEBUG("fail to fcntl, %s", strerror(errno));
    return -1;
  }
#endif

  return 0;
}

int portable_is_socket_valid(SOCKET sock) {
#ifdef _WIN32
  return sock != INVALID_SOCKET;
#else
  return sock >= 0;
#endif
}

int portable_close_socket(SOCKET sock) {
#ifdef _WIN32
  return closesocket(sock);
#else
  return close(sock);
#endif
}

int portable_init_environment() {
#ifdef _WIN32
  WSADATA wsaData;
  int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    LOG_DEBUG("fail to WSAStartup, return code %d", iResult);
    return 1;
  }
  return iResult;
#else
  return 0;
#endif
}

int portable_is_EWOULDBLOCK_or_EAGAIN() {
#ifdef _WIN32
  int iError = WSAGetLastError();
  LOG_DEBUG("portable_is_EWOULDBLOCK_or_EAGAIN, iError code %d", iError);
  return iError == WSAEWOULDBLOCK;
#else
  return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

const char* portable_socket_api_strerror() {
#ifdef _WIN32
  static char error_str[32];
  sprintf(error_str, "WSA_Socket_Error_Code[%d]", WSAGetLastError());
  return error_str;
#else
  return strerror(errno);
#endif
}