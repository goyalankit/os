/*
 *
 * A simple program loader.
 *
 * References:
 * http://www.skyfree.org/linux/references/ELF_Format.pdf
 * http://linux.die.net/man/5/elf
 *
 * Stack Reference from http://articles.manugarg.com/aboutelfauxiliaryvectors.html
 *
position            content                     size (bytes) + comment
  ------------------------------------------------------------------------
  stack pointer ->  [ argc = number of args ]     4
                    [ argv[0] (pointer) ]         4   (program name)
                    [ argv[1] (pointer) ]         4
                    [ argv[..] (pointer) ]        4 * x
                    [ argv[n - 1] (pointer) ]     4
                    [ argv[n] (pointer) ]         4   (= NULL)

                    [ envp[0] (pointer) ]         4
                    [ envp[1] (pointer) ]         4
                    [ envp[..] (pointer) ]        4
                    [ envp[term] (pointer) ]      4   (= NULL)

                    [ auxv[0] (Elf32_auxv_t) ]    8
                    [ auxv[1] (Elf32_auxv_t) ]    8
                    [ auxv[..] (Elf32_auxv_t) ]   8
                    [ auxv[term] (Elf32_auxv_t) ] 8   (= AT_NULL vector)

                    [ padding ]                   0 - 16

                    [ argument ASCIIZ strings ]   >= 0
                    [ environment ASCIIZ str. ]   >= 0

  (0xbffffffc)      [ end marker ]                4   (= NULL)

  (0xc0000000)      < bottom of stack >           0   (virtual)
  ------------------------------------------------------------------------
 *
 *
 * Author: Ankit Goyal
 * */
#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <sys/stat.h>
#include "loader.h"
#include <unistd.h> // page size
#include <sys/types.h>
#include <sys/mman.h>
#include <assert.h>

unsigned long base_virtual_address;

// basic validation checks
void validate_elf(unsigned char *e_ident) {
  assert(e_ident[EI_MAG0] == ELFMAG0); // magic number 0x7f
  assert(e_ident[EI_MAG1] == ELFMAG1); // 'E'
  assert(e_ident[EI_MAG2] == ELFMAG2); // 'L'
  assert(e_ident[EI_MAG3] == ELFMAG3); // 'F'
  assert(e_ident[EI_CLASS] == ELFCLASS64); // 64 bit
  assert(e_ident[EI_DATA] == ELFDATA2LSB); // least significant byte -> lowest address(intel)
}

inline int getProt(Elf32_Word p_flags) {
  int prot = PROT_NONE;

  if (p_flags & PF_W)
    prot |= PROT_READ;
  if (p_flags & PF_R)
    prot |= PROT_WRITE;
  if (p_flags & PF_X)
    prot |= PROT_EXEC;

  return prot;
}

void *load_elf_binary(char* buf, int fd)
{
  /*
   * ELFheader is at a fixed location and has paramters
   * required to understand the other components in the
   * ELF file
   *
   * */
  Elf64_Ehdr *elfHeader;
  Elf64_Phdr *phHeader;
  unsigned char *e_ident;
  long pg_size;

  elfHeader = (Elf64_Ehdr *)buf;
  e_ident   = elfHeader->e_ident;

  // validate elf
  validate_elf(e_ident);

  // get program header that has information about parts that
  // needs to be loaded in memory.
  phHeader  = (Elf64_Phdr *)(buf + elfHeader->e_phoff);

  // get the page size
  ASSERT_I( (pg_size = sysconf(_SC_PAGE_SIZE)), "page size" );
  DEBUG("Page size: %ld\n", pg_size);

  int i, prot;
  unsigned long offsetInPg; // we need to add this number to tatal size
  unsigned long alignedPgAddr; // pg_size - 1 == 000000 => will give rounded up bits
  unsigned long mapSize; // total map size. includes bits wasted due to allignment
  unsigned long offsetInFile; // offset in file
  Elf32_Addr p_vaddr;
  int flags = MAP_PRIVATE | MAP_DENYWRITE;
  unsigned long k_bss;
  int base_address_set = 0;

  for (i = 0; i < elfHeader->e_phnum; ++i) {
    // skip if it's not a load
    if (phHeader[i].p_type != PT_LOAD)
      continue;
    p_vaddr       = phHeader[i].p_vaddr;
    offsetInPg    = p_vaddr & (pg_size - 1);
    alignedPgAddr = p_vaddr & ~(pg_size - 1);
    mapSize       = phHeader[i].p_filesz + offsetInPg;
    offsetInFile  = phHeader[i].p_offset - offsetInPg;
    prot          = getProt(phHeader[i].p_flags);

    if (elfHeader->e_type == ET_EXEC)
      flags |= MAP_FIXED;

    char *m_map;
    ASSERT_I( (m_map = mmap((caddr_t)alignedPgAddr, mapSize, prot,
            flags, fd, offsetInFile)), "mmap");

    CMP_AND_FAIL(m_map, (char *)alignedPgAddr, "Couldn't assign asked virtual address");

    // we shift the base address using statuc link.
    if (base_address_set == 0) {
      base_virtual_address = (unsigned long)m_map;
      base_address_set = 1;
    }

    // adjust the bss segment.
    // bss doesn't occupy any space in file where as it does in memory
    // Note that it is actually present as zero bytes in elf too.
    if (phHeader[i].p_memsz > phHeader[i].p_filesz) {
      alignedPgAddr = phHeader[i].p_vaddr + phHeader[i].p_filesz;
      // move to next page and find the boundary
      alignedPgAddr = (alignedPgAddr + pg_size - 1) & ~(pg_size - 1);
      mapSize = phHeader[i].p_memsz - phHeader[i].p_filesz;

      flags |= MAP_ANONYMOUS;
      // map this region to Anonymous which is "zeroed" out
      ASSERT_I( (m_map = mmap((caddr_t)alignedPgAddr, mapSize, prot,
              flags, -1, 0)), "mmap");
    }

    DEBUG("Address %p\n", (void *)phHeader[i].p_vaddr);
    unsigned long pg_offset = (p_vaddr & ~(pg_size -1));
    DEBUG("pg_offset: %li\n", pg_offset);
  }

  // the entry point for program to execute
  DEBUG("Entry Point: %p\n", (void *)elfHeader->e_entry);
  DEBUG("Base aaddress: %li\n", base_virtual_address);

  return (void *)elfHeader->e_entry;;
}

