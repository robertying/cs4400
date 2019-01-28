#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <elf.h>
#include "decode.h"

/* Given the in-memory ELF header pointer as `ehdr` and a section
   header pointer as `shdr`, returns a pointer to the memory that
   contains the in-memory content of the section */
#define AT_SEC(ehdr, shdr) ((void *)(ehdr) + (shdr)->sh_offset)

/*************************************************************/

Elf64_Shdr *find_section(Elf64_Ehdr *ehdr, char *section_name)
{
  Elf64_Shdr *shdrs = (void *)ehdr + ehdr->e_shoff;
  char *strs = AT_SEC(ehdr, shdrs + ehdr->e_shstrndx);
  int shdr_size = ehdr->e_shnum;

  int i;
  for (i = 0; i < shdr_size; i++)
  {
    if (!strcmp((char *)(shdrs[i].sh_name + strs), section_name))
    {
      return (Elf64_Shdr *)(shdrs + i);
    };
  }
  return (Elf64_Shdr *)(shdrs);
}

unsigned char *find_code(Elf64_Ehdr *ehdr, char *function_name)
{
  Elf64_Shdr *shdrs = (void *)ehdr + ehdr->e_shoff;
  char *strs = AT_SEC(ehdr, find_section(ehdr, ".dynstr"));
  Elf64_Shdr *sym_section = find_section(ehdr, ".dynsym");
  Elf64_Sym *syms = AT_SEC(ehdr, sym_section);
  int syms_size = sym_section->sh_size / sizeof(Elf64_Sym);

  int i;
  for (i = 0; i < syms_size; i++)
  {
    if (!strcmp((char *)(syms[i].st_name + strs), function_name))
    {
      int j = syms[i].st_shndx;
      return AT_SEC(ehdr, shdrs + j) + (syms[i].st_value - shdrs[j].sh_addr);
    };
  }
  return AT_SEC(ehdr, shdrs + syms[0].st_shndx) + (syms[0].st_value - shdrs[syms[0].st_shndx].sh_addr);
}

char *find_accessed_var_name(Elf64_Ehdr *ehdr, Elf64_Addr var_addr)
{
  Elf64_Shdr *rela_section = find_section(ehdr, ".rela.dyn");
  Elf64_Rela *relas = AT_SEC(ehdr, rela_section);
  char *strs = AT_SEC(ehdr, find_section(ehdr, ".dynstr"));
  Elf64_Sym *syms = AT_SEC(ehdr, find_section(ehdr, ".dynsym"));
  int relas_size = rela_section->sh_size / sizeof(Elf64_Rela);

  int i;
  for (i = 0; i < relas_size; i++)
  {
    if (relas[i].r_offset == var_addr)
    {
      int m = ELF64_R_SYM(relas[i].r_info);
      return strs + syms[m].st_name;
    };
  }
  return strs;
}

char *find_called_fun_name(Elf64_Ehdr *ehdr, Elf64_Addr jump_addr)
{
  Elf64_Shdr *rela_section = find_section(ehdr, ".rela.plt");
  Elf64_Rela *relas = AT_SEC(ehdr, rela_section);
  char *strs = AT_SEC(ehdr, find_section(ehdr, ".dynstr"));
  Elf64_Sym *syms = AT_SEC(ehdr, find_section(ehdr, ".dynsym"));
  int relas_size = rela_section->sh_size / sizeof(Elf64_Rela);

  int i;
  for (i = 0; i < relas_size; i++)
  {
    if (relas[i].r_offset == jump_addr)
    {
      int m = ELF64_R_SYM(relas[i].r_info);
      return strs + syms[m].st_name;
    };
  }
  return strs;
}

int starts_with(const char *str, const char *pre)
{
  return strncmp(pre, str, strlen(pre)) == 0;
}

