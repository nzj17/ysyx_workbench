/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

//#include <isa.h>


#ifndef __ISA_H__
#define __ISA_H__

// Located at src/isa/$(GUEST_ISA)/include/isa-def.h
#include <isa-def.h>

// The macro `__GUEST_ISA__` is defined in $(CFLAGS).
// It will be expanded as "x86" or "mips32" ...
typedef concat(__GUEST_ISA__, _CPU_state) CPU_state;
typedef concat(__GUEST_ISA__, _ISADecodeInfo) ISADecodeInfo;
// monitor
extern unsigned char isa_logo[];
void init_isa();
// reg
extern CPU_state cpu;
void isa_reg_display();
word_t isa_reg_str2val(const char *name, bool *success);

// exec
struct Decode;
int isa_exec_once(struct Decode *s);

// memory
enum { MMU_DIRECT, MMU_TRANSLATE, MMU_FAIL };
enum { MEM_TYPE_IFETCH, MEM_TYPE_READ, MEM_TYPE_WRITE };
enum { MEM_RET_OK, MEM_RET_FAIL, MEM_RET_CROSS_PAGE };
#ifndef isa_mmu_check
int isa_mmu_check(vaddr_t vaddr, int len, int type);
#endif
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type);

// interrupt/exception
vaddr_t isa_raise_intr(word_t NO, vaddr_t epc);
#define INTR_EMPTY ((word_t)-1)
word_t isa_query_intr();

// difftest
bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc);
void isa_difftest_attach();

#endif
//#include "local-include/reg.h"

#ifndef __RISCV_REG_H__
#define __RISCV_REG_H__

#include <common.h>

static inline int check_reg_idx(int idx) {
  IFDEF(CONFIG_RT_CHECK, assert(idx >= 0 && idx < MUXDEF(CONFIG_RVE, 16, 32)));
  return idx;
}

#define gpr(idx) (cpu.gpr[check_reg_idx(idx)])

static inline const char* reg_name(int idx) {
  extern const char* regs[];
  return regs[check_reg_idx(idx)];
}

#endif
//索引 代码中名字 ABI名 含义/约定
//x0 $0 zero 硬连线为 0，写入无效；不是“立即数为零的默认目标”，而是常量零寄存器
//x1 ra ra 返回地址，函数调用时保存返回地址
//x2 sp sp 栈指针，指向栈顶
//x3 gp gp 全局指针，通常指向全局数据区
//x4 tp tp 线程指针，线程局部存储等使用
//x5-x7 t0-t2 t0-t2 临时寄存器，caller-saved
//x8 s0 s0/fp 保存寄存器；常可作帧指针 fp，但是否作为 fp 取决于编译器约定
//x9 s1 s1 保存寄存器，callee-saved
//x10-x17 a0-a7 a0-a7 参数寄存器；其中 a0、a1 也用于函数返回值
//x18-x27 s2-s11 s2-s11 保存寄存器，callee-saved
//x28-x31 t3-t6 t3-t6 临时寄存器，caller-saved
//x1 x2 x3 x4  地址
const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
  printf("yausfpao iujiaio");
for(int i=0;i<32;i++) {
   if(i==1||i==2||i==3||i==4) {
printf("%-5s\t0x%08x\t0x%08x\n",regs[i],cpu.gpr[i],cpu.gpr[i]);}
   else {printf("%-5s\t0x%08x\t%u\n",regs[i],cpu.gpr[i],cpu.gpr[i]);}
}//32 bin : u 0-4294967295
 printf("PC   \t0x%08x\t0x%08x\n",cpu.pc,cpu.pc); 
}
word_t isa_reg_str2val(const char *s, bool *success) {
  return 0;
}
