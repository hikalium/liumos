#include <stdio.h>
#include <unordered_map>

#include "liumos.h"

#include "virtio_net.h"

#include "kernel.h"

constexpr uint64_t kSyscallIndex_sys_read = 0;
constexpr uint64_t kSyscallIndex_sys_write = 1;
constexpr uint64_t kSyscallIndex_sys_open = 2;
constexpr uint64_t kSyscallIndex_sys_close = 3;
constexpr uint64_t kSyscallIndex_sys_mmap = 9;
constexpr uint64_t kSyscallIndex_sys_msync = 26;
constexpr uint64_t kSyscallIndex_sys_socket = 41;
constexpr uint64_t kSyscallIndex_sys_sendto = 44;
constexpr uint64_t kSyscallIndex_sys_recvfrom = 45;
constexpr uint64_t kSyscallIndex_sys_bind = 49;
constexpr uint64_t kSyscallIndex_sys_exit = 60;
constexpr uint64_t kSyscallIndex_sys_ftruncate = 77;
constexpr uint64_t kSyscallIndex_sys_getdents64 = 217;
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

struct PerProcessSyscallData {
  Sheet* window_sheet;
  int num_getdents64_called;
  // for opening normal file: fd = 6
  int idx_in_root_files;
  uint64_t read_offset;
};

std::unordered_map<Process::PID, PerProcessSyscallData>
    per_process_syscall_data;

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

static bool IsUDPPacketToPort(void* frame_data,
                              size_t frame_size,
                              uint16_t port) {
  using EtherFrame = Network::EtherFrame;
  using IPv4Packet = Network::IPv4Packet;
  using IPv4UDPPacket = Network::IPv4UDPPacket;
  if (frame_size < sizeof(IPv4UDPPacket)) {
    return false;
  }
  EtherFrame& eth = *reinterpret_cast<EtherFrame*>(frame_data);
  if (!eth.HasEthType(EtherFrame::kTypeIPv4)) {
    return false;
  }
  IPv4Packet& p = *reinterpret_cast<IPv4Packet*>(frame_data);
  if (p.protocol != IPv4Packet::Protocol::kUDP) {
    return false;
  }
  IPv4UDPPacket& udp = *reinterpret_cast<IPv4UDPPacket*>(frame_data);
  if (udp.GetDestinationPort() != port) {
    return false;
  }
  return true;
}

static ssize_t sys_recvfrom(int sockfd,
                            void* buf,
                            size_t buf_size,
                            int64_t,
                            struct sockaddr_in* recv_addr,
                            socklen_t*) {
  /* returns -1 on failure */
  using IPv4Packet = Network::IPv4Packet;
  using IPv4UDPPacket = Network::IPv4UDPPacket;
  using ICMPPacket = Network::ICMPPacket;
  using EtherFrame = Network::EtherFrame;
  using Socket = Network::Socket;
  Network& network = Network::GetInstance();
  auto pid = liumos->scheduler->GetCurrentProcess().GetID();
  auto sock_holder = network.FindSocket(pid, sockfd);
  if (!sock_holder.has_value()) {
    kprintf("%s: fd %d is not a socket\n", __func__, sockfd);
    return -1;
  }
  Socket::Type socket_type = (*sock_holder).type;
  if (socket_type == Socket::Type::kICMPDatagram) {
    for (;;) {
      while (network.HasPacketInRXBuffer()) {
        auto packet = network.PopFromRXBuffer();
        if (!IsICMPPacket(packet.data, packet.size)) {
          continue;
        }
        ICMPPacket& icmp = *reinterpret_cast<ICMPPacket*>(packet.data);
        size_t icmp_data_size = packet.size - sizeof(IPv4Packet);
        size_t copy_size = std::min(icmp_data_size, buf_size);
        memcpy(buf, &icmp.type, copy_size);
        recv_addr->sin_addr = icmp.ip.src_ip;
        return icmp_data_size;
      }
      Sleep();
    }
    return -1;
  }
  if (socket_type == Socket::Type::kICMPRaw) {
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
    return -1;
  }
  if (socket_type == Socket::Type::kUDP) {
    uint16_t port = (*sock_holder).listen_port;
    for (;;) {
      while (network.HasPacketInRXBuffer()) {
        auto packet = network.PopFromRXBuffer();
        if (!IsUDPPacketToPort(packet.data, packet.size, port)) {
          continue;
        }
        size_t udp_data_size = packet.size - sizeof(IPv4UDPPacket);
        size_t copy_size = std::min(udp_data_size, buf_size);
        memcpy(buf, &packet.data[sizeof(IPv4UDPPacket)], copy_size);
        IPv4UDPPacket* udp_packet =
            reinterpret_cast<IPv4UDPPacket*>(packet.data);
        recv_addr->sin_addr = udp_packet->ip.src_ip;
        recv_addr->sin_port =
            *reinterpret_cast<uint16_t*>(&udp_packet->src_port);
        return udp_data_size;
      }
      Sleep();
    }
    return -1;
  }
  kprintf("%s: socket_type = %d is not a supported yet\n", __func__,
          socket_type);
  return -1;
}

