#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include "misc/utils.h"
#include "misc/logger.h"
#include "stdbool.h"
#include "platform/portable_network.h"
#include "proto/punch.h"

const int SERVER_PORT = 6666;
struct sockaddr_in self_addr;
socklen_t self_addr_len;
struct sockaddr_in virtual_addr;
SOCKET clifd;

void handle_member_broadcast(struct sockaddr_in server_addr, struct sockaddr_in peer_addr, struct PUNCH_MESSAGE *msg);
void punch_round_1(struct PUNCH_MEMBER self_member, struct PUNCH_MEMBER peer_member);
void handle_p2p_packet(struct sockaddr_in peer_addr, struct PUNCH_MESSAGE *msg);

int main(int argc, char const *argv[])
{
  if (argc != 3)
  {
    ERROR_PRINT_THEN_EXIT("Usage: udp_client [server_ip] [virtual_ip]\n");
  }
  const char *server_ip = argv[1];
  const char *virtual_ip = argv[2];

  printf("Menber IP: %s\n", virtual_ip);

  memset(&virtual_addr, 0, sizeof(virtual_addr));
  virtual_addr.sin_family = AF_INET;
  if (inet_pton(AF_INET, virtual_ip, &virtual_addr.sin_addr) != 1)
  {
    ERROR_PRINT_THEN_EXIT("fail to inet_pton: %s",
                          portable_socket_api_strerror());
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = SERVER_PORT;
  if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) != 1)
  {
    ERROR_PRINT_THEN_EXIT("fail to inet_pton: %s",
                          portable_socket_api_strerror());
  }

  clifd = socket(AF_INET, SOCK_DGRAM, 0);
  if (!portable_is_socket_valid(clifd))
  {
    ERROR_PRINT_THEN_EXIT("fail to socket: %s\n",
                          portable_socket_api_strerror());
  }

  // if (bind(clifd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0)
  // {
  //   LOG_ERROR("bind: %s\n", portable_socket_api_strerror());
  //   exit(1);
  // }

  int payload_size = sizeof(virtual_addr);
  int msgsz = sizeof(struct PUNCH_MESSAGE) + sizeof(virtual_addr);
  char msg_buf[msgsz];
  memset(msg_buf, 0, msgsz);
  struct PUNCH_MESSAGE *msg = (struct PUNCH_MESSAGE *)msg_buf;
  msg->type = MSGTYPE_REGISTER_REQUEST;
  msg->size = payload_size;
  memcpy(msg->data, &virtual_addr, payload_size);

  int ret = sendto(clifd, msg, msgsz, 0, (struct sockaddr *)&server_addr,
                   sizeof(server_addr));
  if (ret < 0)
  {
    ERROR_PRINT_THEN_EXIT("sendto: %s\n", portable_socket_api_strerror());
  }

  self_addr_len = sizeof(self_addr);
  if (getsockname(clifd, (struct sockaddr *)&self_addr, &self_addr_len) != 0)
  {
    ERROR_PRINT_THEN_EXIT("getsockname: %s\n", portable_socket_api_strerror());
  }

  char *buf[BUFSIZ];
  struct sockaddr_in peer_addr;
  socklen_t peer_socklen = sizeof(peer_addr);

  while (true)
  {
    msgsz = recvfrom(clifd, buf, sizeof(buf), 0, (struct sockaddr *)&peer_addr,
                     &peer_socklen);
    if (msgsz < 0)
    {
      ERROR_PRINT_THEN_EXIT("fail to recvfrom: %s\n",
                            portable_socket_api_strerror());
    }

    struct PUNCH_MESSAGE *msg = (struct PUNCH_MESSAGE *)buf;

    switch (msg->type)
    {
    case MSGTYPE_MEMBER_BROADCAST:
      handle_member_broadcast(server_addr, peer_addr, msg);
      break;
    case MSGTYPE_P2P_PACKET:
      handle_p2p_packet(peer_addr, msg);
      break;
    default:
      ERROR_PRINT_THEN_EXIT("Unknown message type: 0x%x\n", msg->type);
      break;
    }
  }
  return 0;
}

