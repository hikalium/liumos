#pragma once

#include <optional>
#include <unordered_map>

#include "generic.h"
#include "ring_buffer.h"

class Network {
 public:
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
  packed_struct EtherAddr {
    uint8_t mac[6];
    bool IsEqualTo(EtherAddr to) const {
      return *reinterpret_cast<const uint32_t*>(mac) ==
                 *reinterpret_cast<const uint32_t*>(to.mac) &&
             *reinterpret_cast<const uint16_t*>(&mac[4]) ==
                 *reinterpret_cast<const uint16_t*>(&to.mac[4]);
    }
    bool operator==(const EtherAddr& rhs) const { return IsEqualTo(rhs); }
    void Print() const;
  };
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

 private:
  static Network* network_;

  ARPTable arp_table_;
  RingBuffer<PacketContainer, kRXBufferSize> rx_buffer_;

  Network(){};
};

void NetworkManager();
void SendARPRequest(Network::IPv4Addr);
void SendARPRequest(const char*);
