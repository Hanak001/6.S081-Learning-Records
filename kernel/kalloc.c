// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#define PA2IDX(pa) (((uint64)pa) >> 12)
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  uint64  ref_count[(PHYSTOP - KERNBASE) / PGSIZE];
} kmem;
void
increase_rc(void *pa)
{
  acquire(&kmem.lock);
  kmem.ref_count[(((uint64)pa - KERNBASE) / PGSIZE)]++;
  release(&kmem.lock);
}
//减少计数
void
decrease_rc(void *pa)
{
  acquire(&kmem.lock);
  kmem.ref_count[(((uint64)pa - KERNBASE) / PGSIZE)]--;
  release(&kmem.lock);
}
//获取计数
int
get_rc(void *pa)
{
  acquire(&kmem.lock);
  int rc = kmem.ref_count[(((uint64)pa - KERNBASE) / PGSIZE)];
  release(&kmem.lock);
  return rc;
}

void add_ref_count(uint64 pa)
 { 
   // write operation locking
   acquire(&kmem.lock);
   kmem.ref_count[(pa - KERNBASE) / PGSIZE] ++;
   release(&kmem.lock);
 }
 
 uint64 get_ref_count(uint64 pa)
 {
   // read operation without lock
   return kmem.ref_count[(pa - KERNBASE) / PGSIZE];
 }



void
kinit()
{

  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    acquire(&kmem.lock);
     // init reference count as 1
     kmem.ref_count[((uint64)p - KERNBASE) / PGSIZE] = 1; 
     release(&kmem.lock);
    kfree(p);
  }
    
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
   acquire(&kmem.lock); // lock
   // calculate index 
   uint64 idx = ((uint64)pa - KERNBASE) / PGSIZE;
   if(-- kmem.ref_count[idx] == 0)
   {
     release(&kmem.lock);
 
     memset(pa, 1, PGSIZE);
     r = (struct run*)pa;
 
     acquire(&kmem.lock);
     r->next = kmem.freelist;
     kmem.freelist = r;
   }
   // Fill with junk to catch dangling refs.
   
   release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
  {
    kmem.freelist = r->next;
    kmem.ref_count[((uint64)r - KERNBASE) / PGSIZE] += 1;
  }
    
  release(&kmem.lock);

  if(r)
  {
    memset((char*)r, 5, PGSIZE); // fill with junk
    //increase_rc((void*)r);
  }
    
  return (void*)r;
}
