#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket

vserver_addr = ("0.0.0.0", 6666)
vserver_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
vserver_sock.bind(vserver_addr)

arp_cache = {}

while True:
  data, vclient_addr = vserver_sock.recvfrom(1518)
  eth_header = data[:14]
  eth_dst = ":".join("{:02x}".format(x) for x in eth_header[0:6])
  eth_src = ":".join("{:02x}".format(x) for x in eth_header[6:12])
  eth_type = eth_header[12:14]

  print(f"[VServer] vclient_addr<{vclient_addr}> src<{eth_src}> src<{eth_dst}> datasz<{len(data)}>")
  
  if (eth_src not in arp_cache):
    arp_cache[eth_src] = vclient_addr
    print(f"    ARP Cache: {arp_cache}")

  if eth_dst in arp_cache:
    vserver_sock.sendto(data, arp_cache[eth_dst])
    print(f"    To: {eth_dst}")
  elif eth_dst == "ff:ff:ff:ff:ff:ff":
    brd_dst_macs = list(arp_cache.keys())
    brd_dst_macs.remove(eth_src)
    brd_dst_vports = {arp_cache[mac] for mac in brd_dst_macs}
    print(f"    Broadcast: {brd_dst_vports}")
    for brd_dst in brd_dst_vports:
      vserver_sock.sendto(data, brd_dst)
  else:
    print(f"    Missed")