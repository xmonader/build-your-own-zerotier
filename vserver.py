#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket

# 0. create UDP socket, bind to service port
vserver_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
vserver_sock.bind(("0.0.0.0", 6666))

arp_cache = {}

while True:
  # 1. read ethernet frame from VClient/VPort
  data, vclient_addr = vserver_sock.recvfrom(1518)

  # 2. parse ethernet frame
  eth_header = data[:14]
  #    ethernet destination hardware address (MAC)
  eth_dst = ":".join("{:02x}".format(x) for x in eth_header[0:6])
  #    ethernet source hardware address (MAC)
  eth_src = ":".join("{:02x}".format(x) for x in eth_header[6:12])

  print(f"[VServer] vclient_addr<{vclient_addr}> "
        "src<{eth_src}> dst<{eth_dst}> datasz<{len(data)}>")
  
  # 3. insert/update arp cache
  if (eth_src not in arp_cache or arp_cache[eth_src] != vclient_addr):
    arp_cache[eth_src] = vclient_addr
    print(f"    ARP Cache: {arp_cache}")

  # 4. forward ethernet frame
  #    if dest in arp cache, forward ethernet frame to it
  if eth_dst in arp_cache:
    vserver_sock.sendto(data, arp_cache[eth_dst])
    print(f"    Forwarded to: {eth_dst}")
  #    if dest is broadcast address, 
  #    broadcast ethernet frame to every known VPort except source VPort
  elif eth_dst == "ff:ff:ff:ff:ff:ff":
    brd_dst_macs = list(arp_cache.keys())
    brd_dst_macs.remove(eth_src)
    brd_dst_vports = {arp_cache[mac] for mac in brd_dst_macs}
    print(f"    Broadcasted to: {brd_dst_vports}")
    for brd_dst in brd_dst_vports:
      vserver_sock.sendto(data, brd_dst)
  #    otherwise, for simplicity, discard the ethernet frame
  else:
    print(f"    Discarded")