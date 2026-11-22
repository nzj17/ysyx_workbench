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

#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <isa.h>

#if   defined(CONFIG_PMEM_MALLOC) 
static uint8_t *pmem = NULL;
#else // CONFIG_PMEM_GARRAY
// CONFIG_MSIZE 0x8000000=128*1024*1024bytes=128MB
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};
//pmem array .+index is menu-computer memory虚拟Vaddr
//0   1   ... ..134,217,728-1=7FFFFFF
//0x0 0x1 ... ..0x8000000-1  实际物理内存编号从0开始的host？?<<< guest_to_host函数(menu物理Paddr) 转
//0x80000000 ...0x88000000-1 menu-computer memory物理Paddr<<<虚拟Paddr=paddr 转
//0x80000000 ...0x88000000-1 menu-computer memory虚拟Paddr 

//i don‘t know 实际的虚拟机对应的物理addr after all
//0x0x7fffefbff010(rand)...实际linux的gcc给的PMEM数组的虚拟addr《malloc》
//nemu-computer 的memory地址 include 物理Paddr和虚拟Vaddr
//关于x扫描：虚拟vaddr要先转为nemu的物理paddr。
//i feel 只有把抽象的虚拟addr转成实际的物理addr才能通过解引用(这里内存是c-array)获得memory data
#endif  
uint8_t* guest_to_host(paddr_t paddr) 
{ return pmem + (paddr - CONFIG_MBASE); } 
paddr_t host_to_guest(uint8_t *haddr)
{ return (haddr - pmem) + CONFIG_MBASE; }
static word_t pmem_read(paddr_t addr, int len) {
  word_t ret = host_read(guest_to_host(addr), len);
  return ret;
} 
static void pmem_write(paddr_t addr, int len, word_t data) {
  host_write(guest_to_host(addr), len, data);
}
static void out_of_bound(paddr_t addr) {
  panic("address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
      addr, PMEM_LEFT, PMEM_RIGHT, cpu.pc);
} 
void init_mem() {
#if   defined(CONFIG_PMEM_MALLOC) //y
  pmem = malloc(CONFIG_MSIZE);
  assert(pmem);  //pmem points to heap
#endif
  IFDEF(CONFIG_MEM_RANDOM, memset(pmem, rand(), CONFIG_MSIZE)); 
//assign peme[0x8..] as a!!a nr(so pmem[0]=pmem[n])
  Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
}
word_t paddr_read(paddr_t addr, int len) {
  if (likely(in_pmem(addr))) return pmem_read(addr, len);
  IFDEF(CONFIG_DEVICE, return mmio_read(addr, len));
//MMIO 外设
  out_of_bound(addr);
  return 0;
}

void paddr_write(paddr_t addr, int len, word_t data) {
  if (likely(in_pmem(addr))) { pmem_write(addr, len, data); return; }
  IFDEF(CONFIG_DEVICE, mmio_write(addr, len, data); return);
  out_of_bound(addr);
}

