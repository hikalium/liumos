#include <stdio.h>

#include "liumos.h"

#include "virtio_net.h"

#include "kernel.h"

constexpr uint64_t kSyscallIndex_sys_read = 0;
constexpr uint64_t kSyscallIndex_sys_write = 1;
constexpr uint64_t kSyscallIndex_sys_close = 3;
constexpr uint64_t kSyscallIndex_sys_socket = 41;
constexpr uint64_t kSyscallIndex_sys_sendto = 44;
constexpr uint64_t kSyscallIndex_sys_recvfrom = 45;
constexpr uint64_t kSyscallIndex_sys_bind = 49;
constexpr uint64_t kSyscallIndex_sys_exit = 60;
constexpr uint64_t kSyscallIndex_arch_prctl = 158;
// constexpr uint64_t kArchSetGS = 0x1001;
constexpr uint64_t kArchSetFS = 0x1002;
// constexpr uint64_t kArchGetFS = 0x1003;
// constexpr uint64_t kArchGetGS = 0x1004;

// https://elixir.bootlin.com/linux/v4.15/source/include/uapi/asm-generic/errno-base.h#L6
enum ErrorNumber {
  kBadFileDescriptor = -9,
  kInvalid = -22,
};

// c.f.
// https://elixir.bootlin.com/linux/v4.15/source/include/uapi/linux/in.h#L232
// sockaddr_in means sockaddr for InterNet protocol(IP)
struct sockaddr_in {
  uint16_t sin_family;
  uint16_t sin_port;
  Network::IPv4Addr sin_addr;
  uint8_t padding[8];
  // c.f.
  // https://elixir.bootlin.com/linux/v4.15/source/include/uapi/linux/in.h#L231
};
typedef uint32_t socklen_t;

extern "C" uint64_t GetCurrentKernelStack(void) {
  ExecutionContext& ctx =
      liumos->scheduler->GetCurrentProcess().GetExecutionContext();
  if (int a = 0; a) {
  }
  return ctx.GetKernelRSP();
}

static bool IsICMPPacket(void* frame_data, size_t frame_size) {
  using EtherFrame = Network::EtherFrame;
  using IPv4Packet = Network::IPv4Packet;
  if (frame_size < sizeof(IPv4Packet)) {
    return false;
  }
  EtherFrame& eth = *reinterpret_cast<EtherFrame*>(frame_data);
  if (!eth.HasEthType(EtherFrame::kTypeIPv4)) {
    return false;
  }
  IPv4Packet& p = *reinterpret_cast<IPv4Packet*>(frame_data);
  if (p.protocol != IPv4Packet::Protocol::kICMP) {
    return false;
  }
  return true;
}

static ssize_t sys_recvfrom(int64_t sockfd,
                            void* buf,
                            size_t buf_size,
                            int64_t,
                            struct sockaddr_in*,
                            size_t*) {
  if (sockfd != 3) {
    kprintf("%s: sockfd = %d is not supported yet.\n", __func__, sockfd);
    return -1;
  }
  using EtherFrame = Network::EtherFrame;
  Network& network = Network::GetInstance();
  for (;;) {
    while (network.HasPacketInRXBuffer()) {
      auto packet = network.PopFromRXBuffer();
      if (!IsICMPPacket(packet.data, packet.size)) {
        continue;
      }
      size_t ip_data_size = packet.size - sizeof(EtherFrame);
      size_t copy_size = std::min(ip_data_size, buf_size);
      memcpy(buf, &packet.data[sizeof(EtherFrame)], copy_size);
      return ip_data_size;
    }
    Sleep();
  }
  return 0;
}

static int sys_socket(int domain, int type, int protocol) {
  constexpr int kDomainIPv4 = 2;
  constexpr int kTypeRawSocket = 3;
  constexpr int kProtocolICMP = 1;
  if (domain == kDomainIPv4 && type == kTypeRawSocket &&
      protocol == kProtocolICMP) {
    kprintf("%s: socket created (IPv4, Raw, ICMP)\n", __func__);
    return 3 /* Always returns 3 for now */;
  }
  kprintf("%s: socket(%d, %d, %d) is not supported yet\n", __func__, domain,
          type, protocol);
  return ErrorNumber::kInvalid;
}

static int sys_bind(int sockfd, sockaddr_in* addr, socklen_t addrlen) {
  Network& network = Network::GetInstance();
  auto pid = liumos->scheduler->GetCurrentProcess().GetID();
  auto sock_holder = network.FindSocket(pid, sockfd);
  if (!sock_holder.has_value()) {
    kprintf("%s: fd %d is not a socket\n", __func__, sockfd);
    return ErrorNumber::kInvalid;
  }
  kprintf("%s: bind(%d, %p, %d)\n", __func__, sockfd, addr, addrlen);
  return ErrorNumber::kInvalid;
}

static ssize_t sys_read(int fd, void* buf, size_t count) {
  if (fd != 0) {
    kprintf("%s: fd %d is not supported yet: only stdin is supported now.\n",
            __func__, fd);
    return ErrorNumber::kInvalid;
  }
  if (count < 1)
    return ErrorNumber::kInvalid;
  uint16_t keyid;
  while ((keyid = liumos->main_console->GetCharWithoutBlocking()) ==
             KeyID::kNoInput ||
         (keyid & KeyID::kMaskBreak)) {
    StoreIntFlagAndHalt();
  }
  reinterpret_cast<uint8_t*>(buf)[0] = keyid;
  return 1;
}

