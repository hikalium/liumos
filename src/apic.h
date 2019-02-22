#pragma once
#include "generic.h"

class LocalAPIC {
 public:
  void Init(void);
  uint8_t GetID() { return id_; }
  void SendEndOfInterrupt(void);

 private:
  uint32_t* GetRegisterAddr(uint64_t offset) {
    return (uint32_t*)(base_addr_ + offset);
  }

  uint64_t base_addr_;
  uint32_t id_;
  bool is_x2apic_;
};

void InitIOAPIC(uint64_t local_apic_id);
