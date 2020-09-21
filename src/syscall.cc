#include <stdio.h>

#include "liumos.h"

#include "virtio_net.h"

#include "kernel.h"

constexpr uint64_t kSyscallIndex_sys_read = 0;
constexpr uint64_t kSyscallIndex_sys_write = 1;
constexpr uint64_t kSyscallIndex_sys_close = 3;
constexpr uint64_t kSyscallIndex_sys_socket = 41;
constexpr uint64_t kSyscallIndex_sys_sendto = 44;
constexpr uint64_t kSyscallIndex_sys_exit = 60;
constexpr uint64_t kSyscallIndex_arch_prctl = 158;
// constexpr uint64_t kArchSetGS = 0x1001;
constexpr uint64_t kArchSetFS = 0x1002;
// constexpr uint64_t kArchGetFS = 0x1003;
// constexpr uint64_t kArchGetGS = 0x1004;

extern "C" uint64_t GetCurrentKernelStack(void) {
  ExecutionContext& ctx =
      liumos->scheduler->GetCurrentProcess().GetExecutionContext();
  return ctx.GetKernelRSP();
}
__attribute__((ms_abi)) extern "C" void SyscallHandler(uint64_t* args) {
  // This function will be called under exceptions are masked
  // with Kernel Stack
  uint64_t idx = args[0];
  if (idx == kSyscallIndex_sys_read) {
    const uint64_t fildes = args[1];
    uint8_t* buf = reinterpret_cast<uint8_t*>(args[2]);
    uint64_t nbyte = args[3];
    if (fildes != 0) {
      PutStringAndHex("fildes", fildes);
      Panic("Only stdin is supported for now.");
    }
    if (nbyte < 1)
      return;

    uint16_t keyid;
    while ((keyid = liumos->main_console->GetCharWithoutBlocking()) ==
               KeyID::kNoInput ||
           (keyid & KeyID::kMaskBreak)) {
      StoreIntFlagAndHalt();
    }

    buf[0] = keyid;
    return;
  }
  if (idx == kSyscallIndex_sys_write) {
    uint64_t t0 = liumos->hpet->ReadMainCounterValue();
    const uint64_t fildes = args[1];
    const uint8_t* buf = reinterpret_cast<uint8_t*>(args[2]);
    uint64_t nbyte = args[3];
    if (fildes != 1) {
      PutStringAndHex("fildes", fildes);
      Panic("Only stdout is supported for now.");
    }
    while (nbyte--) {
      PutChar(*(buf++));
    }
    uint64_t t1 = liumos->hpet->ReadMainCounterValue();
    liumos->scheduler->GetCurrentProcess().AddSysTimeFemtoSec(
        (t1 - t0) * liumos->hpet->GetFemtosecondPerCount());
    return;
  }
  if (idx == kSyscallIndex_sys_close) {
    args[0] = 0;
    return;
  }
  if (idx == kSyscallIndex_sys_exit) {
    const uint64_t exit_code = args[1];
    PutStringAndHex("exit: exit_code", exit_code);
    liumos->scheduler->KillCurrentProcess();
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
    int domain = static_cast<int>(args[1]);
    int type = static_cast<int>(args[2]);
    int protocol = static_cast<int>(args[3]);
    constexpr int kDomainIPv4 = 2;
    if (domain != kDomainIPv4) {
      Panic("domain != IPv4");
    }
    constexpr int kTypeRawSocket = 3;
    if (type != kTypeRawSocket) {
      Panic("type != kTypeRawSocket");
    }
    constexpr int kProtocolICMP = 1;
    if (protocol != kProtocolICMP) {
      Panic("protocol != kProtocolICMP");
    }
    PutString("socket(IPv4, Raw, ICMP)\n");
    args[1] = 3;
    return;
  }
  if (idx == kSyscallIndex_sys_sendto) {
    kprintf("sendto()!\n");
    {
      using Net = Virtio::Net;
      using IPv4Packet = Virtio::Net::IPv4Packet;
      using ICMPPacket = Virtio::Net::ICMPPacket;
      Net& net = Net::GetInstance();
      ICMPPacket& icmp =
          *net.GetNextTXPacketBuf<ICMPPacket*>(sizeof(ICMPPacket));
      // ip.eth
      icmp.ip.eth.dst = {0xf8, 0xff, 0xc2, 0x01, 0xdf, 0x39};
      icmp.ip.eth.src = net.GetSelfEtherAddr();
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
      icmp.ip.src_ip = net.GetSelfIPv4Addr();
      icmp.ip.dst_ip = {10, 10, 10, 90};
      icmp.ip.CalcAndSetChecksum();
      // icmp
      icmp.type = ICMPPacket::Type::kEchoReply;
      icmp.code = 0;
      icmp.identifier = 0;
      icmp.sequence = 0;
      icmp.csum.Clear();
      icmp.csum = Net::CalcChecksum(&icmp, offsetof(ICMPPacket, type),
                                    sizeof(ICMPPacket));
      // send
      net.SendPacket();
    }
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