void modify_auxv(char *envp[], char *buf) {
  Elf64_Ehdr *elfHeader;
  elfHeader = (Elf64_Ehdr *)buf;
  size_t pg_size;
  ASSERT_I( (pg_size = sysconf(_SC_PAGE_SIZE)), "page size" );

  Elf64_auxv_t *auxv;

  while(*envp++ != NULL);

  for (auxv = (Elf64_auxv_t *)envp; auxv->a_type != AT_NULL; auxv++) {
    switch (auxv->a_type) {
      case AT_PHDR:
        {
          DEBUG("AT_PHDR set\n");
          auxv->a_un.a_val = (uint64_t)(buf + elfHeader->e_phoff);
          break;
        }
      case AT_PHENT:
        {
          DEBUG("AT_PHENT set\n");
          auxv->a_un.a_val = (uint64_t)(buf + elfHeader->e_phentsize);
          break;
        }
      case AT_PHNUM:
        {
          DEBUG("AT_PHNUM set\n");
          auxv->a_un.a_val =(uint64_t)(buf + elfHeader->e_phnum);
          break;
        }

      case AT_PAGESZ:
        {
          DEBUG("AT_PAGESZ set\n");
          auxv->a_un.a_val = pg_size;
          break;
        }

      case AT_BASE:
        {
          DEBUG("AT_BASE set\n");
          auxv->a_un.a_val = base_virtual_address;
          break;
        }

      case AT_ENTRY:
        {
          DEBUG("AT_ENTRY set\n");
          auxv->a_un.a_val = elfHeader->e_entry;
          break;
        }
      case AT_EXECFN:
        {
          DEBUG("AT_EXECFN set\n");
        }
      default:
        {
          break;
        }
    }
  }
}

  int
main(int argc, char* argv[], char* envp[])
{
  struct stat fstat;
  int size;
  FILE *fh;
  int fd;
  char *buf;
  void *e_entry;


  if (argc < 2) {
    printf("Usage: %s <Valid Object/Executable ELF file>\n", argv[0]);
    exit(1);
  }

  // open the binary file
  ASSERT_P( (fh = fopen(argv[1], "rb")), "open" );
  DEBUG("Opened the file\n");

  // stat the file to get size
  ASSERT_I( stat(argv[1], &fstat), "fstat" );
  size = fstat.st_size;
  DEBUG("File Size: %d\n", size);

  // allocate memory for file buffer
  ASSERT_P( (buf = (char *)malloc(sizeof(char) * size)), "malloc" );

  // read from file
  CMP_AND_FAIL(fread(buf, 1, size, fh), size, "fread");

  ASSERT_I((fd = fileno(fh)), "fileno");
  e_entry = load_elf_binary(buf, fd);
  modify_auxv(envp, buf);

  // close the file
  //ASSERT_I(fclose(fh), "fclose");
}