void check_function(Elf64_Ehdr *ehdr, int status, code_t *code_ptr, Elf64_Addr code_addr)
{
  Elf64_Shdr *plt_section = find_section(ehdr, ".plt");
  code_t *plt_code_ptr = AT_SEC(ehdr, plt_section);
  Elf64_Addr plt_code_addr = plt_section->sh_addr;
  instruction_t ins;
  instruction_t jump_ins;

  do
  {
    decode(&ins, code_ptr, code_addr);

    switch (ins.op)
    {
    case MOV_ADDR_TO_REG_OP:
      if (starts_with(find_accessed_var_name(ehdr, ins.addr), "protected_"))
      {
        if (status == 0)
        {
          replace_with_crash(code_ptr, &ins);
        }
      }
      break;
    case JMP_TO_ADDR_OP:
      if (!strcmp(find_called_fun_name(ehdr, ins.addr), "open_it"))
      {
        if (status == 0)
        {
          status = 1;
        }
        else
        {
          replace_with_crash(code_ptr, &ins);
        }
      }
      if (!strcmp(find_called_fun_name(ehdr, ins.addr), "close_it"))
      {
        if (status == 1)
        {
          status = 0;
        }
        else
        {
          replace_with_crash(code_ptr, &ins);
        }
      }
      break;
    case MAYBE_JMP_TO_ADDR_OP:
      check_function(ehdr, status, code_ptr + ins.addr - code_addr, ins.addr);
      break;
    case CALL_OP:
      decode(&jump_ins, plt_code_ptr + ins.addr - plt_code_addr, ins.addr);
      if (!strcmp(find_called_fun_name(ehdr, jump_ins.addr), "open_it"))
      {
        if (status == 0)
        {
          status = 1;
        }
        else
        {
          replace_with_crash(code_ptr, &ins);
        }
      }
      if (!strcmp(find_called_fun_name(ehdr, jump_ins.addr), "close_it"))
      {
        if (status == 1)
        {
          status = 0;
        }
        else
        {
          replace_with_crash(code_ptr, &ins);
        }
      }
      break;
    case RET_OP:
      if (status == 1)
      {
        replace_with_crash(code_ptr, &ins);
      }
      break;
    default:
      break;
    }

    code_ptr += ins.length;
    code_addr += ins.length;
  } while (ins.op != RET_OP && ins.op != JMP_TO_ADDR_OP);
}

void enforce(Elf64_Ehdr *ehdr)
{
  Elf64_Shdr *shdrs = (void *)ehdr + ehdr->e_shoff;
  Elf64_Shdr *sym_section = find_section(ehdr, ".dynsym");
  Elf64_Sym *syms = AT_SEC(ehdr, sym_section);
  int syms_size = sym_section->sh_size / sizeof(Elf64_Sym);

  int i;
  for (i = 0; i < syms_size; i++)
  {
    int type = ELF64_ST_TYPE(syms[i].st_info);
    int size = syms[i].st_size;
    if (type == 2 && size != 0)
    {
      int j = syms[i].st_shndx;
      code_t *code_ptr = AT_SEC(ehdr, shdrs + j) + (syms[i].st_value - shdrs[j].sh_addr);
      Elf64_Addr code_addr = syms[i].st_value;
      check_function(ehdr, 0, code_ptr, code_addr);
    }
  }
}

/*************************************************************/

static void fail(char *reason, int err_code)
{
  fprintf(stderr, "%s (%d)\n", reason, err_code);
  exit(1);
}

static void check_for_shared_object(Elf64_Ehdr *ehdr)
{
  if ((ehdr->e_ident[EI_MAG0] != ELFMAG0) || (ehdr->e_ident[EI_MAG1] != ELFMAG1) || (ehdr->e_ident[EI_MAG2] != ELFMAG2) || (ehdr->e_ident[EI_MAG3] != ELFMAG3))
    fail("not an ELF file", 0);

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    fail("not a 64-bit ELF file", 0);

  if (ehdr->e_type != ET_DYN)
    fail("not a shared-object file", 0);
}

int main(int argc, char **argv)
{
  int fd_in, fd_out;
  size_t len;
  void *p_in, *p_out;
  Elf64_Ehdr *ehdr;

  if (argc != 3)
    fail("expected two file names on the command line", 0);

  /* Open the shared-library input file */
  fd_in = open(argv[1], O_RDONLY);
  if (fd_in == -1)
    fail("could not open input file", errno);

  /* Find out how big the input file is: */
  len = lseek(fd_in, 0, SEEK_END);

  /* Map the input file into memory: */
  p_in = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd_in, 0);
  if (p_in == (void *)-1)
    fail("mmap input failed", errno);

  /* Since the ELF file starts with an ELF header, the in-memory image
     can be cast to a `Elf64_Ehdr *` to inspect it: */
  ehdr = (Elf64_Ehdr *)p_in;

  /* Check that we have the right kind of file: */
  check_for_shared_object(ehdr);

  /* Open the shared-library output file and set its size: */
  fd_out = open(argv[2], O_RDWR | O_CREAT, 0777);
  if (fd_out == -1)
    fail("could not open output file", errno);
  if (ftruncate(fd_out, len))
    fail("could not set output file file", errno);

  /* Map the output file into memory: */
  p_out = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd_out, 0);
  if (p_out == (void *)-1)
    fail("mmap output failed", errno);

  /* Copy input file to output file: */
  memcpy(p_out, p_in, len);

  /* Close input */
  if (munmap(p_in, len))
    fail("munmap input failed", errno);
  if (close(fd_in))
    fail("close input failed", errno);

  ehdr = (Elf64_Ehdr *)p_out;

  enforce(ehdr);

  if (munmap(p_out, len))
    fail("munmap input failed", errno);
  if (close(fd_out))
    fail("close input failed", errno);

  return 0;
}