void handle_member_broadcast(struct sockaddr_in server_addr, struct sockaddr_in peer_addr, struct PUNCH_MESSAGE *msg)
{
  printf("[handle_member_broadcast]\n");

  struct PUNCH_MEMBER *members = (struct PUNCH_MEMBER *)msg->data;
  int member_count = msg->size / sizeof(struct PUNCH_MEMBER);

  struct PUNCH_MEMBER self_member;
  bool self_found = false;
  char addr_str[128], vaddr_str[128];
  uint16_t member_port;

  for (int i = 0; i < member_count; i++)
  {
    bool is_self = memcmp(&members[i].vaddr, &virtual_addr, sizeof(virtual_addr)) == 0;

    if (is_self)
    {
      self_member = members[i];
      self_found = true;
    }
  }

  if (!self_found)
  {
    ERROR_PRINT_THEN_EXIT("fail to find self member\n");
  }

  for (int i = 0; i < member_count; i++)
  {
    if (inet_ntop(AF_INET, &members[i].addr.sin_addr, addr_str,
                  sizeof(addr_str)) == NULL)
    {
      ERROR_PRINT_THEN_EXIT("fail to inet_ntop: %s\n",
                            portable_socket_api_strerror());
    }
    member_port = ntohs(members[i].addr.sin_port);
    if (inet_ntop(AF_INET, &members[i].vaddr.sin_addr, vaddr_str,
                  sizeof(vaddr_str)) == NULL)
    {
      ERROR_PRINT_THEN_EXIT("fail to inet_ntop: %s\n",
                            portable_socket_api_strerror());
    }

    bool is_self = memcmp(&members[i].vaddr, &virtual_addr, sizeof(virtual_addr)) == 0;

    printf("\tmember info: addr=%s:%d, vaddr=%s, self=%s\n", addr_str, member_port, vaddr_str,
           is_self ? "YES" : "NO");

    if (!is_self)
    {
      punch_round_1(self_member, members[i]);
    }
  }
}

void punch_round_1(struct PUNCH_MEMBER self_member, struct PUNCH_MEMBER peer_member)
{
  struct PUNCH_MESSAGE msg;
  msg.type = MSGTYPE_P2P_PACKET;
  msg.size = 0;

  int ret = sendto(clifd, &msg, sizeof(msg), 0, (struct sockaddr *)&peer_member.addr,
                   sizeof(peer_member.addr));

  char addr_str[128], vaddr_str[128];
  uint16_t member_port;

  if (inet_ntop(AF_INET, &peer_member.addr.sin_addr, addr_str,
                sizeof(addr_str)) == NULL)
  {
    ERROR_PRINT_THEN_EXIT("fail to inet_ntop: %s\n",
                          portable_socket_api_strerror());
  }
  member_port = ntohs(peer_member.addr.sin_port);
  if (inet_ntop(AF_INET, &peer_member.vaddr.sin_addr, vaddr_str,
                sizeof(vaddr_str)) == NULL)
  {
    ERROR_PRINT_THEN_EXIT("fail to inet_ntop: %s\n",
                          portable_socket_api_strerror());
  }

  printf("[punch_round_1]\n");
  printf("\tsend to: addr=%s:%d, vaddr=%s\n", addr_str, member_port, vaddr_str);
}

void handle_p2p_packet(struct sockaddr_in peer_addr, struct PUNCH_MESSAGE *msg)
{
  printf("[handle_p2p_packet]\n");

  char addr_str[128], vaddr_str[128];
  uint16_t member_port;

  if (inet_ntop(AF_INET, &peer_addr.sin_addr, addr_str,
                sizeof(addr_str)) == NULL)
  {
    ERROR_PRINT_THEN_EXIT("fail to inet_ntop: %s\n",
                          portable_socket_api_strerror());
  }
  member_port = ntohs(peer_addr.sin_port);

  printf("\tmember info: addr=%s:%d\n", addr_str, member_port);
}