__attribute__((ms_abi)) extern "C" void SyscallHandler(uint64_t* args) {
  // This function will be called under exceptions are masked
  // with Kernel Stack
  uint64_t idx = args[0];
  if (idx == kSyscallIndex_sys_read) {
    args[0] = sys_read(static_cast<int>(args[1]),
                       reinterpret_cast<void*>(args[2]), args[3]);
    return;
  }
  if (idx == kSyscallIndex_sys_write) {
    const uint64_t fildes = args[1];
    const uint8_t* buf = reinterpret_cast<uint8_t*>(args[2]);
    uint64_t nbyte = args[3];
    if (fildes != 1) {
      kprintf("%s: fd = %d is not supported yet\n", __func__, fildes);
      args[0] = ErrorNumber::kBadFileDescriptor;
      return;
    }
    if ((nbyte >> 63)) {
      kprintf("%s: fd = %llu is too big. May be negative?\n", __func__, nbyte);
      args[0] = ErrorNumber::kInvalid;
      return;
    }
    while (nbyte--) {
      PutChar(*(buf++));
    }
    return;
  }
  if (idx == kSyscallIndex_sys_close) {
    args[0] = 0;
    return;
  }
  if (idx == kSyscallIndex_sys_exit) {
    if (liumos->debug_mode_enabled) {
      const uint64_t exit_code = args[1];
      PutStringAndHex("exit: exit_code", exit_code);
    }
    liumos->scheduler->KillCurrentProcess();
    Sleep();
    for (;;) {
      StoreIntFlagAndHalt();
    };
    return;
  }
  if (idx == kSyscallIndex_arch_prctl) {
    Panic("arch_prctl!");
    if (args[1] == kArchSetFS) {
      WriteMSR(MSRIndex::kFSBase, args[2]);
      return;
    }
    PutStringAndHex("arg1", args[1]);
    PutStringAndHex("arg2", args[2]);
    PutStringAndHex("arg3", args[3]);
  }
  if (idx == kSyscallIndex_sys_socket) {
    args[0] = sys_socket(static_cast<int>(args[1]), static_cast<int>(args[2]),
                         static_cast<int>(args[3]));
    return;
  }
  if (idx == kSyscallIndex_sys_sendto) {
    using Net = Virtio::Net;
    using IPv4Packet = Virtio::Net::IPv4Packet;
    using ICMPPacket = Virtio::Net::ICMPPacket;
    using IPv4Addr = Network::IPv4Addr;
    using EtherAddr = Network::EtherAddr;

    Net& virtio_net = Net::GetInstance();
    Network& network = Network::GetInstance();

    IPv4Addr target_ip_addr = reinterpret_cast<sockaddr_in*>(args[5])->sin_addr;
    EtherAddr target_eth_addr;
    for (;;) {
      auto target_eth_container = network.ResolveIPv4(target_ip_addr);
      if (target_eth_container.has_value()) {
        target_eth_addr = *target_eth_container;
        break;
      }
      SendARPRequest(target_ip_addr);
      liumos->hpet->BusyWait(100);
    }
    ICMPPacket& icmp = *virtio_net.GetNextTXPacketBuf<ICMPPacket*>(
        sizeof(IPv4Packet) + args[3]);
    // ip.eth
    icmp.ip.eth.dst = target_eth_addr;
    icmp.ip.eth.src = virtio_net.GetSelfEtherAddr();
    icmp.ip.eth.SetEthType(Net::EtherFrame::kTypeIPv4);
    // ip
    icmp.ip.version_and_ihl =
        0x45;  // IPv4, header len = 5 * sizeof(uint32_t) = 20 bytes
    icmp.ip.dscp_and_ecn = 0;
    icmp.ip.SetDataLength(sizeof(ICMPPacket) - sizeof(IPv4Packet));
    icmp.ip.ident = 0;
    icmp.ip.flags = 0;
    icmp.ip.ttl = 0xFF;
    icmp.ip.protocol = Net::IPv4Packet::Protocol::kICMP;
    icmp.ip.src_ip = virtio_net.GetSelfIPv4Addr();
    icmp.ip.dst_ip = target_ip_addr;
    icmp.ip.CalcAndSetChecksum();
    // icmp
    memcpy(&icmp.type /*first member of ICMP*/,
           reinterpret_cast<void*>(args[2]), args[3]);
    // send
    virtio_net.SendPacket();
    return;
  }
  if (idx == kSyscallIndex_sys_recvfrom) {
    args[0] = sys_recvfrom(args[1], reinterpret_cast<void*>(args[2]), args[3],
                           args[4], reinterpret_cast<sockaddr_in*>(args[5]),
                           reinterpret_cast<size_t*>(args[6]));
    return;
  }
  if (idx == kSyscallIndex_sys_bind) {
    args[0] = sys_bind(static_cast<int>(args[1]),
                       reinterpret_cast<struct sockaddr_in*>(args[2]),
                       static_cast<socklen_t>(args[3]));
    return;
  }
  char s[64];
  snprintf(s, sizeof(s), "Unhandled syscall. rax = %lu\n", idx);
  PutString(s);
  liumos->scheduler->KillCurrentProcess();
  for (;;) {
    StoreIntFlagAndHalt();
  };
}

void EnableSyscall() {
  uint64_t star = static_cast<uint64_t>(GDT::kKernelCSSelector) << 32;
  star |= static_cast<uint64_t>(GDT::kUserCS32Selector) << 48;
  WriteMSR(MSRIndex::kSTAR, star);

  uint64_t lstar = reinterpret_cast<uint64_t>(AsmSyscallHandler);
  WriteMSR(MSRIndex::kLSTAR, lstar);

  WriteMSR(MSRIndex::kFMASK, 1ULL << 9);

  uint64_t efer = ReadMSR(MSRIndex::kEFER);
  efer |= 1;  // SCE
  WriteMSR(MSRIndex::kEFER, efer);
}
