#pragma once

#include <optional>
#include <unordered_map>
#include <vector>

#include "generic.h"
#include "ring_buffer.h"

class Network {
 public:
  //
  // Ethernet
  //
  packed_struct EtherAddr {
    uint8_t mac[6];
    //
    bool IsEqualTo(EtherAddr to) const {
      return *reinterpret_cast<const uint32_t*>(mac) ==
                 *reinterpret_cast<const uint32_t*>(to.mac) &&
             *reinterpret_cast<const uint16_t*>(&mac[4]) ==
                 *reinterpret_cast<const uint16_t*>(&to.mac[4]);
    }
    bool operator==(const EtherAddr& rhs) const { return IsEqualTo(rhs); }
    void Print() const;
  };
  static constexpr EtherAddr kBroadcastEtherAddr = {0xFF, 0xFF, 0xFF,
                                                    0xFF, 0xFF, 0xFF};
  packed_struct EtherFrame {
    EtherAddr dst;
    EtherAddr src;
    uint8_t eth_type[2];
    //
    static constexpr uint8_t kTypeARP[2] = {0x08, 0x06};
    static constexpr uint8_t kTypeIPv4[2] = {0x08, 0x00};
    void SetEthType(const uint8_t(&etype)[2]) {
      eth_type[0] = etype[0];
      eth_type[1] = etype[1];
    }
    bool HasEthType(const uint8_t(&etype)[2]) {
      return eth_type[0] == etype[0] && eth_type[1] == etype[1];
    }
  };

