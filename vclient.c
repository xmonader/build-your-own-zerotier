#include "tuntap.h"
#include "misc/utils.h"
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_ether.h>
#include <net/if_arp.h>
#include "proto/arp.h"

void handle_arp_packet(const char *data, int datasz);
void print_ether_frame(const char *data, int datasz);

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
      const struct tun_pi *proto_info = (const struct tun_pi *)buf;
      const char *ether_data = buf + sizeof(struct tun_pi);
      int ether_datasz = readsz - sizeof(struct tun_pi);

      printf("<TYPE=%04x>\n", ntohs(proto_info->proto));

      switch (ntohs(proto_info->proto))
      {
      case ETHERTYPE_ARP:
        handle_arp_packet(ether_data, ether_datasz);
        break;

      default:
        print_ether_frame(ether_data, ether_datasz);
        break;
      }
    }
  }

  return 0;
}

void print_ether_frame(const char *data, int datasz)
{
  assert(datasz >= 14);

  const struct ether_header *hdr = (const struct ether_header *)data;

  printf("[Ethernet Frame] \n"
         "  dhost: %02x:%02x:%02x:%02x:%02x:%02x\n"
         "  shost: %02x:%02x:%02x:%02x:%02x:%02x\n"
         "  type: %04x\n"
         "  datasz=%d\n",
         hdr->ether_dhost[0], hdr->ether_dhost[1], hdr->ether_dhost[2], hdr->ether_dhost[3], hdr->ether_dhost[4], hdr->ether_dhost[5],
         hdr->ether_shost[0], hdr->ether_shost[1], hdr->ether_shost[2], hdr->ether_shost[3], hdr->ether_shost[4], hdr->ether_shost[5],
         ntohs(hdr->ether_type),
         datasz);
}

void handle_arp_packet(const char *data, int datasz)
{
  assert(datasz >= 14);

  const struct ether_header *hdr = (const struct ether_header *)data;
  const struct arp_frame *arp_frame = (const struct arp_frame *)(data + sizeof(struct ether_header));

  print_ether_frame(data, datasz);

  char sender_addr_str[INET_ADDRSTRLEN], target_addr_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, (struct in_addr *)&arp_frame->ar_sip, sender_addr_str, sizeof(sender_addr_str));
  inet_ntop(AF_INET, (struct in_addr *)&arp_frame->ar_tip, target_addr_str, sizeof(target_addr_str));

  printf("  [ARP Frame] \n"
         "    hardware type   : 0x%04x\n"
         "    protocol type   : 0x%04x\n"
         "    hardware length : 0x%02x\n"
         "    protocol length : 0x%02x\n"
         "    operation       : 0x%04x\n"
         "    sender hardware : %02x:%02x:%02x:%02x:%02x:%02x\n"
         "    sender ip       : %s\n"
         "    target hardware : %02x:%02x:%02x:%02x:%02x:%02x\n"
         "    target ip       : %s\n",
         ntohs(arp_frame->ar_hrd),
         ntohs(arp_frame->ar_pro),
         arp_frame->ar_hln,
         arp_frame->ar_pln,
         ntohs(arp_frame->ar_op),
         arp_frame->ar_sha[0], arp_frame->ar_sha[1], arp_frame->ar_sha[2], arp_frame->ar_sha[3], arp_frame->ar_sha[4], arp_frame->ar_sha[5],
         sender_addr_str,
         arp_frame->ar_tha[0], arp_frame->ar_tha[1], arp_frame->ar_tha[2], arp_frame->ar_tha[3], arp_frame->ar_tha[4], arp_frame->ar_tha[5],
         target_addr_str);
}
