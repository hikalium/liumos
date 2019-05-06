#include <iostream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "elf.h"

int main(int argc, char *argv[]) {
  if(argc < 2) {
    std::cout << argv[0] << " <file_name>" << std::endl;
    return 0;
  }
  const char* file_name = argv[1];
  int fd = open(file_name, O_RDONLY);
  if(fd < 0) {
    perror("Open failed.");
    return 1;
  }
  struct stat s;
  fstat(fd, &s);
  size_t size = static_cast<size_t>(s.st_size);
  uint8_t *buf = reinterpret_cast<uint8_t*>(mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0));
  Elf64_Ehdr &ehdr = *reinterpret_cast<Elf64_Ehdr *>(buf);
  Elf64_Shdr &str_shdr = *reinterpret_cast<Elf64_Shdr *>(buf + ehdr.e_shoff + ehdr.e_shentsize * ehdr.e_shstrndx);
  std::vector<Elf64_Shdr*> shdr_list;
  std::vector<Elf64_Shdr*> shdr_no_mem_list;
  for(size_t sh_idx = 0; sh_idx < ehdr.e_shnum; sh_idx++){
    Elf64_Shdr &shdr = *reinterpret_cast<Elf64_Shdr *>(buf + ehdr.e_shoff + ehdr.e_shentsize * sh_idx);
    if(shdr.sh_addr) shdr_list.push_back(&shdr);
    else shdr_no_mem_list.push_back(&shdr);
  }
  std::sort(shdr_list.begin(), shdr_list.end(), [](const auto a, const auto b){ return a->sh_addr < b->sh_addr; });
  std::cout << "Segments on memory:" << std::endl;
  for(auto shdrp : shdr_list) {
    Elf64_Shdr &shdr = *shdrp;
    printf("[ 0x%08llx - 0x%08llx ) %s", shdr.sh_addr, shdr.sh_addr + shdr.sh_size, reinterpret_cast<char *>(&buf[str_shdr.sh_offset + shdr.sh_name]));
    fflush(stdout);
    if(shdr.sh_type == SHT_PROGBITS) std::cout << " PROGBITS";
    if(shdr.sh_type == SHT_NOBITS) std::cout << " NOBITS";
    std::cout << std::endl;
  }
  std::cout << std::endl;
  std::cout << "Segments not on memory:" << std::endl;
  for(auto shdrp : shdr_no_mem_list) {
    Elf64_Shdr &shdr = *shdrp;
    printf("0x%08llx %s", shdr.sh_addr + shdr.sh_size, reinterpret_cast<char *>(&buf[str_shdr.sh_offset + shdr.sh_name]));
    fflush(stdout);
    if(shdr.sh_type == SHT_PROGBITS) std::cout << " PROGBITS";
    if(shdr.sh_type == SHT_NOBITS) std::cout << " NOBITS";
    std::cout << std::endl;
  }
  return 0;
}
