extern "C" {
#include "types.h"
#include "amd64.h"
#include "kernel.h"
#include "mmu.h"
#include "spinlock.h"
#include "condvar.h"
#include "queue.h"
#include "proc.h"
#include "cpu.h"
}

#include "vm.hh"

long
sys_fork(void)
{
  int flags;

  if(argint32(0, &flags) < 0)
    return -1;
  return fork(flags);
}

long
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

long
sys_wait(void)
{
  return wait();
}

long
sys_kill(void)
{
  int pid;

  if(argint32(0, &pid) < 0)
    return -1;
  return kill(pid);
}

long
sys_getpid(void)
{
  return myproc()->pid;
}

long
sys_sbrk(void)
{
  uptr addr;
  int n;

  if(argint32(0, &n) < 0)
    return -1;
  addr = myproc()->brk;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

long
sys_sleep(void)
{
  int n;
  u32 ticks0;
  
  if(argint32(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    cv_sleep(&cv_ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
long
sys_uptime(void)
{
  u64 xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

long
sys_map(void)
{
  uptr addr;
  u64 len;

  if (argint64(0, &addr) < 0)
    return -1;
  if (argint64(1, &len) < 0)
    return -1;

  vmnode *vmn = new vmnode(PGROUNDUP(len) / PGSIZE);
  if (vmn == 0)
    return -1;

  if (myproc()->vmap->insert(vmn, PGROUNDDOWN(addr)) < 0) {
    delete vmn;
    return -1;
  }

  return 0;
}

long
sys_unmap(void)
{
  uptr addr;
  uptr len;

  if (argint64(0, &addr) < 0)
    return -1;
  if (argint64(1, &len) < 0)
    return -1;

  uptr align_addr = PGROUNDDOWN(addr);
  uptr align_len = PGROUNDUP(addr + len) - align_addr;
  if (myproc()->vmap->remove(align_addr, align_len) < 0)
    return -1;

  updatepages(myproc()->vmap->pml4,
              (void*) align_addr, (void*) (align_addr+align_len-1), 0);
  tlbflush();
  return 0;
}

long
sys_halt(void)
{
  int i;
  const char s[] = "Shutdown";

  for(i = 0; i < 8; i++)
    outb(0x8900, s[i]);
  return 0;
}