#include "liumos.h"

constexpr uint64_t kSyscallIndex_sys_write = 1;
constexpr uint64_t kSyscallIndex_sys_exit = 60;
constexpr uint64_t kSyscallIndex_arch_prctl = 158;
// constexpr uint64_t kArchSetGS = 0x1001;
constexpr uint64_t kArchSetFS = 0x1002;
// constexpr uint64_t kArchGetFS = 0x1003;
// constexpr uint64_t kArchGetGS = 0x1004;

__attribute__((ms_abi)) extern "C" void SyscallHandler(uint64_t* args) {
  for (;;) {
  };
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
    for (;;) {
      StoreIntFlagAndHalt();
    };
  } else if (idx == kSyscallIndex_arch_prctl) {
    if (args[1] == kArchSetFS) {
      WriteMSR(MSRIndex::kFSBase, args[2]);
      return;
    }
    PutStringAndHex("arg1", args[1]);
    PutStringAndHex("arg2", args[2]);
    PutStringAndHex("arg3", args[3]);
    Panic("arch_prctl!");
  }
  PutStringAndHex("idx", idx);
  Panic("syscall handler!");
}