static int sys_socket(int domain, int type, int protocol) {
  /* returns -1 on failure */
  constexpr int kDomainIPv4 = 2;
  constexpr int kTypeDatagram = 2; /* UDP under kDomainIPv4 */
  constexpr int kTypeRawSocket = 3;
  constexpr int kProtocolICMP = 1;
  Network& network = Network::GetInstance();
  auto pid = liumos->scheduler->GetCurrentProcess().GetID();
  const int sockfd = 3; /* Always assign 3 for now */
  if (domain == kDomainIPv4) {
    if (type == kTypeDatagram && protocol == kProtocolICMP) {
      if (network.RegisterSocket(pid, sockfd,
                                 Network::Socket::Type::kICMPDatagram)) {
        kprintf("kernel: %s: failed to register socket.\n", __func__);
        return -1 /* Return -1 on error */;
      }
      kprintf("kernel: %s: socket (fd=%d) created (IPv4, DGRAM, ICMP)\n",
              __func__, sockfd);
      return sockfd /* Always returns 3 for now */;
    }
    if (type == kTypeRawSocket && protocol == kProtocolICMP) {
      if (network.RegisterSocket(pid, sockfd,
                                 Network::Socket::Type::kICMPRaw)) {
        kprintf("kernel: %s: failed to register socket.\n", __func__);
        return -1 /* Return -1 on error */;
      }
      kprintf("kernel: %s: socket (fd=%d) created (IPv4, Raw, ICMPRaw)\n",
              __func__, sockfd);
      return sockfd /* Always returns 3 for now */;
    }
    if (type == kTypeDatagram && (protocol == 0 || protocol == 17)) {
      /* UDP */
      if (network.RegisterSocket(pid, sockfd, Network::Socket::Type::kUDP)) {
        kprintf("kernel: %s: failed to register socket.\n", __func__);
        return -1 /* Return -1 on error */;
      }
      kprintf("kernel: %s: socket (fd=%d) created (IPv4, DGRAM, %d)(UDP)\n",
              __func__, sockfd, protocol);
      return sockfd;
    }
  }
  kprintf("kernel: %s: socket(%d, %d, %d) is not supported yet\n", __func__,
          domain, type, protocol);
  return -1 /* Return -1 on error */;
}

static int sys_bind(int sockfd, sockaddr_in* addr, socklen_t addrlen) {
  /* returns -1 on failure */
  Network& network = Network::GetInstance();
  auto pid = liumos->scheduler->GetCurrentProcess().GetID();
  auto sock_holder = network.FindSocket(pid, sockfd);
  if (!sock_holder.has_value()) {
    kprintf("%s: fd %d is not a socket\n", __func__, sockfd);
    return -1;
  }
  if (network.BindToPort(
          pid, sockfd,
          ((addr->sin_port >> 8) & 0xFF) | (addr->sin_port << 8))) {
    kprintf("%s: BindToPort failed\n", __func__, sockfd);
    return -1;
  }
  kprintf("%s: bind(%d, %p, %d)\n", __func__, sockfd, addr, addrlen);
  return 0;
}

