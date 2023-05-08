#ifndef __PROTO_ARP_H
#define __PROTO_ARP_H

#include <linux/if_ether.h>

struct arp_frame
{
  unsigned short int ar_hrd; /* Format of hardware address.  */
  unsigned short int ar_pro; /* Format of protocol address.  */
  unsigned char ar_hln;      /* Length of hardware address.  */
  unsigned char ar_pln;      /* Length of protocol address.  */
  unsigned short int ar_op;  /* ARP opcode (command).  */
  /* Ethernet looks like this : This bit is variable sized
     however...  */
  unsigned char ar_sha[ETH_ALEN]; /* Sender hardware address.  */
  unsigned char ar_sip[4];        /* Sender IP address.  */
  unsigned char ar_tha[ETH_ALEN]; /* Target hardware address.  */
  unsigned char ar_tip[4];        /* Target IP address.  */
} __attribute__((packed));

#endif