  //
  // Checksum
  //
  packed_struct InternetChecksum {
    // https://tools.ietf.org/html/rfc1071
    uint8_t csum[2];

    void Clear() {
      csum[0] = 0;
      csum[1] = 0;
    }
    bool IsEqualTo(InternetChecksum to) const {
      return *reinterpret_cast<const uint16_t*>(csum) ==
             *reinterpret_cast<const uint16_t*>(to.csum);
    }
    static InternetChecksum Calc(void* buf, size_t start, size_t end) {
      // https://tools.ietf.org/html/rfc1071
      uint8_t* p = reinterpret_cast<uint8_t*>(buf);
      uint32_t sum = 0;
      for (size_t i = start; i < end; i += 2) {
        sum += (static_cast<uint16_t>(p[i + 0])) << 8 | p[i + 1];
      }
      while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
      }
      sum = ~sum;
      return {static_cast<uint8_t>((sum >> 8) & 0xFF),
              static_cast<uint8_t>(sum & 0xFF)};
    }
  };

  //
  // IPv4
  //
  packed_struct IPv4Addr {
    uint8_t addr[4];
    bool IsEqualTo(IPv4Addr to) const {
      return *reinterpret_cast<const uint32_t*>(addr) ==
             *reinterpret_cast<const uint32_t*>(to.addr);
    }
    bool operator==(const IPv4Addr& rhs) const { return IsEqualTo(rhs); }
    void Print() const;
    static std::optional<IPv4Addr> CreateFromString(const char* s) {
      IPv4Addr ip_addr;
      for (int i = 0;; i++) {
        ip_addr.addr[i] = StrToByte(s, &s);
        if (i == 3)
          break;
        if (*s != '.')
          return std::nullopt;
        s++;
      }
      if (*s != 0)
        return std::nullopt;
      return ip_addr;
    }
  };
  struct IPv4AddrHash {
    std::size_t operator()(const IPv4Addr& v) const {
      return std::hash<uint32_t>{}(*reinterpret_cast<const uint32_t*>(v.addr));
    }
  };
  static constexpr IPv4Addr kBroadcastIPv4Addr = {0xFF, 0xFF, 0xFF, 0xFF};
  static constexpr IPv4Addr kWildcardIPv4Addr = {0x00, 0x00, 0x00, 0x00};

  packed_struct IPv4Packet {
    enum class Protocol : uint8_t {
      kICMP = 1,
      kTCP = 6,
      kUDP = 17,
    };

    EtherFrame eth;
    uint8_t version_and_ihl;
    uint8_t dscp_and_ecn;
    uint8_t length[2];
    uint16_t ident;
    uint16_t flags;
    uint8_t ttl;
    Protocol protocol;
    InternetChecksum csum;  // for this header
    IPv4Addr src_ip;
    IPv4Addr dst_ip;

    void SetDataLength(uint16_t size) {
      size += sizeof(IPv4Packet) - sizeof(EtherFrame);  // IP header size
      size = (size + 1) & ~1;                           // make size odd
      length[0] = size >> 8;
      length[1] = size & 0xFF;
    }
    void CalcAndSetChecksum() {
      csum.Clear();
      csum = InternetChecksum::Calc(this, offsetof(IPv4Packet, version_and_ihl),
                                    sizeof(IPv4Packet));
    }
  };

  //
  // ICMP
  //
  packed_struct ICMPPacket {
    enum class Type : uint8_t {
      kEchoReply = 0,
      kEchoRequest = 8,
    };

    IPv4Packet ip;
    Type type;
    uint8_t code;
    InternetChecksum csum;
    uint16_t identifier;
    uint16_t sequence;
  };

  //
  // ARP
  //
  packed_struct ARPPacket {
    EtherFrame eth;
    uint8_t hw_type[2];
    uint8_t proto_type[2];
    uint8_t hw_addr_len;
    uint8_t proto_addr_len;
    uint8_t op[2];
    Network::EtherAddr sender_eth_addr;
    Network::IPv4Addr sender_proto_addr;
    Network::EtherAddr target_eth_addr;
    Network::IPv4Addr target_proto_addr;
    /*
      ARP example : who has 10.10.10.135? Tell 10.10.10.90
      ff ff ff ff ff ff  // dst
      f8 ff c2 01 df 39  // src
      08 06              // ether type (ARP)
      00 01              // hardware type (ethernet)
      08 00              // protocol type (ipv4)
      06                 // hw addr len (6)
      04                 // proto addr len (4)
      00 01              // operation (arp request)
      f8 ff c2 01 df 39  // sender eth addr
      0a 0a 0a 5a        // sender protocol addr
      00 00 00 00 00 00  // target eth addr (0 since its unknown)
      0a 0a 0a 87        // target protocol addr
     */
    enum class Operation {
      kUnknown,
      kRequest,
      kReply,
    };
    Operation GetOperation() {
      if (op[0] != 0)
        return Operation::kUnknown;
      if (op[1] == 0x01)
        return Operation::kRequest;
      if (op[1] == 0x02)
        return Operation::kReply;
      return Operation::kUnknown;
    }
    void SetupRequest(const Network::IPv4Addr target_ip,
                      const Network::IPv4Addr src_ip,
                      const Network::EtherAddr src_mac) {
      for (int i = 0; i < 6; i++) {
        eth.dst.mac[i] = 0xff;
        target_eth_addr.mac[i] = 0x00;
      }
      eth.src = src_mac;
      sender_eth_addr = src_mac;
      sender_proto_addr = src_ip;
      target_proto_addr = target_ip;
      eth.SetEthType(EtherFrame::kTypeARP);
      hw_type[0] = 0x00;
      hw_type[1] = 0x01;
      proto_type[0] = 0x08;
      proto_type[1] = 0x00;
      hw_addr_len = 6;
      proto_addr_len = 4;
      op[0] = 0x00;
      op[1] = 0x01;  // Request
    }
    void SetupReply(
        const Network::IPv4Addr target_ip, const Network::IPv4Addr src_ip,
        const Network::EtherAddr target_mac, const Network::EtherAddr src_mac) {
      eth.dst = target_mac;
      eth.src = src_mac;
      target_eth_addr = target_mac;
      sender_eth_addr = src_mac;
      sender_proto_addr = src_ip;
      target_proto_addr = target_ip;
      eth.SetEthType(EtherFrame::kTypeARP);
      hw_type[0] = 0x00;
      hw_type[1] = 0x01;
      proto_type[0] = 0x08;
      proto_type[1] = 0x00;
      hw_addr_len = 6;
      proto_addr_len = 4;
      op[0] = 0x00;
      op[1] = 0x02;  // Reply
    }
  };

  //
  // UDP
  //
  packed_struct IPv4UDPPacket {
    IPv4Packet ip;
    uint8_t src_port[2];  // optional
    uint8_t dst_port[2];
    uint8_t length[2];
    InternetChecksum csum;

    void SetSourcePort(uint16_t port) {
      src_port[0] = port >> 8;
      src_port[1] = port & 0xFF;
    }
    uint16_t GetSourcePort() {
      return static_cast<uint16_t>(src_port[0]) << 8 | src_port[1];
    }
    void SetDestinationPort(uint16_t port) {
      dst_port[0] = port >> 8;
      dst_port[1] = port & 0xFF;
    }
    uint16_t GetDestinationPort() {
      return static_cast<uint16_t>(dst_port[0]) << 8 | dst_port[1];
    }
    void SetDataSize(uint16_t size) {
      size += 8;               // UDP header size
      size = (size + 1) & ~1;  // make size odd
      length[0] = size >> 8;
      length[1] = size & 0xFF;
    }
  };
  static InternetChecksum CalcUDPChecksum(void* buf,
                                          size_t start,
                                          size_t end,
                                          Network::IPv4Addr src_addr,
                                          Network::IPv4Addr dst_addr,
                                          uint8_t (&udp_length)[2]) {
    // https://tools.ietf.org/html/rfc1071
    uint8_t* p = reinterpret_cast<uint8_t*>(buf);
    uint32_t sum = 0;
    // Pseudo-header
    sum += (static_cast<uint16_t>(src_addr.addr[0]) << 8) | src_addr.addr[1];
    sum += (static_cast<uint16_t>(src_addr.addr[2]) << 8) | src_addr.addr[3];
    sum += (static_cast<uint16_t>(dst_addr.addr[0]) << 8) | dst_addr.addr[1];
    sum += (static_cast<uint16_t>(dst_addr.addr[2]) << 8) | dst_addr.addr[3];
    sum += (static_cast<uint16_t>(udp_length[0]) << 8) | udp_length[1];
    sum += 17;  // Protocol: UDP
    for (size_t i = start; i < end; i += 2) {
      sum += (static_cast<uint16_t>(p[i + 0])) << 8 | p[i + 1];
    }
    while (sum >> 16) {
      sum = (sum & 0xffff) + (sum >> 16);
    }
    sum = ~sum;
    return {static_cast<uint8_t>((sum >> 8) & 0xFF),
            static_cast<uint8_t>(sum & 0xFF)};
  }

  //
  // DHCP
  //
  packed_struct DHCPPacket {
    IPv4UDPPacket udp;
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    Network::IPv4Addr ciaddr;
    Network::IPv4Addr yiaddr;
    Network::IPv4Addr siaddr;
    Network::IPv4Addr giaddr;
    Network::EtherAddr chaddr;
    uint8_t chaddr_padding[10];
    uint8_t sname[64];
    uint8_t file[128];
    void SetupRequest(const Network::EtherAddr& src_eth_addr) {
      // ip.eth
      udp.ip.eth.dst = Network::kBroadcastEtherAddr;
      udp.ip.eth.src = src_eth_addr;
      udp.ip.eth.SetEthType(EtherFrame::kTypeIPv4);
      // ip
      udp.ip.version_and_ihl =
          0x45;  // IPv4, header len = 5 * sizeof(uint32_t) = 20 bytes
      udp.ip.dscp_and_ecn = 0;
      udp.ip.SetDataLength(sizeof(DHCPPacket) - sizeof(IPv4Packet));
      udp.ip.ident = 0x426b;
      udp.ip.flags = 0;
      udp.ip.ttl = 0xFF;
      udp.ip.protocol = IPv4Packet::Protocol::kUDP;
      udp.ip.src_ip = Network::kWildcardIPv4Addr;
      udp.ip.dst_ip = Network::kBroadcastIPv4Addr;
      udp.ip.CalcAndSetChecksum();
      // udp
      udp.SetSourcePort(68);
      udp.SetDestinationPort(67);
      udp.SetDataSize(sizeof(DHCPPacket) - sizeof(IPv4UDPPacket));
      udp.csum.Clear();
      // dhcp
      op = 1;
      htype = 1;
      hlen = 6;
      hops = 0;
      xid = 0x1234;
      secs = 0;
      flags = 0;
      ciaddr = Network::kWildcardIPv4Addr;
      yiaddr = Network::kWildcardIPv4Addr;
      siaddr = Network::kWildcardIPv4Addr;
      giaddr = Network::kWildcardIPv4Addr;
      chaddr = src_eth_addr;
      for (int i = 0; i < 10; i++) {
        chaddr_padding[i] = 0;
      }
      for (int i = 0; i < 64; i++) {
        sname[i] = 0;
      }
      for (int i = 0; i < 128; i++) {
        file[i] = 0;
      }
      //
      udp.csum = CalcUDPChecksum(this, offsetof(DHCPPacket, udp.src_port),
                                 sizeof(DHCPPacket), Network::kWildcardIPv4Addr,
                                 Network::kBroadcastIPv4Addr, udp.length);
    }
  };

  //
  // ARP Table
  //
  using ARPTable = std::unordered_map<IPv4Addr, EtherAddr, IPv4AddrHash>;
  const ARPTable& GetARPTable() { return arp_table_; }
  void RegisterARPResolution(IPv4Addr ip_addr, EtherAddr eth_addr) {
    arp_table_[ip_addr] = eth_addr;
  }
  std::optional<EtherAddr> ResolveIPv4(IPv4Addr ip_addr) {
    if (arp_table_.find(ip_addr) == arp_table_.end())
      return std::nullopt;
    return arp_table_[ip_addr];
  }

  //
  // RX buffer
  //
  static constexpr int kPacketContainerSize = 2048;
  static constexpr int kRXBufferSize = 32;
  packed_struct PacketContainer {
    size_t size;
    uint8_t data[kPacketContainerSize];
  };
  void PushToRXBuffer(const void* data, size_t begin, size_t end) {
    assert(begin < end);
    PacketContainer buf;
    buf.size = end - begin;
    assert(buf.size <= kPacketContainerSize);
    memcpy(buf.data, reinterpret_cast<const uint8_t*>(data) + begin, buf.size);
    rx_buffer_.Push(buf);
  }
  PacketContainer PopFromRXBuffer() { return rx_buffer_.Pop(); }
  bool HasPacketInRXBuffer() { return !rx_buffer_.IsEmpty(); }

  static Network& GetInstance();

  //
  // sockets
  //
  struct Socket {
    uint64_t pid;
    int fd;
    uint16_t listen_port;
    enum class Type {
      kICMP,
      kUDP,
    } type;
  };

  bool RegisterSocket(uint64_t pid, int fd, Socket::Type type) {
    // returns true on failure
    if (FindSocket(pid, fd).has_value()) {
      return true;
    }
    sockets_.push_back({pid, fd, 0, type});
    return false;
  }
  bool BindToPort(uint64_t pid, int fd, uint16_t port) {
    // returns true on failure
    for (auto& it : sockets_) {
      if (it.pid == pid && it.fd == fd) {
        it.listen_port = port;
        return false;
      }
    }
    return true;
  }
  std::optional<Socket> FindSocket(uint64_t pid, int fd) {
    for (auto& it : sockets_) {
      if (it.pid == pid && it.fd == fd) {
        return it;
      }
    }
    return std::nullopt;
  }

 private:
  static Network* network_;

  ARPTable arp_table_;
  RingBuffer<PacketContainer, kRXBufferSize> rx_buffer_;
  std::vector<Socket> sockets_;

  Network(){};
};

void NetworkManager();
void SendARPRequest(Network::IPv4Addr);
void SendARPRequest(const char*);
void SendDHCPRequest();