static ssize_t sys_read(int fd, void* buf, size_t count) {
  if (fd == 0) {
    if (count < 1)
      return ErrorNumber::kInvalid;
    auto& proc_stdin = liumos->scheduler->GetCurrentProcess().GetStdIn();
    while (proc_stdin.IsEmpty()) {
      StoreIntFlagAndHalt();
    }
    reinterpret_cast<uint8_t*>(buf)[0] = proc_stdin.Pop();
    return 1;
  }
  if (fd == 6) {
    auto pid = liumos->scheduler->GetCurrentProcess().GetID();
    auto& ppdata = per_process_syscall_data[pid];
    LoaderInfo& loader_info = *reinterpret_cast<LoaderInfo*>(
        reinterpret_cast<uint64_t>(&GetLoaderInfo()) +
        GetKernelStraightMappingBase());
    EFIFile& file = loader_info.root_files[ppdata.idx_in_root_files];
    uint64_t file_size = file.GetFileSize();
    const uint8_t* src = reinterpret_cast<const uint8_t*>(
        reinterpret_cast<uint64_t>(file.GetBuf()) +
        GetKernelStraightMappingBase());
    assert(file_size >= ppdata.read_offset);
    uint64_t copy_size = std::min(file_size - ppdata.read_offset, count);
    if (copy_size) {
      memcpy(buf, src + ppdata.read_offset, copy_size);
      ppdata.read_offset += copy_size;
    }
    return copy_size;
  }
  kprintf("%s: fd %d is not supported yet: only stdin is supported now.\n",
          __func__, fd);
  return ErrorNumber::kInvalid;
}

static std::optional<Network::EtherAddr> ResolveIPv4WithTimeout(
    Network::IPv4Addr dst_ip_addr,
    uint64_t timeout_ms) {
  Network& network = Network::GetInstance();
  uint64_t time_passed_ms = 0;
  constexpr uint64_t kWaitTimePerTryMs = 200;
  Network::IPv4Addr nexthop_ip_addr;
  if (dst_ip_addr.IsInSameSubnet(network.GetIPv4DefaultGateway(),
                                 network.GetIPv4NetMask())) {
    kprintf("kernel: dst is in the same subnet.\n");
    nexthop_ip_addr = dst_ip_addr;
  } else {
    kprintf("kernel: dst is in a different subnet.\n");
    nexthop_ip_addr = network.GetIPv4DefaultGateway();
  }
  while (time_passed_ms < timeout_ms) {
    auto eth_container = network.ResolveIPv4(nexthop_ip_addr);
    if (eth_container.has_value()) {
      kprintf("kernel: ARP entry found!\n");
      return eth_container;
    }
    SendARPRequest(nexthop_ip_addr);
    kprintf("kernel: ARP request sent to %d.%d.%d.%d...\n",
            nexthop_ip_addr.addr[0], nexthop_ip_addr.addr[1],
            nexthop_ip_addr.addr[2], nexthop_ip_addr.addr[3]);
    Sleep();
    HPET::GetInstance().BusyWait(kWaitTimePerTryMs);
    time_passed_ms += kWaitTimePerTryMs;
  }
  kprintf("kernel: ARP resolution failed. (timeout)\n");
  return std::nullopt;
}

