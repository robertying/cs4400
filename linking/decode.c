#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include "decode.h"

#define MODRM_MOD(v) (((v) >> 6) & 0x3)
#define MODRM_REG(v) (((v) >> 3) & 0x7)
#define MODRM_RM(v) ((v) & 0x7)

void decode(instruction_t *ins, code_t *code_ptr, Elf64_Addr code_addr)
{
  int size = 4;

  ins->length = 0;

  if ((code_ptr[0] == 0x48)
      && (code_ptr[1] == 0x8B)
      && (MODRM_MOD(code_ptr[2]) == 0)
      && (MODRM_RM(code_ptr[2]) == 5)) {
    /* movq $<addr>(%RIP), <reg>
       Although this case would be covered by `mov` variants
       below, we want to detect RIP-relative addresses,
       since those can refer to GOT entries. */
    int rel_addr = *(int *)(code_ptr + 3);
    ins->op = MOV_ADDR_TO_REG_OP;
    ins->length += 7;
    ins->addr = code_addr + 7 + rel_addr;
    return;
  }

  if ((code_ptr[0] == 0xFF)
      && (code_ptr[1] == 0x25)) {
    /* jmp <addr> */
    int rel_addr = *(int *)(code_ptr + 2);
    ins->op = JMP_TO_ADDR_OP;
    ins->length += 6;
    ins->addr = code_addr + 6 + rel_addr;
    return;
  }

  if ((code_ptr[0] >= 0x70)
      && (code_ptr[0] <= 0x7F)) {
    /* j[cc] <addr> */
    int rel_addr = code_ptr[1];
    ins->op = MAYBE_JMP_TO_ADDR_OP;
    ins->length += 2;
    ins->addr = code_addr + 2 + rel_addr;
    return;
  }
  
  if (code_ptr[0] == 0xe8) {
    /* call */
    int rel_addr = *(int *)(code_ptr + 1);
    ins->op = CALL_OP;
    ins->addr = code_addr + 5 + rel_addr;
    ins->length += 5;
    return;
  }
  
  if (code_ptr[0] == 0x90) {
    /* nop */
    ins->op = OTHER_OP;
    ins->length += 1;
    return;
  }

  if (code_ptr[0] == 0x48) {
    /* Effectively a "q" suffix */
    code_ptr++;
    ins->length++;
    size = 8;
  }

  while (code_ptr[0] == 0x66) {
    /* Effectively a "w" suffix, can repeat */
    code_ptr++;
    ins->length++;
    size = 2;
  }

  if (code_ptr[0] == 0x2E) {
    /* CS segment */
    code_ptr++;
    ins->length++;
  }

  if ((code_ptr[0] == 0x0F)
      && (code_ptr[1] == 0x1F)
      && (MODRM_REG(code_ptr[2]) == 0)) {
    /* nop */
    ins->op = OTHER_OP;
    ins->length += 3;
    if (MODRM_MOD(code_ptr[2]) != 3) {
      if (MODRM_MOD(code_ptr[2]) == 1)
	ins->length += 1;
      else if (MODRM_MOD(code_ptr[2]) == 2)
	ins->length += 4;
      if (MODRM_RM(code_ptr[2]) == 4)
	ins->length++;
    }
    return;
  }

  if ((code_ptr[0] == 0x53) /* push */
      || (code_ptr[0] == 0x54) /* push */
      || (code_ptr[0] == 0x55) /* push */
      || (code_ptr[0] == 0x5b) /* pop */
      || (code_ptr[0] == 0x5c) /* pop */
      || (code_ptr[0] == 0x5d) /* pop */
      || 0) {
    ins->op = OTHER_OP;
    ins->length += 1;
    return;
  }
    

  if ((code_ptr[0] == 0x63)    /* mov */
      || (code_ptr[0] == 0x89) /* mov */
      || (code_ptr[0] == 0x8B) /* mov */
      || (code_ptr[0] == 0x39) /* cmp */
      || (code_ptr[0] == 0x3B) /* cmp */
      || (code_ptr[0] == 0x85) /* test */
      || (code_ptr[0] == 0x01) /* add */
      || (code_ptr[0] == 0x03) /* add */
      || (code_ptr[0] == 0x29) /* sub */
      || (code_ptr[0] == 0x28) /* sub */
      || (code_ptr[0] == 0x31) /* xor */
      || (code_ptr[0] == 0x32) /* xor */
      || 0) {
    /* mov, cmp, etc., variants */
    ins->op = OTHER_OP;
    if (MODRM_MOD(code_ptr[1]) == 3)
      ins->length += 2;
    else if ((MODRM_MOD(code_ptr[1]) == 0)
	     && (MODRM_RM(code_ptr[1]) == 5))
      ins->length += 6;
    else if ((MODRM_MOD(code_ptr[1]) != 3)
	     && (MODRM_RM(code_ptr[1]) == 4)) {
      ins->length += 3;
      if (MODRM_MOD(code_ptr[1]) == 1)
	ins->length++;
      else if (MODRM_MOD(code_ptr[1]) == 2)
	ins->length += 4;
    } else {
      ins->length += 2;
      if (MODRM_MOD(code_ptr[1]) == 1)
	ins->length++;
      else if (MODRM_MOD(code_ptr[1]) == 2)
	ins->length += 4;
    }
    return;
  }

  if ((code_ptr[0] >= 0xB8)
      && (code_ptr[0] <= 0xBF)) {
    /* more mov $<imm>, <reg> variants */
    ins->op = OTHER_OP;
    ins->length += 1 + size;
    return;
  }

  if (code_ptr[0] == 0x83) {
    /* add/sub byte */
    ins->op = OTHER_OP;
    ins->length += 3;
    return;
  }

  if (code_ptr[0] == 0x81) {
    /* add/sub word */
    ins->op = OTHER_OP;
    ins->length += 4;
    return;
  }

  if ((code_ptr[0] == 0x83)
      && (MODRM_REG(code_ptr[1]) == 7)) {
    /* cmp $<imm>, <reg> */
    ins->op = OTHER_OP;
    ins->length += 3;
    return;
  }

  if (code_ptr[0] == 0xfe) {
    /* rep, especially with ret */
    ins->length++;
    /* fall through... */
  }

  if (code_ptr[0] == 0xc3) {
    /* ret */
    ins->op = RET_OP;
    ins->length++;
    return;
  }

  if ((code_ptr[0] == 0xf3)
      && (code_ptr[1] == 0xc3)) {
    /* repz ret */
    ins->op = RET_OP;
    ins->length += 2;
    return;
  }

  if ((code_ptr[0] == 0xe9)
      || (code_ptr[0] == 0xeb)) {
    /* jmp <addr> */
    int dp = ((code_ptr[0] == 0xeb) ? 1 : 4);
    int rel_addr = ((dp == 1) ? ((char *)code_ptr)[1] : *(int *)(code_ptr + 1));
    ins->op = JMP_TO_ADDR_OP;
    ins->length += 1 + dp;
    ins->addr = code_addr + 1 + dp + rel_addr;
    return;
  }

  if ((code_ptr[0] >= 0x90)
      && (code_ptr[0] <= 0x98)) {
    /* xchg */
    ins->op = OTHER_OP;
    ins->length++;
    return;
  }

  fprintf(stderr, "unrecognized instruction starts %x\n", code_ptr[0]);
  exit(1);
}

void replace_with_crash(code_t *code_ptr, instruction_t *ins)
{
  if (ins->length == 1) {
    /* single-byte int $3 encoding */
    code_ptr[0] = 0xCC;
  } else {
    int i;

    if (ins->length < 2) {
      fprintf(stderr, "cannot crash within too-small length %d\n",
              ins->length);
      exit(1);
    }
    
    /* int $3 */
    code_ptr[0] = 0xCD;
    code_ptr[1] = 3;
    
    /* fill rest with nop */
    for (i = 2; i < ins->length; i++)
      code_ptr[i] = 0x90;
  }
}
