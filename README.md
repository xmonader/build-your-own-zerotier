## 项目介绍

实现一个类似于 Zerotier 的 L2 VPN，或者叫虚拟交换机（Virtual Switch）。

它的功能就是模拟一个物理交换机的行为，负责为连接到交换机各个端口的设备提供Ethernet帧交换服务。

不同之处在于，这个虚拟交换机的各个端口可以跨越互联网连接到世界各地的设备上，对于操作系统来说，仿佛它们处于同一个局域网。

## 项目架构
- 由一个服务端（VServer）和若干客户端（VClient/VPort）组成
- 服务端（VServer）的功能是模拟物理交换机的行为，为连接到交换机的各个客户端（VClient/VPort）提供Ethernet帧交换服务。
  - 维护ARP表
    |MAC|VPort/VClient|
    |--|--|
    11:11:11:11:11:11 | VClient-1
    aa:aa:aa:aa:aa:aa | VClient-a
  - 根据 ARP 表，实现 L2 数据包转发
- 客户端（VClient）的功能是模拟物理交换机端口（VPort），负责将交换机发往端口的数据送往计算机，以及将计算机发往端口的数据发给交换机
  - 一端连接 TAP 设备
  - 一端连接通过网络 VServer
  - 负责 TAP 设备和 VServer 之间的数据包转发
    ```
    Linux Kernel <==[TAP]==> VClient <==[UDP]==> VServer
    ```
