#ifndef __PROTO_DUMP_H
#define __PROTO_DUMP_H

#include <net/ethernet.h>
#include <arpa/inet.h>
#include "arp.h"

void print_ether_frame(const char *ether_data, int ether_datasz);
void print_arp_packet(const char *ether_data, int ether_datasz);

#endif