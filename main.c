#include "tuntap.h"
#include "misc/utils.h"
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <net/ethernet.h>

void print_pdu(const char *data, int datasz);

int main(int argc, char const *argv[])
{
  char ifname[IFNAMSIZ] = "tapyuan";
  int tapfd = tap_alloc(ifname);
  if (tapfd < 0)
  {
    ERROR_PRINT_THEN_EXIT("fail to tap_alloc: %s\n", strerror(errno));
  }

  printf("tap fd is : %d\n", tapfd);

  char buf[BUFSIZ];
  while (true)
  {
    int readsz = read(tapfd, buf, BUFSIZ);
    if (readsz > 0)
    {
      print_pdu(buf, readsz);
    }
  }

  return 0;
}

void print_pdu(const char *data, int datasz)
{
  assert(datasz >= 14);

  const struct ether_header *hdr = (const struct ether_header *)data;

  printf("[PDU] \n"
         "  dest_mac: %02x:%02x:%02x:%02x:%02x:%02x\n"
         "  hdr->ether_shost: %02x:%02x:%02x:%02x:%02x:%02x\n"
         "  type: %04x\n"
         "  datasz=%d\n",
         hdr->ether_dhost[0], hdr->ether_dhost[1], hdr->ether_dhost[2], hdr->ether_dhost[3], hdr->ether_dhost[4], hdr->ether_dhost[5],
         hdr->ether_shost[0], hdr->ether_shost[1], hdr->ether_shost[2], hdr->ether_shost[3], hdr->ether_shost[4], hdr->ether_shost[5],
         ntohs(hdr->ether_type),
         datasz);
}
