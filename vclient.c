#include "tap_utils.h"
#include "sys_utils.h"
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <pthread.h>

/**
 * VClient Instance
 */
struct vclient_t
{
  int tapfd;                       // TAP device file descriptor, connected to linux kernel network stack
  int vclient_sockfd;              // client socket, for communicating with VServer/VSwitch
  struct sockaddr_in vserver_addr; // VServer address
};

void vclient_init(struct vclient_t *vclient, const char *vserver_ip_str, int vserver_port);
void *forward_ether_data_to_vserver(void *raw_vclient);
void *forward_ether_data_to_tap(void *raw_vclient);

int main(int argc, char const *argv[])
{
  // parse arguments
  if (argc != 3)
  {
    ERROR_PRINT_THEN_EXIT("Usage: vclient {vserver_ip} {vserver_port}\n");
  }
  const char *vserver_ip_str = argv[1];
  int vserver_port = atoi(argv[2]);

  // vclient init
  struct vclient_t vclient;
  vclient_init(&vclient, vserver_ip_str, vserver_port);

  // up forwarder
  pthread_t up_forwarder;
  if (pthread_create(&up_forwarder, NULL, forward_ether_data_to_vserver, &vclient) != 0)
  {
    ERROR_PRINT_THEN_EXIT("fail to pthread_create: %s\n", strerror(errno));
  }

  // down forwarder
  pthread_t down_forwarder;
  if (pthread_create(&down_forwarder, NULL, forward_ether_data_to_tap, &vclient) != 0)
  {
    ERROR_PRINT_THEN_EXIT("fail to pthread_create: %s\n", strerror(errno));
  }

  // wait for up forwarder & down forwarder
  if (pthread_join(up_forwarder, NULL) != 0 || pthread_join(down_forwarder, NULL) != 0)
  {
    ERROR_PRINT_THEN_EXIT("fail to pthread_join: %s\n", strerror(errno));
  }

  return 0;
}

/**
 * init VClient instance: create TAP device and client socket
 */
void vclient_init(struct vclient_t *vclient, const char *vserver_ip_str, int vserver_port)
{
  // alloc tap device
  char ifname[IFNAMSIZ] = "tapyuan";
  int tapfd = tap_alloc(ifname);
  if (tapfd < 0)
  {
    ERROR_PRINT_THEN_EXIT("fail to tap_alloc: %s\n", strerror(errno));
  }

  // create socket
  int vclient_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (vclient_sockfd < 0)
  {
    ERROR_PRINT_THEN_EXIT("fail to socket: %s\n", strerror(errno));
  }

  // setup vserver info
  struct sockaddr_in vserver_addr;
  memset(&vserver_addr, 0, sizeof(vserver_addr));
  vserver_addr.sin_family = AF_INET;
  vserver_addr.sin_port = htons(vserver_port);
  if (inet_pton(AF_INET, vserver_ip_str, &vserver_addr.sin_addr) != 1)
  {
    ERROR_PRINT_THEN_EXIT("fail to inet_pton: %s\n", strerror(errno));
  }

  vclient->tapfd = tapfd;
  vclient->vclient_sockfd = vclient_sockfd;
  vclient->vserver_addr = vserver_addr;

  printf("[VClient] TAP device name: %s, VServer: %s:%d\n", ifname, vserver_ip_str, vserver_port);
}

/**
 * Forward ethernet frame from TAP device to VServer
 */
void *forward_ether_data_to_vserver(void *raw_vclient)
{
  struct vclient_t *vclient = (struct vclient_t *)raw_vclient;
  char ether_data[ETHER_MAX_LEN];
  while (true)
  {
    // read ethernet from tap device
    int ether_datasz = read(vclient->tapfd, ether_data, sizeof(ether_data));
    if (ether_datasz > 0)
    {
      assert(ether_datasz >= 14);
      const struct ether_header *hdr = (const struct ether_header *)ether_data;

      // forward ethernet frame to VServer/VSwitch
      ssize_t sendsz = sendto(vclient->vclient_sockfd, ether_data, ether_datasz, 0, (struct sockaddr *)&vclient->vserver_addr, sizeof(vclient->vserver_addr));
      if (sendsz != ether_datasz)
      {
        fprintf(stderr, "sendto size mismatch: ether_datasz=%d, sendsz=%d\n", ether_datasz, sendsz);
      }

      printf("[VClient] Sent to VServer:"
             " dhost<%02x:%02x:%02x:%02x:%02x:%02x>"
             " shost<%02x:%02x:%02x:%02x:%02x:%02x>"
             " type<%04x>"
             " datasz=<%d>\n",
             hdr->ether_dhost[0], hdr->ether_dhost[1], hdr->ether_dhost[2], hdr->ether_dhost[3], hdr->ether_dhost[4], hdr->ether_dhost[5],
             hdr->ether_shost[0], hdr->ether_shost[1], hdr->ether_shost[2], hdr->ether_shost[3], hdr->ether_shost[4], hdr->ether_shost[5],
             ntohs(hdr->ether_type),
             ether_datasz);
    }
  }
}

/**
 * Forward ethernet frame from VServer to TAP device
 */
void *forward_ether_data_to_tap(void *raw_vclient)
{
  struct vclient_t *vclient = (struct vclient_t *)raw_vclient;
  char ether_data[ETHER_MAX_LEN];
  while (true)
  {
    // read ethernet frame from VServer/VSwitch
    socklen_t vserver_addr = sizeof(vclient->vserver_addr);
    int ether_datasz = recvfrom(vclient->vclient_sockfd, ether_data, sizeof(ether_data), 0,
                                (struct sockaddr *)&vclient->vserver_addr, &vserver_addr);
    if (ether_datasz > 0)
    {
      assert(ether_datasz >= 14);
      const struct ether_header *hdr = (const struct ether_header *)ether_data;

      // forward ethernet frame to TAP device (Linux network stack)
      ssize_t sendsz = write(vclient->tapfd, ether_data, ether_datasz);
      if (sendsz != ether_datasz)
      {
        fprintf(stderr, "sendto size mismatch: ether_datasz=%d, sendsz=%d\n", ether_datasz, sendsz);
      }

      printf("[VClient] Forward to TAP device:"
             " dhost<%02x:%02x:%02x:%02x:%02x:%02x>"
             " shost<%02x:%02x:%02x:%02x:%02x:%02x>"
             " type<%04x>"
             " datasz=<%d>\n",
             hdr->ether_dhost[0], hdr->ether_dhost[1], hdr->ether_dhost[2], hdr->ether_dhost[3], hdr->ether_dhost[4], hdr->ether_dhost[5],
             hdr->ether_shost[0], hdr->ether_shost[1], hdr->ether_shost[2], hdr->ether_shost[3], hdr->ether_shost[4], hdr->ether_shost[5],
             ntohs(hdr->ether_type),
             ether_datasz);
    }
  }
}