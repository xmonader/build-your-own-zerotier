#include "dump.h"

void print_ether_frame(const char *ether_data, int ether_datasz)
{
  assert(ether_datasz >= 14);

  const struct ether_header *hdr = (const struct ether_header *)ether_data;

  printf("[Ethernet Frame] \n"
         "  dhost: %02x:%02x:%02x:%02x:%02x:%02x\n"
         "  shost: %02x:%02x:%02x:%02x:%02x:%02x\n"
         "  type: %04x\n"
         "  datasz=%d\n",
         hdr->ether_dhost[0], hdr->ether_dhost[1], hdr->ether_dhost[2], hdr->ether_dhost[3], hdr->ether_dhost[4], hdr->ether_dhost[5],
         hdr->ether_shost[0], hdr->ether_shost[1], hdr->ether_shost[2], hdr->ether_shost[3], hdr->ether_shost[4], hdr->ether_shost[5],
         ntohs(hdr->ether_type),
         ether_datasz);
}

void print_arp_packet(const char *ether_data, int ether_datasz)
{
  assert(ether_datasz >= 14);

  const struct ether_header *hdr = (const struct ether_header *)ether_data;
  const struct arp_frame *arp_frame = (const struct arp_frame *)(ether_data + sizeof(struct ether_header));

  print_ether_frame(ether_data, ether_datasz);

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
