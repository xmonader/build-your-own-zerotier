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
#include <glib.h>

const int SERVER_PORT = 6666;

GList *member_list = NULL;

void handle_register_request(struct sockaddr_in client_addr, const struct PUNCH_MESSAGE *msg);
void print_member_list();
void broadcast_member_list(int srvfd);

int main(int argc, char const *argv[])
{
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = SERVER_PORT;
  server_addr.sin_addr.s_addr = INADDR_ANY;

  SOCKET srvfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (!portable_is_socket_valid(srvfd))
  {
    ERROR_PRINT_THEN_EXIT("fail to socket: %s\n",
                          portable_socket_api_strerror());
  }

  if (bind(srvfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
  {
    ERROR_PRINT_THEN_EXIT("fail to bind: %s\n", portable_socket_api_strerror());
  }

  char *buf[BUFSIZ];
  int msgsz;
  struct sockaddr_in client_addr;
  socklen_t client_socklen = sizeof(client_addr);

  while (true)
  {
    msgsz = recvfrom(srvfd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr,
                     &client_socklen);
    if (msgsz < 0)
    {
      ERROR_PRINT_THEN_EXIT("fail to recvfrom: %s\n",
                            portable_socket_api_strerror());
    }

    struct PUNCH_MESSAGE *msg = (struct PUNCH_MESSAGE *)buf;

    switch (msg->type)
    {
    case MSGTYPE_REGISTER_REQUEST:
      handle_register_request(client_addr, msg);
      print_member_list();
      broadcast_member_list(srvfd);
      break;

    default:
      ERROR_PRINT_THEN_EXIT("Unknown message type: 0x%x\n", msg->type);
      break;
    }
  }

  g_list_free(member_list);
  return 0;
}

void handle_register_request(struct sockaddr_in client_addr, const struct PUNCH_MESSAGE *msg)
{
  char addr_str[128], vaddr_str[128];
  uint16_t port;
  if (inet_ntop(AF_INET, &client_addr.sin_addr, addr_str,
                sizeof(addr_str)) == NULL)
  {
    ERROR_PRINT_THEN_EXIT("fail to inet_ntop: %s\n",
                          portable_socket_api_strerror());
  }
  port = ntohs(client_addr.sin_port);

  struct sockaddr_in *virtual_addr = (struct sockaddr_in *)msg->data;
  if (inet_ntop(AF_INET, &virtual_addr->sin_addr, vaddr_str,
                sizeof(vaddr_str)) == NULL)
  {
    ERROR_PRINT_THEN_EXIT("fail to inet_ntop: %s\n",
                          portable_socket_api_strerror());
  }

  printf("[handle_register_request]\n");
  printf("\tmessage type: 0x%x\n", msg->type);
  printf("\tmessage size: %d\n", msg->size);
  printf("\tmessage addr: %s:%d\n", addr_str, port);
  printf("\tmessage vaddr: %s\n", vaddr_str);

  struct PUNCH_MEMBER *member = malloc(sizeof(struct PUNCH_MEMBER));
  member->addr = client_addr;
  member->vaddr = *virtual_addr;
  member_list = g_list_append(member_list, member);
}

void print_member_list()
{
  GList *iter;
  struct PUNCH_MEMBER *member;
  char addr_str[128], vaddr_str[128];
  uint16_t member_port;

  printf("[print_member_list]\n");

  for (iter = member_list; iter != NULL; iter = iter->next)
  {
    member = iter->data;

    if (inet_ntop(AF_INET, &member->addr.sin_addr, addr_str,
                  sizeof(addr_str)) == NULL)
    {
      ERROR_PRINT_THEN_EXIT("fail to inet_ntop: %s\n",
                            portable_socket_api_strerror());
    }
    member_port = ntohs(member->addr.sin_port);
    if (inet_ntop(AF_INET, &member->vaddr.sin_addr, vaddr_str,
                  sizeof(vaddr_str)) == NULL)
    {
      ERROR_PRINT_THEN_EXIT("fail to inet_ntop: %s\n",
                            portable_socket_api_strerror());
    }

    printf("\tmember info: addr=%s:%d, vaddr=%s\n", addr_str, member_port, vaddr_str);
  }
}

void broadcast_member_list(int srvfd)
{
  printf("[broadcast_member_list]\n");

  GList *iter;
  int member_count = g_list_length(member_list);
  int payload_size = member_count * sizeof(struct PUNCH_MEMBER);
  int msgsz = sizeof(struct PUNCH_MESSAGE) + payload_size;
  char msg_buf[msgsz];
  struct PUNCH_MESSAGE *msg = (struct PUNCH_MESSAGE *)msg_buf;
  msg->type = MSGTYPE_MEMBER_BROADCAST;
  msg->size = payload_size;
  struct PUNCH_MEMBER *members = (struct PUNCH_MEMBER *)msg->data;

  int member_index = 0;
  for (iter = member_list; iter != NULL; iter = iter->next, member_index++)
  {
    members[member_index] = *(struct PUNCH_MEMBER *)iter->data;
  }

  struct PUNCH_MEMBER *member;
  char addr_str[128], vaddr_str[128];
  uint16_t member_port;

  for (iter = member_list; iter != NULL; iter = iter->next)
  {
    member = iter->data;

    if (inet_ntop(AF_INET, &member->addr.sin_addr, addr_str,
                  sizeof(addr_str)) == NULL)
    {
      ERROR_PRINT_THEN_EXIT("fail to inet_ntop: %s\n",
                            portable_socket_api_strerror());
    }
    member_port = ntohs(member->addr.sin_port);
    if (inet_ntop(AF_INET, &member->vaddr.sin_addr, vaddr_str,
                  sizeof(vaddr_str)) == NULL)
    {
      ERROR_PRINT_THEN_EXIT("fail to inet_ntop: %s\n",
                            portable_socket_api_strerror());
    }

    int ret = sendto(srvfd, msg, msgsz, 0, (struct sockaddr *)&member->addr,
                     sizeof(member->addr));
    if (ret < 0)
    {
      ERROR_PRINT_THEN_EXIT("sendto: %s\n", portable_socket_api_strerror());
    }

    printf("\tbroadcast to: addr=%s:%d, vaddr=%s\n", addr_str, member_port, vaddr_str);
  }
}