static ssize_t sys_sendto(int sockfd,
                          const void* buf,
                          size_t len,
                          int /*flags*/,
                          const struct sockaddr_in* dest_addr,
                          socklen_t /*addrlen*/) {
  using Net = Virtio::Net;
  using IPv4Packet = Virtio::Net::IPv4Packet;
  using IPv4Addr = Network::IPv4Addr;
  using EtherAddr = Network::EtherAddr;
  using Socket = Network::Socket;

  Net& virtio_net = Net::GetInstance();
  Network& network = Network::GetInstance();
  auto pid = liumos->scheduler->GetCurrentProcess().GetID();
  auto sock_holder = network.FindSocket(pid, sockfd);
  if (!sock_holder.has_value()) {
    kprintf("%s: fd %d is not a socket\n", __func__, sockfd);
    return -1;
  }
  Socket::Type socket_type = (*sock_holder).type;

  IPv4Addr target_ip_addr = dest_addr->sin_addr;
  std::optional<EtherAddr> target_eth_addr_holder =
      ResolveIPv4WithTimeout(target_ip_addr, 1000);
  if (!target_eth_addr_holder.has_value()) {
    kprintf("%s: ARP resolution failed.\n", __func__);
    return -1;
  }
  if (socket_type == Network::Socket::Type::kICMPRaw ||
      socket_type == Network::Socket::Type::kICMPDatagram) {
    using ICMPPacket = Virtio::Net::ICMPPacket;
    ICMPPacket& icmp =
        *virtio_net.GetNextTXPacketBuf<ICMPPacket*>(sizeof(IPv4Packet) + len);
    // ip.eth
    icmp.ip.eth.dst = *target_eth_addr_holder;
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
    memcpy(&icmp.type /*first member of ICMP*/, buf, len);
    // send
    virtio_net.SendPacket();
    return len;
  }
  if (socket_type == Network::Socket::Type::kUDP) {
    len = (len + 1) & ~1;  // make size even
    using IPv4UDPPacket = Virtio::Net::IPv4UDPPacket;
    IPv4UDPPacket& udp = *virtio_net.GetNextTXPacketBuf<IPv4UDPPacket*>(
        sizeof(IPv4UDPPacket) + len);
    // ip.eth
    udp.ip.eth.dst = *target_eth_addr_holder;
    udp.ip.eth.src = virtio_net.GetSelfEtherAddr();
    udp.ip.eth.SetEthType(Net::EtherFrame::kTypeIPv4);
    // ip
    udp.ip.version_and_ihl =
        0x45;  // IPv4, header len = 5 * sizeof(uint32_t) = 20 bytes
    udp.ip.dscp_and_ecn = 0;
    udp.ip.SetDataLength(sizeof(IPv4UDPPacket) + len - sizeof(IPv4Packet));
    udp.ip.ident = 0;
    udp.ip.flags = 0;
    udp.ip.ttl = 0xFF;
    udp.ip.protocol = Net::IPv4Packet::Protocol::kUDP;
    udp.ip.src_ip = virtio_net.GetSelfIPv4Addr();
    udp.ip.dst_ip = target_ip_addr;
    udp.ip.CalcAndSetChecksum();
    // udp
    memcpy(reinterpret_cast<uint8_t*>(&udp) +
               sizeof(IPv4UDPPacket) /*right after the UDP header*/,
           buf, len);
    udp.SetSourcePort((*sock_holder).listen_port);
    *reinterpret_cast<uint16_t*>(&udp.dst_port) = dest_addr->sin_port;
    udp.SetDataSize(len);
    udp.csum = Network::CalcUDPChecksum(
        &udp, offsetof(IPv4UDPPacket, src_port), sizeof(IPv4UDPPacket) + len,
        udp.ip.src_ip, udp.ip.dst_ip, udp.length);
    // send
    virtio_net.SendPacket();
    return len;
  }
  kprintf("%s: socket_type = %d is not supported\n", __func__, socket_type);
  return -1;
}

packed_struct DirectoryEntry {
  uint64_t inode;        // +0
  uint64_t next_offset;  // +8
  uint16_t this_size;    // +16
  uint8_t d_type;        // +18
};
static_assert(sizeof(DirectoryEntry) == 19);

