## 预期目标

实现一个类似于 Zerotier 的 L2 VPN。

程序架构 v1：
- 服务端：负责设备管理、数据包中继
- 客户端：模拟 L2 设备，借助服务端实现数据包在虚拟局域网内的传输

程序架构 v2：
- 服务端（VServer）：模拟交换机
  - 维护ARP表
    |MAC|VPort/VClient|
    |--|--|
    11:11:11:11:11:11 | VClient-1
    aa:aa:aa:aa:aa:aa | VClient-a
  - 根据 ARP 表，实现 L2 数据包转发
- 客户端（VClient）：模拟交换机端口（VPort）
  - 一端连接 TAP 设备
  - 一端连接通过网络 VServer
  - 负责 TAP 设备和 VServer 之间的数据包转发
    ```
    Linux Kernel <==[TAP]==> VClient <==[TCP/UDP]==> VServer
    ```
