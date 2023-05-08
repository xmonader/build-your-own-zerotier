## Project Introduction

Implement a L2 VPN similar to Zerotier or a Virtual Switch.

Its function is to simulate the behavior of a physical switch, providing Ethernet frame exchange services for devices connected to the switch's ports.

The difference is that the ports of this virtual switch can be connected to devices all over the world through the Internet, making them appear to be in the same local area network for the operating system.

## Background Knowledge

### Switch

Switches work at the L2 (Data Link Layer) of the OSI Model and can recognize and forward Ethernet frames. When forwarding packets, switches use a forwarding table to look up the port corresponding to the destination address.

In a switch, the forwarding table is generally called the MAC address table. This table maintains the MAC addresses known in the local network and their corresponding port numbers. When a switch receives a data frame, it looks up the port corresponding to the destination MAC address in the forwarding table, and forwards the data frame only to that port, thereby achieving data forwarding within the local area network. If the destination MAC address is not in the table, the switch will forward the data frame to all ports except the receiving port so that the target device can respond and update the forwarding table.

In this project, we will write software as a virtual switch to achieve Ethernet frame exchange function.

### TAP Device

TAP is a type of virtual network device that can simulate a physical network interface, allowing operating systems and applications to use it like a physical interface. TAP devices are commonly used to create virtual private network (VPN) connections between different computers for secure data transmission over public networks.

The TAP device is implemented in the operating system kernel. It looks like a regular network interface and can be used in applications like a regular physical network card. When packets are sent through the TAP device, they are passed to the TUN/TAP driver in the kernel, which passes the packets to the application. The application can process the packets and pass them to other devices or networks. Similarly, when applications send packets, they are passed to the TUN/TAP driver, which forwards them to the specified target device or network.

In this project, the TAP device is used to connect client computers and the virtual switch, enabling packet forwarding between client computers and the virtual switch.

## Project Architecture
- Composed of one server (VServer/VSwitch) and several clients (VClient/VPort)
- The server (VServer/VSwitch) simulates the behavior of a physical switch, providing Ethernet frame exchange services for each client (VClient/VPort) connected to the switch.
  - Maintains a MAC table
    |MAC|VPort/VClient|
    |--|--|
    11:11:11:11:11:11 | VClient-1
    aa:aa:aa:aa:aa:aa | VClient-a
  - Implement Ethernet frame forwarding based on the MAC table
- The client (VClient) simulates the behavior of a physical switch port (VPort), responsible for forwarding data sent to the switch to the computer, and forwarding data sent from the computer to the switch.
  - One end is connected to the TAP device.
  - One end is connected to the VServer through the network.
  - Responsible for packet forwarding between the TAP device and VServer.
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