static ssize_t sys_getdents64(int, void* buf, size_t) {
  auto pid = liumos->scheduler->GetCurrentProcess().GetID();
  auto& ppdata = per_process_syscall_data[pid];

  if (ppdata.num_getdents64_called != 0) {
    return 0;
  }
  ppdata.num_getdents64_called++;

  DirectoryEntry* de = reinterpret_cast<DirectoryEntry*>(buf);
  const char* file_name = "TEST_FILE.bin";
  size_t file_name_len = strlen(file_name);
  de->inode = 123;
  de->next_offset = 0;
  de->this_size = sizeof(DirectoryEntry) + file_name_len + 1;
  char* dst = reinterpret_cast<char*>(de + 1);
  for (size_t i = 0; i < file_name_len + 1; i++) {
    dst[i] = file_name[i];
  }
  return de->this_size;
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
  if (idx == kSyscallIndex_sys_open) {
    auto pid = liumos->scheduler->GetCurrentProcess().GetID();
    auto& ppdata = per_process_syscall_data[pid];

    const char* file_name = reinterpret_cast<const char*>(args[1]);
    kprintf("file name: %s\n", file_name);

    if (IsEqualString(".", file_name)) {
      ppdata.num_getdents64_called = 0;
      args[0] = 5;
      return;
    }

    if (IsEqualString("window.bmp", file_name)) {
      args[0] = 7;
      return;
    }

    int found_idx = -1;

    LoaderInfo& loader_info = *reinterpret_cast<LoaderInfo*>(
        reinterpret_cast<uint64_t>(&GetLoaderInfo()) +
        GetKernelStraightMappingBase());
    kprintf("&GetLoaderInfo(): %p\n", &GetLoaderInfo());
    kprintf("&loader_info: %p\n", &loader_info);
    for (int i = 0; i < loader_info.root_files_used; i++) {
      if (IsEqualString(loader_info.root_files[i].GetFileName(), file_name)) {
        found_idx = i;
        break;
      }
    }
    if (found_idx == -1) {
      kprintf("not found!\n");
      *((int64_t*)&args[0]) = -1;
      return;
    }
    kprintf("found at index = %d!\n", found_idx);
    ppdata.idx_in_root_files = found_idx;
    ppdata.read_offset = 0;
    args[0] = 6;
    return;
  }
  if (idx == kSyscallIndex_sys_close) {
    args[0] = 0;
    return;
  }
  if (idx == kSyscallIndex_sys_mmap) {
    uint64_t size = args[2];
    uint64_t fd = args[5];
    if (fd != 7) {
      kprintf("fd != 7\n");
      args[0] = static_cast<uint64_t>(-1);
      return;
    }
    uint64_t map_size = size;
    kprintf("window_fb_map_size = %d\n", map_size);
    uint8_t* buf_kernel = AllocKernelMemory<uint8_t*>(map_size);
    uint64_t phys_addr = v2p(buf_kernel);
    uint64_t user_cr3 = ReadCR3();
    WriteCR3(liumos->kernel_pml4_phys);
    CreatePageMapping(GetSystemDRAMAllocator(),
                      *reinterpret_cast<IA_PML4*>(user_cr3), 0x1'0000'0000,
                      phys_addr, map_size,
                      kPageAttrPresent | kPageAttrWritable | kPageAttrUser);
    WriteCR3(user_cr3);
    args[0] = 0x1'0000'0000;
    return;
  }
  if (idx == kSyscallIndex_sys_msync) {
    uint64_t addr = args[1];
    if (addr != 0x1'0000'0000) {
      kprintf("invalid addr");
      args[0] = static_cast<uint64_t>(-1);
      return;
    }
    uint32_t offset_to_data = *reinterpret_cast<uint32_t*>(addr + 10);
    int32_t xsize = *reinterpret_cast<uint32_t*>(addr + 18);
    int32_t ysize = *reinterpret_cast<uint32_t*>(addr + 22);
    if (ysize >= 0) {
      kprintf("ysize should be negative\n");
      args[0] = static_cast<uint64_t>(-1);
      return;
    }
    ysize = -ysize;
    auto pid = liumos->scheduler->GetCurrentProcess().GetID();
    auto& ppdata = per_process_syscall_data[pid];
    auto& sheet = ppdata.window_sheet;
    if (!sheet) {
      kprintf("offset_to_data = %d, xsize = %d, ysize = %d\n", offset_to_data,
              xsize, ysize);
      // TODO(hikalium): allocate this backing sheet on fopen.
      sheet = AllocKernelMemory<Sheet*>(sizeof(Sheet));
      bzero(sheet, sizeof(Sheet));
      sheet->Init(reinterpret_cast<uint32_t*>(addr + offset_to_data), xsize,
                  ysize, xsize, 0, 0);
      kprintf("vram_sheet is at %p\n", liumos->vram_sheet);
      sheet->SetParent(liumos->vram_sheet);
    }
    sheet->Flush(0, 0, xsize, ysize);
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
  if (idx == kSyscallIndex_sys_ftruncate) {
    uint64_t fd = args[1];
    uint64_t size = args[2];
    kprintf("ftruncate(fd=%d, size=%d)\n", fd, size);
    if (fd != 5) {
      args[0] = -1;
      return;
    }
    args[0] = 0;
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
    args[0] =
        sys_sendto(static_cast<int>(args[1]), reinterpret_cast<void*>(args[2]),
                   static_cast<size_t>(args[3]), static_cast<int>(args[4]),
                   reinterpret_cast<const sockaddr_in*>(args[5]),
                   static_cast<socklen_t>(args[6]));
    return;
  }
  if (idx == kSyscallIndex_sys_recvfrom) {
    args[0] = sys_recvfrom(
        static_cast<int>(args[1]), reinterpret_cast<void*>(args[2]), args[3],
        static_cast<int>(args[4]), reinterpret_cast<sockaddr_in*>(args[5]),
        reinterpret_cast<socklen_t*>(args[6]));
    return;
  }
  if (idx == kSyscallIndex_sys_bind) {
    args[0] = sys_bind(static_cast<int>(args[1]),
                       reinterpret_cast<struct sockaddr_in*>(args[2]),
                       static_cast<socklen_t>(args[3]));
    return;
  }
  if (idx == kSyscallIndex_sys_getdents64) {
    args[0] = sys_getdents64(static_cast<int>(args[1]),
                             reinterpret_cast<void*>(args[2]), args[3]);
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
