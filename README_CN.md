[English](README.md) | [中文](README_CN.md)

# Build your own Zerotier

## 项目介绍

实现一个类似于 Zerotier 的 L2 VPN，或者叫虚拟交换机（Virtual Switch）。

它的功能就是模拟一个物理交换机的行为，负责为连接到交换机各个端口的设备提供 Ethernet 帧交换服务。

不同之处在于，这个虚拟交换机的各个端口可以跨越互联网连接到世界各地的设备上，对于操作系统来说，仿佛它们处于同一个局域网。


## 背景知识

### 交换机

交换机在 OSI 模型中工作在数据链路层，能够识别和转发以太网帧。在转发数据包时，交换机使用转发表来查找目标地址所对应的端口。

在交换机中，转发表一般称为 MAC 地址表。这个表维护着本地网络中已知的 MAC 地址及其对应的端口号信息。当交换机收到一个数据帧时，它会查找目标 MAC 地址在转发表中对应的端口，并只向该端口转发该数据帧，从而实现局域网内的数据转发。如果目标 MAC 地址不在表中，则交换机会向除接收端口外的所有端口转发该数据帧，以便让目标设备响应并更新转发表。

在本项目中，将编写一个软件作为虚拟交换机，实现以太网帧交换功能。

### TAP设备

TAP 是一种虚拟网络设备，它可以模拟一个物理网络接口，使得操作系统和应用程序能够像使用物理接口一样使用它。TAP 设备通常用于在不同计算机之间创建虚拟私有网络（VPN）连接，以便在公共网络上安全地传输数据。

TAP 设备在操作系统内核中实现，它看起来像一个普通的网络接口，可以在应用程序中像普通的物理网卡一样使用。当数据包通过 TAP 设备发送时，它们会被传递到内核中的 TUN/TAP 驱动程序，该驱动程序会将数据包传递到应用程序，应用程序可以对数据包进行处理并将它们传递到其他设备或网络中。同样，当应用程序要发送数据包时，它们会被传递到 TUN/TAP 驱动程序，驱动程序会将它们转发到指定的目标设备或网络中。

在本项目中，TAP 设备用于连接客户端计算机和虚拟交换机，以便实现客户端计算机和虚拟交换机之间的数据包转发。

## 项目架构
- 由一个服务端（VServer/VSwitch）和若干客户端（VClient/VPort）组成
- 服务端（VServer）的功能是模拟物理交换机的行为，为连接到交换机的各个客户端（VClient/VPort）提供Ethernet帧交换服务。
  - 维护 MAC 表
    |MAC|VPort/VClient|
    |--|--|
    11:11:11:11:11:11 | VClient-1
    aa:aa:aa:aa:aa:aa | VClient-a
  - 根据 MAC 表，实现 Ethernet 帧转发
- 客户端（VClient）的功能是模拟物理交换机端口（VPort），负责将交换机发往端口的数据送往计算机，以及将计算机发往端口的数据发给交换机
  - 一端连接 TAP 设备
  - 一端连接通过网络 VServer
  - 负责 TAP 设备和 VServer 之间的数据包转发
    ```
    Linux Kernel <==[TAP]==> VClient <==[UDP]==> VServer
    ```

    ```
        +----------------------------------------------+
        |               VServer/VSwitch                |
        |                                              |
        |     +----------------+---------------+       |
        |     |            MAC Table           |       |
        |     |------------------------0-------+       |
        |     |      MAC       |      VPort    |       |
        |     |--------------------------------+       |
        |     | 11:11:11:11:11 |   VClient-1   |       |
        |     |--------------------------------+       |
        |     | aa:aa:aa:aa:aa |   VClient-a   |       |
        |     +----------------+---------------+       |
        |                                              |
        |           ^                       ^          |
        +-----------|-----------------------|----------+
            +-------|--------+     +--------|-------+
            |       v        |     |        v       |
            | +------------+ |     | +------------+ |
            | | UDP Socket | |     | | UDP Socket | |
            | +------------+ |     | +------------+ |
            |       ^        |     |        ^       |
            |       |        |     |        |       |
            | Ethernet Frame |     | Ethernet Frame |
            |       |        |     |        |       |
      VPort |       v        |     |        v       | VPort
            | +------------+ |     | +------------+ |
            | | TAP Device | |     | | TAP Device | |
            | +------------+ |     | +------------+ |
            |       ^        |     |        ^       |
            +-------|--------+     +--------|-------+
                    v                       v
        ------------------------------------------------
                        Linux Kernel                   

    ```

## 代码说明

1. `vserver.py`: code fro VServer
2. `vclient.c`: code fro VClient

## 编译
```
make
```

## 运行

### 环境准备

- 一个具备公网 IP 的服务器，用于运行 VServer
- 至少两台客户端，用于运行 VClient，接入 VServer 构建 Virtual Private Network
- 假设公网 IP 为 `VSERVER_IP`，服务器端口为 `VSERVER_PORT`

### Step 1. 运行 VServer
在具有公网IP的服务器上
```
python3 vserver.py
```

### Step 2. 运行并配置 VClient-1

- 运行 VClient
    ```
    sudo ./vclient ${VSERVER_IP} ${VSERVER_PORT}
    ```
- 配置 TAP 设备
    ```
    sudo ip addr add 10.1.1.101/24 dev tapyuan
    sudo ip link set tapyuan up
    ```

### Step 3. 运行并配置 VClient-2

- 运行 VClient
    ```
    sudo ./vclient ${VSERVER_IP} ${VSERVER_PORT}
    ```
- 配置 TAP 设备
    ```
    sudo ip addr add 10.1.1.102/24 dev tapyuan
    sudo ip link set tapyuan up
    ```

### Step 4. ping 连通性测试

- 在 VClient-1 上 ping VClient-2
    ```
    ping 10.1.1.102
    ```
- 在 VClient-2 上 ping VClient-1
    ```
    ping 10.1.1.101
    ```
### 效果图
![](https://cdn.jsdelivr.net/gh/peiyuanix/picgo-repo/data/QQ图片20230509034140.png)