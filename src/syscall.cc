#include "liumos.h"

constexpr uint64_t kSyscallIndex_sys_write = 1;
constexpr uint64_t kSyscallIndex_sys_exit = 60;
constexpr uint64_t kSyscallIndex_arch_prctl = 158;
// constexpr uint64_t kArchSetGS = 0x1001;
constexpr uint64_t kArchSetFS = 0x1002;
// constexpr uint64_t kArchGetFS = 0x1003;
// constexpr uint64_t kArchGetGS = 0x1004;

__attribute__((ms_abi)) extern "C" void SyscallHandler(uint64_t* args) {
  uint64_t idx = args[0];
  if (idx == kSyscallIndex_sys_write) {
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
    return;
  } else if (idx == kSyscallIndex_sys_exit) {
    const uint64_t exit_code = args[1];
    PutStringAndHex("exit: exit_code", exit_code);
    liumos->scheduler->KillCurrentContext();
    ExecutionContext* ctx = liumos->scheduler->GetCurrentContext();
    ChangeRSP(ctx->GetKernelRSP());
    for (;;) {
      StoreIntFlagAndHalt();
    };
  } else if (idx == kSyscallIndex_arch_prctl) {
    Panic("arch_prctl!");
    if (args[1] == kArchSetFS) {
      WriteMSR(MSRIndex::kFSBase, args[2]);
      return;
    }
    PutStringAndHex("arg1", args[1]);
    PutStringAndHex("arg2", args[2]);
    PutStringAndHex("arg3", args[3]);
  }
  PutStringAndHex("idx", idx);
  Panic("syscall handler!");
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
